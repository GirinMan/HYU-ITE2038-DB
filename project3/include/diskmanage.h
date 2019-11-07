#ifndef __DISKMANAGE_H__
#define __DISKMANAGE_H__

#include <utility.h>

// Struct that defines a record in leaf nodes
// key(8B) + value(120B) -> Order = 32
typedef struct LeafRecord{

    keyval_t key;
    char value[120];

} LeafRecord;

// Struct that defines a record in internal nodes
// key(8B) + page number(8B) -> Order = 249
typedef struct InternalRecord{
    
    keyval_t key;
    Pagenum_t page_num;

} InternalRecord;

/*
    In-memory page structures used to temporary store data on the memory
*/

typedef struct HeaderPage_t {

    // points the first free pages
    // 0 if there's no free page left
    Pagenum_t free_page_num; 

    // points the root page within the data file
    Pagenum_t root_page_num;

    // denote the number of pages existing in this data file now
    Pagenum_t num_page;

    // unused bytes of header page
    char reserved[4072];

} HeaderPage_t;

typedef struct FreePage_t {

    // points the next free page
    // 0 if the page is the end of the free page list
    Pagenum_t next_free_page_num;

    // unused bytes of header page
    char reserved[4088];
    
} FreePage_t;

typedef struct NodePage_t {

    // Page Header ------------------------------
    
    // points the position of parent page
    Pagenum_t parent_page_num;

    // indicate whether it is a leaf page or not
    int is_leaf;

    // denote the number of keys within this page
    int num_key; 

    // unused bytes of node pade
    char reserved[104];

    // ------------------------------------------

    union
    {
    // an extra page number for interpreting key range
    // pointing at the leftmost child
    Pagenum_t extra_page_num;

    // stores the page's right sibling page's page number
    // 0 if the page is a rightmost page
    Pagenum_t right_page_num;
    };

    union
    {
    // array storing records in the page
    // maximum 248 records per page(branching factor = 249)
    InternalRecord in_record[248];
       
    // array storing records in the page
    // maximum 31 records per page(branching factor = 32)
    LeafRecord lf_record[31];
    };

} NodePage_t;

typedef union Page_t{
    HeaderPage_t header_page;
    FreePage_t free_page;
    NodePage_t node_page;
} Page_t;

/* 
    Functions for handling file I/O
*/

// Allocate an on-disk page from the free page list
Pagenum_t file_alloc_page(HeaderPage_t * header, int fd);

// Free an on-disk page to the free page list
void file_free_page(Pagenum_t pagenum, HeaderPage_t * header, int fd);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(Pagenum_t pagenum, Page_t* dest, int fd);

// Write an in-memory page(src) to the on-disk page
void file_write_page(Pagenum_t pagenum, const Page_t* src, int fd);

// Write some multiple pages stored in an array into the file
void file_write_multi_pages(Pagenum_t pagenum, const Page_t* src, const int num_page, int fd);

// If the file exists in given pathname, open it in fd and return 1
// If the file doesn't exist or has error, return 0
int file_open_if_exist(const char * pathname, int * fd);

/*
    Functions for debugging
*/

// Print the information of current header page
void print_header_page_from_disk(int fd);
void print_header_page(HeaderPage_t header_page);

// Print the information of a single node page
void print_node_from_disk(Pagenum_t pagenum, int fd);
void print_node(NodePage_t node_page, Pagenum_t pagenum);

#endif /* __DISKMANAGE__H__ */