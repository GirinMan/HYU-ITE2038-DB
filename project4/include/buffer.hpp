#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "diskmanage.hpp"

typedef struct TableInfo_t{

    int num_table;
    bool in_use[MAX_TABLE_NUMBER + 1];
    char pathname[MAX_TABLE_NUMBER + 1][512];
    int fd[MAX_TABLE_NUMBER + 1];

} TableInfo_t;

typedef struct BufferBlock_t{

    // Physical frame which contains up-to-date contents of target page
    Page_t frame;

    // The unique ID of table(file) containing target page
    int table_id;

    // The target page number within a table
    Pagenum_t page_num;

    // Indicates whether this block is dirty or not
    bool is_dirty;

    // Indicates wheter this block is pinned or not
    int pin_count;
    
    // Points previous and next buffer blocks
    // Buffer blocks are managed by the LRU(least recently used) list
    struct BufferBlock_t * prev, * next;   

} BufferBlock_t;

extern struct BufferBlock_t * buffer;
extern struct TableInfo_t tables;

/* 
    Functions for operating database in top layer
*/

// Allocate the buffer pool (array) with the given number of entries.
// Initialize other fields such as state info, LRU info
// If success, return 0. Otherwise, return non zero value.
int init_db(int num_buf);

// Open existing data file using ‘pathname’ or create one if not existed.
// If success, return the unique table id, which represents the own table in this database. Otherwise,
// return negative value.
// You have to maintain a table id once open_table () is called, which is matching file descriptor
// or file pointer depending on your previous implementation. (table id ≥1 and maximum allocated id is set to 10)
int open_table (char * pathname);

// Write the pages relating to this table to disk and close the table
int close_table(int table_id);

// Destroy buffer manager
int shutdown_db(void);


// Functions for read/write pages in buffer.
// If the page is not in buffer pool (cache miss), read page from disk and maintain that page in buffer block.
// Page modification only occurs in memory buffer. If the page frame in buffer is updated,
// mark the buffer block as dirty.
// According to LRU policy, least recently used buffer is the victim for page eviction.
// Writing page to disk occurs during LRU page eviction.

// Read page from buffer and increase pin count by 1
// This may cause page enviction
BufferBlock_t * buffer_read_page(int table_id, Pagenum_t page_num);

// Write page to buffer
// This makes frame dirty and increases pin count by 1
void buffer_write_page(BufferBlock_t * frame, Page_t page);

// Flush all data of frame into coresponding disk page 
void buffer_flush_page(BufferBlock_t * frame);

// Allocate new page from disk and stage it on a buffer frame
// This increases pin count by 1
BufferBlock_t * buffer_allocate_page(int table_id);

// Free an allocated page from the buffer
// Flush the changes into the disk and remove frame from the buffer
void buffer_free_page(BufferBlock_t * frame);

// Decrease the pin count of frame in buffer by count
void buffer_unpin_page(BufferBlock_t * frame, int count);

// Decrease the pin count of frame in buffer by 1
void buffer_unpin_page(BufferBlock_t * frame);

// Check if the parent and it child is connected well or not
void buffer_check_relationship();

// Print some information about the frame in buffer
void buffer_print_page(BufferBlock_t * frame);

// Print information of all of the frames
// Currently exist in buffer
void buffer_print_all();

// Print information of a currently opened table
void print_all_tables();

#endif /* __BUFFER_H__ */