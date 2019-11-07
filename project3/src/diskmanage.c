#include "diskmanage.h"

// Allocate an on-disk page from the free page list and
// update information of header page
Pagenum_t file_alloc_page(HeaderPage_t * header, int fd){

    NodePage_t allocated_page;
    // file_read_page(HEADER_PAGE_NUMBER, PAGE_ADDRESS(header), fd);

    if(header->free_page_num != 0){ // If there is at least one free page in file

        // Selecting a page number of a page that will be allocated as a node page
        Pagenum_t page_num = header->free_page_num;
        
        // Read the free page to get next free page number that should be connected to header page structure variable
        // And update data of header page in  file on the disk
        FreePage_t free_page;
        file_read_page(page_num, (Page_t *)&free_page, fd);

        header->free_page_num = free_page.next_free_page_num;
        // file_write_page(HEADER_PAGE_NUMBER, PAGE_ADDRESS(header), fd);

        // Create a new node page structure variable and write it on the disk
        file_write_page(page_num, (Page_t *)&allocated_page, fd);

        return page_num;
    }
    else{ // If there isn't any free page left

        // Array of new pages to be written on the file
        Page_t new_pages[DEFAULT_NEW_PAGE_COUNT];

        // Create new node page and allocate
        NodePage_t new_node_page;
        new_pages[0] = (Page_t)new_node_page;

        Pagenum_t new_page_num = header->num_page;

        // new_pages[i]'s page number = new_page_num + i;
        for(int i = 1; i < DEFAULT_NEW_PAGE_COUNT - 1; ++i)
            ((FreePage_t *)(new_pages + i))->next_free_page_num = new_page_num + i + 1;
        ((FreePage_t *)(new_pages + DEFAULT_NEW_PAGE_COUNT - 1))->next_free_page_num = NO_MORE_FREE_PAGE;

        // Update information to the header and file
        header->free_page_num = new_page_num + 1;
        header->num_page = header->num_page + DEFAULT_NEW_PAGE_COUNT;
        // file_write_page(HEADER_PAGE_NUMBER, (Page_t *)&header, fd);

        file_write_multi_pages(new_page_num, new_pages, DEFAULT_NEW_PAGE_COUNT, fd);

        return new_page_num;
    }
}

// Free an on-disk page to the free page list
void file_free_page(Pagenum_t pagenum, HeaderPage_t * header, int fd){

    if(pagenum > 0 && pagenum < header->num_page){
        FreePage_t free_page;

        // Adding new free page in front of the free page list
        // [header - old_free_page_num]  ->  [header - new_free_page_num - old_free_page_num]
        free_page.next_free_page_num = header->free_page_num;

        // Updating data of pages on the disk
        file_write_page(pagenum, (Page_t *)&free_page, fd);

        header->free_page_num = pagenum;
        // file_write_page(HEADER_PAGE_NUMBER, (Page_t *)&header, fd);
    }
    else printf("Error: It is not allowed to free header page or non existing page!\n");
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(Pagenum_t pagenum, Page_t* dest, int fd){
    pread(fd, dest, PAGE_SIZE, PAGE_OFFSET(pagenum));
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(Pagenum_t pagenum, const Page_t* src, int fd){
    pwrite(fd, src, PAGE_SIZE, PAGE_OFFSET(pagenum));
    fdatasync(fd);
}

// Write some multiple pages stored in an array into the file
void file_write_multi_pages(Pagenum_t pagenum, const Page_t* src, const int num_page, int fd){
    pwrite(fd, src, PAGE_SIZE * num_page, PAGE_OFFSET(pagenum));
    fdatasync(fd);
}

// If the file exists in given pathname, open it in fd and return 1
// If the file doesn't exist or has error, create a new file and return 0
int file_open_if_exist(const char * pathname, int * fd){

    int status;

    if(access(pathname, F_OK | R_OK | W_OK )){
        *fd = open(pathname, O_RDWR | O_CREAT, 0644);
        status = 0;
    }
    else{
        *fd = open(pathname, O_RDWR, 0644);
        status = 1;
    }
    return status;
}

// NEW FUNCTIONS FOR THE DISKED-BASED B+ TREE for DEBUG

// Print the information of current header page
void print_header_page_from_disk(int fd){

    HeaderPage_t header;
    file_read_page(HEADER_PAGE_NUMBER, PAGE_ADDRESS(header), fd);
    print_header_page(header);
}

void print_header_page(HeaderPage_t header){

    printf("<header page status> ");
    printf("Free page number: %ld / ", header.free_page_num);
    printf("Root page number: %ld / ", header.root_page_num);
    printf("Number of pages in current file: %ld\n", header.num_page);
}

// Print the information of a single node page
void print_node_from_disk(Pagenum_t pagenum, int fd){

    if (pagenum == HEADER_PAGE_NUMBER){
        print_header_page_from_disk(fd);
        return;
    }
    NodePage_t page;
    file_read_page(pagenum, PAGE_ADDRESS(page), fd);
    print_node(page, pagenum);
}

void print_node(NodePage_t page, Pagenum_t pagenum){

    if(page.is_leaf){
        printf("<Leaf page [%ld] status> ", pagenum);
        printf("Parent page number: %ld / ", page.parent_page_num);
        printf("Number of keys: %d / ", page.num_key);
        printf("Right sibling page number: %ld / ", page.right_page_num);
        printf("Stored records (key, value) / ");
        for(int i = 0; i < page.num_key; i++)
            printf("(%ld, %s) ", page.lf_record[i].key, page.lf_record[i].value);
    }
    else{
        printf("<Non-leaf page [%ld] status> ", pagenum);
        printf("Parent page number: %ld / ", page.parent_page_num);
        printf("Number of keys: %d / ", page.num_key);
        printf("Extra page number: %ld / ", page.extra_page_num);
        printf("Stored records (key, page number) / ");
        for(int i = 0; i < page.num_key; i++)
            printf("(%ld, %ld) ", page.in_record[i].key, page.in_record[i].page_num);
    }
    printf("\n");
}