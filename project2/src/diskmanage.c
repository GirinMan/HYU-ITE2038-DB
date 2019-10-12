#include "diskmanage.h"

int fd;
FILE * fp;

HeaderPage_t header;
pagenum_t root_page_num;

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){

    if(header.free_page_num != 0){ // If there is at least one free page in file

        // Selecting a page number of a page that will be allocated as a node page
        pagenum_t page_num = header.free_page_num;
        
        // Read the free page to get next free page number that should be connected to header page structure variable
        // And update data of header page in  file on the disk
        FreePage_t free_page;
        file_read_page(page_num, (page_t *)&free_page);

        update_header(free_page.next_free_page_num, header.root_page_num, header.num_page);

        // Create a new node page structure variable and write it on the disk
        NodePage_t allocated_page;
        file_write_page(page_num, (page_t *)&allocated_page);

        return page_num;
    }
    else{ // If there isn't any free page left

        // Array of new pages to be written on the file
        page_t new_pages[DEFAULT_NEW_PAGE_COUNT];

        // Create new node page and allocate
        NodePage_t new_node_page;
        new_pages[0] = (page_t)new_node_page;

        pagenum_t new_page_num = header.num_page;

        // new_pages[i]'s page number = new_page_num + i;
        for(int i = 1; i < DEFAULT_NEW_PAGE_COUNT - 1; ++i)
            ((FreePage_t *)(new_pages + i))->next_free_page_num = new_page_num + i + 1;
        ((FreePage_t *)(new_pages + DEFAULT_NEW_PAGE_COUNT - 1))->next_free_page_num = NO_MORE_FREE_PAGE;

        // Update information to the header and file
        update_header(new_page_num + 1, header.root_page_num, header.num_page + DEFAULT_NEW_PAGE_COUNT);
        file_write_multi_pages(new_page_num, new_pages, DEFAULT_NEW_PAGE_COUNT);

        return new_page_num;
    }
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){

    if(pagenum > 0 && pagenum < header.num_page){
        FreePage_t free_page;

        // Adding new free page in front of the free page list
        // [header - old_free_page_num]  ->  [header - new_free_page_num - old_free_page_num]
        free_page.next_free_page_num = header.free_page_num;

        // Updating data of pages on the disk
        file_write_page(pagenum, (page_t *)&free_page);
        update_header(pagenum, header.root_page_num, header.num_page);
        printf("Page number [%ld] was freed!\n", pagenum);
    }
    else printf("Error: It is not allowed to free header page or non existing page!\n");
}

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
    pread(fd, dest, PAGE_SIZE, PAGE_OFFSET(pagenum));
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
    pwrite(fd, src, PAGE_SIZE, PAGE_OFFSET(pagenum));
    fdatasync(fd);
}

// Write some multiple pages stored in an array into the file
void file_write_multi_pages(pagenum_t pagenum, const page_t* src, const int num_page){
    pwrite(fd, src, PAGE_SIZE * num_page, PAGE_OFFSET(pagenum));
    fdatasync(fd);
}

// If the file exists in given pathname, open it in fd and return 1
// If the file doesn't exist or has error, create a new file and return 0
int file_open_if_exist(const char * pathname){

    if(access(pathname, F_OK | R_OK | W_OK )){
        fd = open(pathname, O_RDWR | O_CREAT, 0644);
        printf("File [%s] is not able to be opened.\n", pathname);
        printf("New file is created.\n");
        return 0;
    }
    else{
        fd = open(pathname, O_RDWR, 0644);
        printf("The file [%s] has been opened in [fd: %d].\n", pathname, fd);
        return 1;
    }
}

// Update information of both in-memory and on-disk header pages with given values
void update_header(pagenum_t free_page_num, pagenum_t root_page_num, pagenum_t num_page){
    header.free_page_num = free_page_num;
    header.root_page_num = root_page_num;
    header.num_page = num_page;

    file_write_page(HEADER_PAGE_NUMBER, (page_t *)&header);
}

// NEW FUNCTIONS FOR THE DISKED-BASED B+ TREE

// Open existing data file using ‘pathname’ or create one if not existed.
// If success, return the unique table id, which represents the own table in this database. Otherwise,
// return negative value. (This table id will be used for future assignment.)
int open_table (char * pathname){

    // When a file already exists in the path
    // Update the in-memory header page with data stored in the file
    if(file_open_if_exist(pathname)){
        file_read_page(HEADER_PAGE_NUMBER, (page_t *)&header);
    }

    // When the file in pathname is not available
    // Create a new file and initialize the header file
    else update_header(0, NO_ROOT_NODE, 1);

    return fd;
}


// Find the matching record and delete it if found.
// If success, return 0. Otherwise, return non-zero value.
int db_delete (int64_t key){
    return 2;
}

// Print the information of current header page
void print_header_page(){
    printf("<Current header page> ");
    printf("Free page number: %ld / ", header.free_page_num);
    printf("Root page number: %ld / ", header.root_page_num);
    printf("Number of pages in current file: %ld\n", header.num_page);
}

// Print the information of a single node page
void print_node_page(pagenum_t pagenum){

    NodePage_t page;
    file_read_page(pagenum, PAGE_ADDRESS(page));

    if(page.is_leaf){
        printf("<Leaf page [%ld] status> ", pagenum);
        printf("Number of keys: %d / ", page.num_key);
        printf("Right sibling page number: %ld / ", page.right_page_num);
        printf("Stored records (key, value) / ");
        for(int i = 0; i < page.num_key; i++)
            printf("(%ld, %s) ", page.lf_record[i].key, page.lf_record[i].value);
    }
    else{
        printf("<Non-leaf page [%ld] status> ", pagenum);
        printf("Number of keys: %d / ", page.num_key);
        printf("Extra page number: %ld / ", page.extra_page_num);
        printf("Stored records (key, page number) / ");
        for(int i = 0; i < page.num_key; i++)
            printf("(%ld, %ld) ", page.in_record[i].key, page.in_record[i].page_num);
    }
    printf("\n");
    
}

void print_node(NodePage_t page, pagenum_t pagenum){

    if(page.is_leaf){
        printf("<Leaf page [%ld] status> ", pagenum);
        printf("Number of keys: %d / ", page.num_key);
        printf("Right sibling page number: %ld / ", page.right_page_num);
        printf("Stored records (key, value) / ");
        for(int i = 0; i < page.num_key; i++)
            printf("(%ld, %s) ", page.lf_record[i].key, page.lf_record[i].value);
    }
    else{
        printf("<Non-leaf page [%ld] status> ", pagenum);
        printf("Number of keys: %d / ", page.num_key);
        printf("Extra page number: %ld / ", page.extra_page_num);
        printf("Stored records (key, page number) / ");
        for(int i = 0; i < page.num_key; i++)
            printf("(%ld, %ld) ", page.in_record[i].key, page.in_record[i].page_num);
    }
    printf("\n");
}