#ifndef __TRANSACTION_H__
#define __TRANSACTION_H__

#include <bpt.hpp>


struct UndoLog{

    UndoLog(int, keyval_t, string);

    int table_id;
    keyval_t key;
    string old_value;
};


enum class TransactionState { 
    IDLE, RUNNING, WAITING 
};

enum class LockMode { 
    SHARED, EXCLUSIVE
};

struct Lock;

struct Transaction {

    int trx_id;
    bool is_working;

    TransactionState trx_state;

    list<Lock*> acquired_locks;

    mutex trx_mutex;
    condition_variable trx_cond;

    Lock* wait_lock;

    list<UndoLog> undo_log_list;

    Transaction(int trx_id){
        trx_id = trx_id;
        is_working = false;
    }

    void unlock_all();

};

class TransactionManager{

private:

    deque<Transaction*> trx_table;

    int next_trx_id;
    bool is_working;

    mutex trx_manager_mutex;

public:

    TransactionManager();
    ~TransactionManager();

    Transaction* add_new_trx();
    bool clear_trx(int trx_id);

    int current_id;
    mutex work_mutex;
};

struct Lock{

    Lock();
    Lock(int, Pagenum_t, enum LockMode);
    
    int table_id;
    Transaction* trx;

    Pagenum_t page_num;
    keyval_t key;

    bool acquired;
    LockMode lock_mode;

    Lock * prev;
    Lock * next;
};

class LockManager{

private:

    unordered_map<pair<int, Pagenum_t>, pair<Lock*, Lock*>, PIDHasher> lock_table;

    mutex lock_manager_mutex;

public:

    LockManager();
};

extern LockManager * lock_manager;
extern TransactionManager * trx_manager;

/* Allocate transaction structure and initialize it.
 * Return the unique transaction id if success, otherwise return 0.
 * Note that transaction id should be unique for each transaction, that is you need to
 * allocate transaction id using mutex or atomic instruction, such as
 * sync_fetch_and_add().
 */
int begin_trx();

/* Clean up the transaction with given tid (transaction id) and its related information
 * that has been used in your lock manager. (Shrinking phase of strict 2PL)
 * Return the completed transaction id if success, otherwise return 0.
 */
int end_trx(int tid);

/* Read values in the table with matching key for this transaction which has its id trx_id.
 * return 0 (SUCCESS): operation is successfully done and the transaction can
 * continue the next operation.
 * return non-zero (FAILED): operation is failed (e.g., deadlock detected) and the
 * transaction should be aborted. Note that all tasks that need to be arranged (e.g.,
 * releasing the locks that are held on this transaction, rollback of previous
 * operations, etc… ) should be completed in db_find().
 */
int db_find(int table_id, keyval_t key, char* ret_val, int trx_id);

/* Find the matching key and modify the values, where each value (column) never
 * exceeds the existing one.
 * return 0 (SUCCESS): operation is successfully done and the transaction can
 * continue the next operation.
 * return non-zero (FAILED): operation is failed (e.g., deadlock detected) and the
 * transaction should be aborted. Note that all tasks that need to be arranged (e.g.,
 * releasing the locks that are held on this transaction, rollback of previous
 * operations, etc… ) should be completed in db_update().
 */
int db_update(int table_id, keyval_t keyj, char* values, int trx_id);


#endif /* __TRANSACTION_H__ */

