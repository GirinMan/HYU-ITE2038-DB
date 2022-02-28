#ifndef __LOCKMANAGE_H__
#define __LOCKMANAGE_H__

#include "bpt.hpp"

/* 
 * lock and transaction struct definitions
 */

enum class lock_mode { SHARED, EXCLUSIVE };
enum trx_state { IDLE, RUNNING, WAITING };

struct Lock{
    Lock() = default;
    Lock(int table_id, Pagenum_t page_num, int record_idx, lock_mode mode);

    // Information of table ID and page number
    int table_id;
    Pagenum_t page_num;

    // Index of the record
    int record_idx;

    // Type of current lock
    lock_mode mode;

    // backpointer to lock holder
    struct Transaction* trx;
};

struct Transaction {
    Transaction(int trx_id);

    // Unique transaction ID
    int trx_id;

    // Current state of transaction
    trx_state state;

    // list of holding locks
    list<struct Lock*> trx_locks; 

    // lock object that trx is waiting
    struct Lock* wait_lock; 

    void unlock_all();
};

class LockManager{
    public:

    LockManager() = default;

    // Global mutex which is needed before access
    mutex global_mutex;

    // Lock table in order to keep lock objects per Page
    unordered_map<Pagenum_t, list<struct Lock *>> table;
    // Needed to be changed into 
    // -> unordered_map<tuple<int, Pagenum_t>, LockPageList> table;
    // with new hash function
};

class TrxManager{
    public:
    
    // Global mutex which is needed before access
    mutex global_mutex;

    // Hash table in order to keep transactions
    unordered_map<int, Transaction*> table;

    bool add_new_trx(int trx_id);
    bool clear_trx(int trx_id);
};

extern volatile int last_trx_id;
extern LockManager * lock_manager;
extern TrxManager * trx_manager;

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

#endif /* __LOCKMANAGE_H__ */