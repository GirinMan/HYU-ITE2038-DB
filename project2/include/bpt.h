#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Operating on-disk B+ tree
#include "diskmanage.h"

// Because the size of a record of leaf and an entry of internal node is different,
// Two different orders have to be defined.
#define LEAF_ORDER 32
#define INTERNAL_ORDER 249

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 1024

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

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
extern int order;

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

// FUNCTION PROTOTYPES.

// Output and utility.
void enqueue( Pagenum_t new_node );
Pagenum_t dequeue( void );
void print_tree(Pagenum_t root_page_num);
void print_leaves(Pagenum_t root_page_num);

int height( Pagenum_t root_page_num );
int path_to_root( Pagenum_t root_page_num, Pagenum_t child_page_num );
int find_range( Pagenum_t root_page_num, keyval_t key_start, keyval_t key_end, 
                bool verbose, keyval_t returned_keys[], Pagenum_t returned_page_nums[]); 
Pagenum_t find_leaf( Pagenum_t root_page_num, keyval_t key, bool verbose );
Record_t find( Pagenum_t root_page_num, keyval_t key, bool verbose );
int cut( int length );

// Insertion.

Record_t make_record(char * value);
Pagenum_t make_node( void );
Pagenum_t make_leaf( void );

int get_left_index(NodePage_t parent, Pagenum_t left);
Pagenum_t insert_into_leaf(Pagenum_t leaf_page_num, NodePage_t leaf, keyval_t key, Record_t pointer );
Pagenum_t insert_into_leaf_after_splitting(Pagenum_t root, Pagenum_t leaf, keyval_t key, Record_t pointer);
Pagenum_t insert_into_node(Pagenum_t root_page_num, Pagenum_t parent, int left_index, keyval_t key, Pagenum_t right);
Pagenum_t insert_into_node_after_splitting(Pagenum_t root, Pagenum_t parent, int left_index, keyval_t key, Pagenum_t right);
Pagenum_t insert_into_parent(Pagenum_t root, Pagenum_t left, keyval_t key, Pagenum_t right);
Pagenum_t insert_into_new_root(Pagenum_t left, keyval_t key, Pagenum_t right);
Pagenum_t start_new_tree(keyval_t key, Record_t pointer);

// Insert input ‘key/value’ (record) to data file at the right place.
// If success, return 0. Otherwise, return non-zero value.
int db_insert(keyval_t key, char * value );

// Deletion.

int get_neighbor_index( Pagenum_t n );
Pagenum_t adjust_root(Pagenum_t root);
Pagenum_t coalesce_nodes(Pagenum_t root, Pagenum_t n, Pagenum_t neighbor, int neighbor_index, keyval_t k_prime);
Pagenum_t redistribute_nodes(Pagenum_t root, Pagenum_t n, Pagenum_t neighbor, int neighbor_index, int k_prime_index, keyval_t k_prime);
Pagenum_t delete_entry( Pagenum_t root, Pagenum_t n, keyval_t key);
Pagenum_t delete( Pagenum_t root, keyval_t key );

Pagenum_t remove_entry_from_node(Pagenum_t node_page_num, keyval_t key);

void destroy_tree_nodes(Pagenum_t root);
Pagenum_t destroy_tree(Pagenum_t root);

#endif /* __BPT_H__*/
