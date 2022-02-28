#include "lockmanage.hpp"

volatile int last_trx_id;
LockManager * lock_manager;
TrxManager * trx_manager;

Lock::Lock(int id, Pagenum_t num, int idx, lock_mode mod)
    : table_id(id), page_num(num), record_idx(idx), mode(mod), trx(nullptr){}

Transaction::Transaction(int id) 
    : trx_id(id), state(IDLE), trx_locks(), wait_lock(nullptr){}

void Transaction::unlock_all(){

    auto iter = trx_locks.begin();

    while(iter != trx_locks.end()){
        delete (*iter);
        trx_locks.erase(iter++);
    }

}

bool TrxManager::add_new_trx(int trx_id){
    Transaction *trx = new Transaction(trx_id);
    bool success = table.insert({trx_id, trx}).second;

    if (!success) delete trx;

    return success;
}

bool TrxManager::clear_trx(int trx_id){
    
    auto iter = table.find(trx_id);
    if (iter != table.end()){

        Transaction * trx = (*iter).second;
        trx->unlock_all();
        delete trx;

        table.erase(iter);
        return true;
    }
    return false;
}

/* Allocate transaction structure and initialize it.
 * Return the unique transaction id if success, otherwise return 0.
 * Note that transaction id should be unique for each transaction, that is you need to
 * allocate transaction id using mutex or atomic instruction, such as
 * sync_fetch_and_add().
 */
int begin_trx(){

    __sync_fetch_and_add(&last_trx_id, 1);
    
    int trx_id = last_trx_id;
    
    if (trx_manager->add_new_trx(trx_id)){
        return trx_id;
    }
    return 0;
}

/* Clean up the transaction with given tid (transaction id) and its related information
 * that has been used in your lock manager. (Shrinking phase of strict 2PL)
 * Return the completed transaction id if success, otherwise return 0.
 */
int end_trx(int tid){

    if (trx_manager->clear_trx(tid)){
        return tid;
    }
    return 0;
}

/* Read values in the table with matching key for this transaction which has its id trx_id.
 * return 0 (SUCCESS): operation is successfully done and the transaction can
 * continue the next operation.
 * return non-zero (FAILED): operation is failed (e.g., deadlock detected) and the
 * transaction should be aborted. Note that all tasks that need to be arranged (e.g.,
 * releasing the locks that are held on this transaction, rollback of previous
 * operations, etc… ) should be completed in db_find().
 */
int db_find(int table_id, keyval_t key, char* ret_val, int trx_id){
    return db_find(table_id, key, ret_val);
}

/* Find the matching key and modify the values, where each value (column) never
 * exceeds the existing one.
 * return 0 (SUCCESS): operation is successfully done and the transaction can
 * continue the next operation.
 * return non-zero (FAILED): operation is failed (e.g., deadlock detected) and the
 * transaction should be aborted. Note that all tasks that need to be arranged (e.g.,
 * releasing the locks that are held on this transaction, rollback of previous
 * operations, etc… ) should be completed in db_update().
 */
int db_update(int table_id, keyval_t keyj, char* values, int trx_id){
    return FAILURE;
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

    last_trx_id = 0;
    trx_manager = new TrxManager();
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