#include <buffer.hpp>

TableInfo_t tables;
Buffer *buffer;

size_t PIDHasher::operator()(const pair<int, Pagenum_t> & pInfo) const{
    return (hash<int>()(pInfo.first) >> 1) ^ (hash<uint64_t>()(pInfo.second) << 1);
}

Buffer::Buffer(int num_buf) : pool(nullptr) {
    init(num_buf);
}

Buffer::~Buffer() {
    clear_all();
}

BufferBlock_t& Buffer::read_page(const int table_id, const Pagenum_t page_num){

    BufferBlock_t * temp = pool, * empty = nullptr, * victim = nullptr;

    if (table_id < 1 || table_id > MAX_TABLE_NUMBER){
        cerr << "Invalid table number" << endl;
        return *pool;
    }
    
    // Find requested frame from the buffer list
    auto it = lookup.find(make_pair(table_id, page_num));
    
    if (it != lookup.end()){
        BufferBlock_t * frame = (*it).second;
        frame->pin_page();
        

        return *frame;
    }
    
    while(temp != nullptr){
        
        if(temp == temp->prev || temp == temp->next){
            temp->print();
            while(1){}
        }
        // If current buffer block is not in use, store its pointer
        if(empty == nullptr && temp->table_id == 0){
            empty = temp;
        }

        // If current buffer frame is in use and its pin count is 0,
        // Mark it as victim for page enviction
        if(temp->table_id != 0 && temp->pin_count == 0){
            victim = temp;
        }

        temp = temp->next;
    }

    // Requested page is not found in buffer pool.
    // If there isn't any empty frame to read,
    // Page enviction occurs
    if (empty == nullptr){

        // Find LRU(Least recently used) page victim
        if (victim == nullptr){
            cerr << "Error detected: Not enough buffer space!" << endl;
            exit(1);
        }
        
        // Flush all data if the page is dirty
        if(victim->is_dirty){
            victim->flush();
        }

        // Clean the content of the buffer frame
        victim->clear();
        empty = victim;
    }

    // Read page from disk and store information in the frame
    file_read_page(page_num, PAGE_ADDRESS(empty->frame), tables.fd[table_id]);

    empty->page_num = page_num;
    empty->table_id = table_id;
    empty->pin_page();


    add_lookup(table_id, page_num, empty);
    
    return *empty;
}

BufferBlock_t& Buffer::write_page(BufferBlock_t &frame, const Page_t &page){
    frame.frame = page;
    frame.is_dirty = true;
    frame.pin_page();
    


    
    return frame;
}

BufferBlock_t& Buffer::allocate_page(const int table_id){

    BufferBlock_t& header_frame = Buffer::read_page(table_id, HEADER_PAGE_NUMBER);
    
    Pagenum_t page_num = file_alloc_page(&header_frame.frame.header_page, tables.fd[table_id]);
    header_frame.is_dirty = true;
    header_frame.unpin_page(1);
    
    BufferBlock_t& temp = Buffer::read_page(table_id, page_num);
    
    NodePage_t node;

    node.parent_page_num = 0;
    node.num_key = 0;
    node.extra_page_num = 0;

    temp.frame.node_page = node;
    
    return temp;
}

void Buffer::free_page(BufferBlock_t& frame){
    
    BufferBlock_t& header_frame = Buffer::read_page(frame.table_id, HEADER_PAGE_NUMBER);

    file_free_page(frame.page_num, &header_frame.frame.header_page, tables.fd[frame.table_id]);
    remove_lookup(frame.table_id, frame.page_num);
    frame.clear();

    header_frame.is_dirty = true;
    header_frame.unpin_page(1);

    return;
}

int Buffer::init(int num_buf){

    BufferBlock_t * temp = nullptr;

    if(pool != nullptr){
        cerr << "Previously activated DB is not closed yet!" << endl;
        return FAILURE;
    }

    // Allocate buffer num_buf times and attach new buffer in front of the first buffer
    for (int i = 0; i < num_buf; ++i){
        
        temp = new BufferBlock_t();
        if(temp == nullptr){
            return FAILURE;
        }

        if( pool == nullptr){
            pool = temp;
        }
        else{
            pool->prev = temp;
            temp->next = pool;
            pool = temp;
        }  
    }

    lookup.clear();

    return SUCCESS;
}

void Buffer::add_lookup(const int table_id, const Pagenum_t page_num, BufferBlock_t * frame){
    pair<int, Pagenum_t> PID = make_pair(table_id, page_num);

    lookup.insert(make_pair(PID, frame));
}

void Buffer::remove_lookup(const int table_id, const Pagenum_t page_num){
    pair<int, Pagenum_t> PID = make_pair(table_id, page_num);

    if(lookup.find(PID) != lookup.end()){
        lookup.erase(PID);
    }
}

void Buffer::clear_pages(int table_id){

    BufferBlock_t * temp = pool;

    if(table_id < 1 || table_id > MAX_TABLE_NUMBER){
        cerr << "Invalid table ID is required In Buffer::clear_pages" << endl;
        return;
    }

    while(temp){
        if(temp->table_id == table_id){

            remove_lookup(table_id, temp->page_num);

            temp->flush();
            temp->clear();
        }
        temp = temp->next;
    }
}

void Buffer::clear_all(){

    BufferBlock_t * temp = pool, * temp2;

    while(temp){
        if(temp->table_id > 0 && temp->table_id <= MAX_TABLE_NUMBER){
            temp->flush();
        }
        temp2 = temp;
        temp = temp->next;
        delete temp2;
    }

    lookup.clear();
}

// Print information of all of the frames
// Currently exist in buffer

void Buffer::print_all(){

    BufferBlock_t * temp = pool;
    
    print_lookup();
    printf("\n[Informations of frames currently on buffer]\n");
    cout << "Buffer pool address: " << pool << endl;
    while (temp != NULL){

        temp->print();
        temp = temp->next;
    }
    printf("\n");
}

void Buffer::print_lookup(){
    cout << "[Buffer hash table status]\n-----------------------------------" << endl;
    for(auto it = lookup.begin(); it != lookup.end(); it++){
        cout << "Table ID: [" <<(*it).first.first << "] Page number: [" << (*it).first.second << "]" << endl;
        cout << "\nBuffer block information>" << endl;
        (*it).second->print();
        cout << endl;
    }
}

BufferBlock_t::BufferBlock_t() : table_id(0), page_num(0), is_dirty(false), pin_count(0) {
    prev = next = nullptr;
}

BufferBlock_t::BufferBlock_t(Page_t page, int tid, Pagenum_t pid, bool dirty)
    : table_id(tid), page_num(pid), is_dirty(dirty), pin_count(0) {
    frame = page;
    prev = next = nullptr;
}

void BufferBlock_t::pin_page(){
    pin_count++;
}

void BufferBlock_t::unpin_page(int count){
    pin_count -= count;
}

void BufferBlock_t::insert_between(BufferBlock_t * prev, BufferBlock_t * next){

    BufferBlock_t * this_prev, * this_next;

    if((prev && prev->next != next) || (next && next->prev != prev)){ // prev and next is not connected
        cerr << "prev and next is not connected" << endl;
        return;
    }

    this_prev = this->prev;
    this_next = this->next;

    if (this_prev){
        this_prev->next = this_next;
    }

    if (this_next){
        this_next->prev = this_prev;
    }

    this->prev = prev;
    this->next = next;

    if(prev){
        prev->next = this;
    }

    if(next){
        next->prev = this;
    }

}

void BufferBlock_t::clear(){
    table_id = 0;
    page_num = 0;
    is_dirty = false;
    pin_count = 0;
}

void BufferBlock_t::flush(){
    file_write_page(page_num, PAGE_ADDRESS(frame), tables.fd[table_id]);
}

// Print some information about the frame in buffer
void BufferBlock_t::print(){

    printf("<Current frame info>\n");
    cout << "Address: " << this << endl;
    cout << "Previous: " << this->prev << " / Next: " << this->next << endl;
    if (this->table_id != 0){
        printf("Table ID: %d\n", this->table_id);
        printf("Page number: %ld\n", this->page_num);

        if (this->is_dirty) printf("Is dirty: true\n");
        else printf("Is dirty: false\n");

        printf("Pin count: %d\n", this->pin_count);
        printf("In-this page info:\n");
        if(this->page_num == HEADER_PAGE_NUMBER){
            print_header_page(this->frame.header_page);
        }
        else{
            print_node(this->frame.node_page, this->page_num);
        }
        printf("-----------------------------------------------\n\n");
    }
    else{
        printf("[This buffer frame is not in use]\n\n");
    }


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

    if (table_id < 1 || table_id > MAX_TABLE_NUMBER || tables.in_use[table_id] == false){
        return FAILURE;
    }

    buffer->clear_pages(table_id);

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
    return &buffer->read_page(table_id, page_num);
}

// Write page to buffer
// This makes frame dirty and increases pin count by 1
void buffer_write_page(BufferBlock_t * frame, Page_t page){
    buffer->write_page(*frame, page);
}

// Flush all data of frame into coresponding disk page 
void buffer_flush_page(BufferBlock_t * frame){
    frame->flush();
}

// Allocate new page from disk and stage it on a buffer frame
// This increases pin count by 1 and make header page dirty
BufferBlock_t * buffer_allocate_page(int table_id){
    return &buffer->allocate_page(table_id);
}

// Free an allocated page from the buffer
// Flush the changes into the disk and remove frame from the buffer
// This makes header page dirty
void buffer_free_page(BufferBlock_t * frame){
    buffer->free_page(*frame);
}

// Decrease the pin count of frame in buffer by count
void buffer_unpin_page(BufferBlock_t * frame, int count){
    frame->unpin_page(count);        
}

// Decrease the pin count of frame in buffer by 1
void buffer_unpin_page(BufferBlock_t * frame){
    frame->unpin_page(1);
}

void buffer_print_page(BufferBlock_t* frame){
    frame->print();
}

void buffer_print_all(){
    buffer->print_all();
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