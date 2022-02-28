#include <transaction.hpp>

TransactionManager::TransactionManager(){
    next_trx_id = 1;
    current_id = 1;
}

LockManager::LockManager(){
}


Lock::Lock() : table_id(0), page_num(0), lock_mode(LockMode::SHARED) {}
Lock::Lock(int table_id, Pagenum_t page_num, LockMode lock_mode) : table_id(table_id), page_num(page_num), lock_mode(lock_mode) {}

LockManager * lock_manager;
TransactionManager * trx_manager;

void Transaction::unlock_all(){

    trx_mutex.lock();

    auto iter = acquired_locks.begin();

    while(iter != acquired_locks.end()){
        delete (*iter);
        acquired_locks.erase(iter++);
    }

    trx_mutex.unlock();

}

Transaction* TransactionManager::add_new_trx(){

    trx_manager_mutex.lock();

    Transaction *trx = new Transaction(next_trx_id++);
    trx->is_working = true;
    trx_table.push_back(trx);

    trx_manager_mutex.unlock();

    return trx;
}

bool TransactionManager::clear_trx(int trx_id){
    
    trx_manager_mutex.lock();

    for(auto iter = trx_table.begin(); iter != trx_table.end(); iter++){
        if((*iter)->trx_id == trx_id){
            Transaction * trx = (*iter);
            trx->is_working = false;
            current_id++;
            trx->trx_mutex.unlock();
            return true;
        }
    }

    trx_manager_mutex.unlock();

    return false;
}

int begin_trx(){

    Transaction * trx = trx_manager->add_new_trx();
    
    int trx_id = trx->trx_id;

    return SUCCESS;
}

int end_trx(int tid){

    if (trx_manager->clear_trx(tid)){
        return tid;
    }
    return SUCCESS;
}

int db_find(int table_id, keyval_t key, char* ret_val, int trx_id){

    while(trx_manager->current_id != trx_id){}
    return db_find(table_id, key, ret_val);
}

int db_update(int table_id, keyval_t key, char* values, int trx_id){

    while(trx_manager->current_id != trx_id){}
    if(tables.in_use[table_id] == false){
        return FAILURE;
    }

    BufferBlock_t * header_frame, * node_page_frame;
    HeaderPage_t header;

    header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);
    header = header_frame->frame.header_page;

    int i = 0, result;
    Pagenum_t page_num = find_leaf( table_id, header.root_page_num, key, false );
    NodePage_t node_page;
    Record_t new_record;

    if (page_num == KEY_DO_NOT_EXISTS){
        return FAILURE;
    }

    node_page_frame = buffer_read_page(table_id, page_num);
    node_page = PAGE_CONTENTS(node_page_frame);

    for (i = 0; i < node_page.num_key; i++)
        if (node_page.lf_record[i].key == key) break;
    if (i == node_page.num_key) {
        result = FAILURE;
    }
    else {
        strcpy(node_page.lf_record[i].value, values);
        result = SUCCESS;
    }

    buffer_unpin_page(header_frame, 1);
    buffer_unpin_page(node_page_frame, 1);

    return result;
}


// Allocate the buffer pool (array) with the given number of entries.
// Initialize other fields such as state info, LRU info
// If success, return 0. Otherwise, return non zero value.
int init_db(int num_buf){

    int i;

    // Ignore when num_buf <= 0
    if( num_buf <= 0)
        return FAILURE;

    // Initialize table list
    tables.num_table = 0;
    for (i = 1; i <= MAX_TABLE_NUMBER; ++i){
        tables.pathname[i][0] = 0;
        tables.fd[i] = 0;
        tables.in_use[i] = false;
    }

    buffer = new Buffer(num_buf);
    trx_manager = new TransactionManager();
    lock_manager = new LockManager();

    return SUCCESS;
}

// Destroy buffer manager
int shutdown_db(void){

    int i;
    for(i = 1; i <= MAX_TABLE_NUMBER; i++){
        close_table(i);
    }

    delete buffer;
    delete trx_manager;
    delete lock_manager;

    return SUCCESS;
}