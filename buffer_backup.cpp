#include <buffer_backup.hpp>

BufferBlock_t * buffer = NULL;
TableInfo_t tables;
 
void clear(BufferBlock_t * frame){
    frame->page_num = 0;
    frame->is_dirty = false;
    frame->pin_count = 0;
    frame->table_id = 0;
}

void add_page_into_front(BufferBlock_t * frame){

    if (frame == buffer) return;
    
    BufferBlock_t * prev = frame->prev, * next = frame->next;

    prev->next = next;
    next->prev = prev;

    buffer->prev = frame;
    frame->next = buffer;
    buffer = frame;
}



// Open existing data file using ‘pathname’ or create one if not existed.
// If success, return the unique table id, which represents the own table in this database. Otherwise,
// return negative value.
// You have to maintain a table id once open_table () is called, which is matching file descriptor
// or file pointer depending on your previous implementation. (table id ≥1 and maximum allocated id is set to 10)
int open_table (char * pathname){

    int i, result, fd = -1, empty_id = 0;
    HeaderPage_t header;

    if(!pathname || pathname[0] == 0 || tables.num_table == (MAX_TABLE_NUMBER))
        return FAILURE;

    result = file_open_if_exist(pathname, &fd);

    if(fd == -1) return FAILURE;

    // When the file in pathname is not available
    // Create a new file and initialize the header file
    if(!result){
        header.free_page_num = 0;
        header.root_page_num = NO_ROOT_NODE;
        header.num_page = 1;
        file_write_page(HEADER_PAGE_NUMBER, (Page_t *)&header, fd);
    }

    for (i = 1; i <= MAX_TABLE_NUMBER; ++i){

        if(tables.pathname[i][0] != 0 ){
            // If same table had been opened before
            if (!strcmp(pathname, tables.pathname[i])){

                // If the table is not in used, store table id
                if (tables.in_use[i] == false){
                    empty_id = i;
                    break;
                }

                close(fd);
                return FAILURE;
            }
        }

    }

    for (i = 1; i <= MAX_TABLE_NUMBER; ++i){
        if (empty_id == 0 && tables.in_use[i] == false){
            empty_id = i;
        }
    }

    tables.fd[empty_id] = fd;
    tables.in_use[empty_id] = true;
    strncpy(tables.pathname[empty_id], pathname, 511);
    tables.num_table++;
    
    return empty_id;
}

// Write the pages relating to this table to disk and close the table
int close_table(int table_id){

    if (tables.in_use[table_id] == false){
        return FAILURE;
    }

    BufferBlock_t * temp = buffer;

    while(temp != NULL){

        //buffer_print_page(temp);
        if(temp->table_id == table_id){
            buffer_flush_page(temp);
            clear(temp);
        }
        temp = temp->next;
    }

    close(tables.fd[table_id]);
    //printf("file closed.\n");

    tables.fd[table_id] = 0;
    tables.in_use[table_id] = false;
    tables.num_table--;

    return SUCCESS;
}



// Functions for read/write pages in buffer.
// If the page is not in buffer pool (cache miss), read page from disk and maintain that page in buffer block.
// Page modification only occurs in memory buffer. If the page frame in buffer is updated,
// mark the buffer block as dirty.
// According to LRU policy, least recently used buffer is the victim for page eviction.
// Writing page to disk occurs during LRU page eviction.

// Read page from buffer and increase pin count by 1
BufferBlock_t * buffer_read_page(int table_id, Pagenum_t page_num){

    BufferBlock_t * temp = buffer, * empty = NULL, * victim = NULL, * tail;

    if (table_id < 1 || table_id > MAX_TABLE_NUMBER)
        return NULL;

    // Find requested frame from the buffer list
    while(temp != NULL){

        // If page exists, increases pin count and return the pointer of buffer frame
        if( temp->table_id == table_id && temp->page_num == page_num){
            temp->pin_count++;
            return temp;
        }

        // If current buffer block is not in use, store its pointer
        if(empty == NULL && temp->table_id == 0){
            empty = temp;
        }

        // If current buffer frame is in use and its pin count is 0,
        // Mark it as victim for page enviction
        if(victim == NULL && temp->table_id != 0 && temp->pin_count == 0){
            victim = temp;
        }

        if( temp->next == 0) tail = temp;

        temp = temp->next;
    }

    // Requested page is not found in buffer pool.
    // If there isn't any empty frame to read,
    // Page enviction occurs
    if (empty == NULL){


        // Find LRU(Least recently used) page victim
        if (victim == NULL){
            printf("Error detected: Not enough buffer space!\n");
            exit(1);
        }
        
        // Flush all data if the page is dirty
        if(victim->is_dirty){
            buffer_flush_page(victim);
        }

        // Clean the content of the buffer frame
        clear(victim);
        empty = victim;
    }

    // Read page from disk and store information in the frame
    file_read_page(page_num, PAGE_ADDRESS(empty->frame), tables.fd[table_id]);

    empty->page_num = page_num;
    empty->table_id = table_id;
    empty->pin_count++;

    return empty;
}

// Write page to buffer
// This makes frame dirty and increases pin count by 1
void buffer_write_page(BufferBlock_t * frame, Page_t page){

    frame->frame = page;
    frame->is_dirty = true;
    frame->pin_count++;
    return;
}

// Flush all data of frame into coresponding disk page 
void buffer_flush_page(BufferBlock_t * frame){

    file_write_page(frame->page_num, PAGE_ADDRESS(frame->frame), tables.fd[frame->table_id]);
    return;
}

// Allocate new page from disk and stage it on a buffer frame
// This increases pin count by 1 and make header page dirty
BufferBlock_t * buffer_allocate_page(int table_id){

    BufferBlock_t * header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);

    Pagenum_t page_num = file_alloc_page(&header_frame->frame.header_page, tables.fd[table_id]);
    header_frame->is_dirty = true;
    buffer_unpin_page(header_frame, 1);

    BufferBlock_t * temp = buffer_read_page(table_id, page_num);
    
    NodePage_t node;

    node.parent_page_num = 0;
    node.num_key = 0;
    node.extra_page_num = 0;

    temp->frame.node_page = node;

    return temp;
}

// Free an allocated page from the buffer
// Flush the changes into the disk and remove frame from the buffer
// This makes header page dirty
void buffer_free_page(BufferBlock_t * frame){

    BufferBlock_t * header_frame = buffer_read_page(frame->table_id, HEADER_PAGE_NUMBER);

    file_free_page(frame->page_num, &header_frame->frame.header_page, tables.fd[frame->table_id]);
    clear(frame);

    header_frame->is_dirty = true;
    buffer_unpin_page(header_frame, 1);

    return;
}

// Decrease the pin count of frame in buffer by count
void buffer_unpin_page(BufferBlock_t * frame, int count){

    frame->pin_count -= count;
    if (frame->pin_count < 0){
        printf("Severe Error: pin count of this frame in buffer is lower than 0.\n");
        buffer_print_page(frame);
    }        
}

// Decrease the pin count of frame in buffer by 1
void buffer_unpin_page(BufferBlock_t * frame){
    buffer_unpin_page(frame, 1);
}

void buffer_check_relationship(){

    BufferBlock_t * temp = buffer, * child;
    int i;

    while(temp != NULL){
        if (temp->table_id != 0 && temp->page_num != HEADER_PAGE_NUMBER && temp->frame.node_page.is_leaf == false ){
            for (i = 0; i < temp->frame.node_page.num_key; i++){
                child = buffer_read_page(temp->table_id, INTERNAL_VAL(temp->frame.node_page, i));
                if (temp->page_num != child->frame.node_page.parent_page_num){
                   printf("Parent-Child relation mismatch occured!\n Page [%ld], which is child of Page [%ld]", 
                    child->page_num, temp->page_num);
                   printf(" is not pointing at Page [%ld] as its parent.\n It's parent page number is [%ld].\n",
                    temp->page_num, child->frame.node_page.parent_page_num); 
                }
                buffer_unpin_page(child, 1);
            }
        }
        temp = temp->next;
    }
}

// Print some information about the frame in buffer
void buffer_print_page(BufferBlock_t * frame){

    printf("<Current frame info>\n");

    if (frame->table_id != 0){
        printf("Table ID: %d\n", frame->table_id);
        printf("Page number: %ld\n", frame->page_num);

        if (frame->is_dirty) printf("Is dirty: true\n");
        else printf("Is dirty: false\n");

        printf("Pin count: %d\n", frame->pin_count);
        printf("In-frame page info:\n");
        if(frame->page_num == HEADER_PAGE_NUMBER){
            print_header_page(frame->frame.header_page);
        }
        else{
            print_node(frame->frame.node_page, frame->page_num);
        }
        printf("-----------------------------------------------\n\n");
    }
    else{
        printf("[This buffer frame is not in use]\n\n");
    }


}

// Print information of all of the frames
// Currently exist in buffer
void buffer_print_all(){

    BufferBlock_t * temp = buffer;
    
    printf("\n[Informations of frames currently on buffer]\n");
    while (temp != NULL){

        buffer_print_page(temp);
        temp = temp->next;
    }
    printf("\n");
}

void print_table_info(int table_id){

    int i;

    printf("<Current table [ID: %d] info>\n", table_id);
    printf("fd: %d\n", tables.fd[table_id]);
    if (tables.in_use[table_id]) printf("In-use: true\n");
    else printf("In-use: false\n");
    printf("File path: %s\n", tables.pathname[table_id]);
    
}

void print_all_tables(){
    int i;

    printf("\n[Informations of tables currently opened]\n");
    for (i = 1; i <= MAX_TABLE_NUMBER; i++){
        if (tables.in_use[i]) print_table_info(i);
        if (i != MAX_TABLE_NUMBER && tables.in_use[i]) printf("-----------------------------------------------\n");
    }
    printf("\n");
}