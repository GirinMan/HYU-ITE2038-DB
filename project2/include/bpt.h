#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Operating on-disk B+ tree
#include "diskmanage.h"

// Default order is 32.
#define DEFAULT_ORDER 32

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
typedef struct record {
    bool is_null;
    char value[120];
} record;

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of records(key, value) and the array of corresponding
 * page numbers.  The relation between keys
 * and page numbers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * page number, with a maximum of order - 1 (key, page numer)
 * pairs.  The last page number points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first page number
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * page numbers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
typedef struct node {
    pagenum_t page_num;
    struct node * next; // Used for queue.
} node;

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


// FUNCTION PROTOTYPES.

// Output and utility.


int height( pagenum_t root_page_num );
int path_to_root( pagenum_t root_page_num, pagenum_t child_page_num );
int find_range( pagenum_t root_page_num, keyval_t key_start, keyval_t key_end, 
    bool verbose, keyval_t returned_keys[], pagenum_t returned_page_nums[]); 
pagenum_t find_leaf( pagenum_t root_page_num, keyval_t key, bool verbose );
record find( pagenum_t root_page_num, keyval_t key, bool verbose );
int cut( int length );

// Insertion.

record make_record(char * value);
pagenum_t make_node( void );
pagenum_t make_leaf( void );

int get_left_index(NodePage_t parent, pagenum_t left);
pagenum_t insert_into_leaf(pagenum_t leaf_page_num, NodePage_t leaf, keyval_t key, record pointer );
pagenum_t insert_into_leaf_after_splitting(pagenum_t root, pagenum_t leaf, keyval_t key, record pointer);
pagenum_t insert_into_node(pagenum_t root_page_num, pagenum_t parent, int left_index, keyval_t key, pagenum_t right);
pagenum_t insert_into_node_after_splitting(pagenum_t root, pagenum_t parent, int left_index, keyval_t key, pagenum_t right);
pagenum_t insert_into_parent(pagenum_t root, pagenum_t left, keyval_t key, pagenum_t right);
pagenum_t insert_into_new_root(pagenum_t left, keyval_t key, pagenum_t right);
pagenum_t start_new_tree(keyval_t key, record pointer);

// Insert input ‘key/value’ (record) to data file at the right place.
// If success, return 0. Otherwise, return non-zero value.
int db_insert(keyval_t key, char * value );

// Deletion.

int get_neighbor_index( pagenum_t n );
pagenum_t adjust_root(pagenum_t root);
pagenum_t coalesce_nodes(pagenum_t root, pagenum_t n, pagenum_t neighbor, int neighbor_index, keyval_t k_prime);
pagenum_t redistribute_nodes(pagenum_t root, pagenum_t n, pagenum_t neighbor, int neighbor_index, int k_prime_index, keyval_t k_prime);
pagenum_t delete_entry( pagenum_t root, pagenum_t n, keyval_t key, record pointer );
pagenum_t delete( pagenum_t root, keyval_t key );

void destroy_tree_nodes(pagenum_t root);
pagenum_t destroy_tree(pagenum_t root);

#endif /* __BPT_H__*/
