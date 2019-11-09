#ifndef __BPT_H__
#define __BPT_H__

#include "buffer.hpp"

#define LINE(x) printf("line #%d: ", x);

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct Record_t {
    bool is_null;
    char value[120];
} Record_t;

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int lf_order;
extern int in_order;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
extern bool verbose_output;

typedef struct QNode{
    Pagenum_t page_num;
    struct QNode * next;
}QNode;

extern struct QNode * queue;

// Insert input ‘key/value’ (record) to data file at the right place.
// If success, return 0. Otherwise, return non-zero value.
int db_insert (int table_id, keyval_t key, char * value);

// Find the record containing input ‘key’.
// If found matching ‘key’, store matched ‘value’ string in ret_val and return 0. Otherwise, return non-zero value.
// Memory allocation for record structure(ret_val) should occur in caller function.
int db_find (int table_id, keyval_t key, char * ret_val);

// Find the matching record and delete it if found.
// If success, return 0. Otherwise, return non-zero value.
int db_delete (int table_id, keyval_t key);


// Output and utility.
void enqueue( Pagenum_t new_node );
Pagenum_t dequeue( void );

void print_tree(int table_id);
void print_leaves(int table_id);

int height(int table_id,  Pagenum_t root_page_num );
int path_to_root(int table_id,  Pagenum_t root_page_num, Pagenum_t child_page_num );
int cut( int length );
Pagenum_t find_leaf(int table_id,  Pagenum_t root_page_num, keyval_t key, bool verbose );
Record_t find(int table_id,  Pagenum_t root_page_num, keyval_t key, bool verbose );

// Insertion.

Record_t make_record(char * value);
Pagenum_t make_node( int table_id, bool is_leaf );

int get_left_index(NodePage_t parent, Pagenum_t left);
Pagenum_t insert_into_leaf(int table_id, Pagenum_t leaf_page_num, keyval_t key, Record_t pointer );
Pagenum_t insert_into_leaf_after_splitting(int table_id, Pagenum_t root, Pagenum_t leaf, keyval_t key, Record_t pointer);
Pagenum_t insert_into_node(int table_id, Pagenum_t root_page_num, Pagenum_t parent, int left_index, keyval_t key, Pagenum_t right);
Pagenum_t insert_into_node_after_splitting(int table_id, Pagenum_t root, Pagenum_t parent, int left_index, keyval_t key, Pagenum_t right);
Pagenum_t insert_into_parent(int table_id, Pagenum_t root, Pagenum_t left, keyval_t key, Pagenum_t right);
Pagenum_t insert_into_new_root(int table_id, Pagenum_t left, keyval_t key, Pagenum_t right);
Pagenum_t start_new_tree(int table_id, keyval_t key, Record_t pointer);

// Deletion.

int get_neighbor_index(int table_id,  Pagenum_t n );
Pagenum_t adjust_root(int table_id, Pagenum_t root);
Pagenum_t coalesce_nodes(int table_id, Pagenum_t root, Pagenum_t n, Pagenum_t neighbor, int neighbor_index, keyval_t k_prime);
Pagenum_t redistribute_nodes(int table_id, Pagenum_t root, Pagenum_t n, Pagenum_t neighbor, int neighbor_index, int k_prime_index, keyval_t k_prime);
Pagenum_t delete_entry(int table_id,  Pagenum_t root, Pagenum_t n, keyval_t key);
Pagenum_t remove_entry_from_node(int table_id, Pagenum_t node_page_num, keyval_t key);

void destroy_tree_nodes(int table_id, Pagenum_t root);
Pagenum_t destroy_tree(int table_id, Pagenum_t root);

#endif /* __BPT_H__*/
