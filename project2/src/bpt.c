/*
 *  bpt.c  
 */
#define Version "1.14"
/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 
 *  3. Neither the name of the copyright holder nor the names of its 
 *  contributors may be used to endorse or promote products derived from this 
 *  software without specific prior written permission.
 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 
 *  Author:  Amittai Aviram 
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *  
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 *  Usage:  bpt [order]
 *  where order is an optional argument
 *  (integer MIN_ORDER <= order <= MAX_ORDER)
 *  defined as the maximal number of pointers in any node.
 *
 */

#include "bpt.h"

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (`eys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
int order = DEFAULT_ORDER;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = true;

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES

/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int height( pagenum_t root_page_num ) {
    int h = 0;

    // Create temporary in-memory node structure c
    NodePage_t c;
    file_read_page(root_page_num, PAGE_ADDRESS(c));

    // Read the first child of node c until the node is leaf node
    while (!c.is_leaf) {
        file_read_page(c.extra_page_num, PAGE_ADDRESS(c));
        h++;
    }
    return h;
}

// NEED TO BE CHANGED
/* Utility function to give the length in edges
 * of the path from any node to the root.
 */
int path_to_root( pagenum_t root_page_num, pagenum_t child_page_num ) {
    int length = 0;

    // Create in-memory child node structure
    NodePage_t c;
    file_read_page(child_page_num, PAGE_ADDRESS(c));

    // Create page number variable to store parent's page number
    pagenum_t c_page_num = child_page_num;

    // Read parent page of the node page until the page becomes root page
    while (c_page_num != root_page_num) {
        c_page_num = c.parent_page_num;
        file_read_page(c.parent_page_num, PAGE_ADDRESS(c));
        length++;
    }
    return length;
}

// NEED TO BE CHANGED
/* Finds keys and page numbers of leaf node, if present, in the range specified
 * by key_start and key_end, inclusive.  Places these in the arrays
 * returned_keys and returned_page_numbers, and returns the number of
 * entries found.
 */
int find_range( pagenum_t root_page_num, keyval_t key_start, keyval_t key_end, 
    bool verbose, keyval_t returned_keys[], pagenum_t returned_page_nums[]){

    int i, num_found, num_leaf;
    num_found = 0;
    num_leaf = 0;

    // find out the leaf page that contains key_start
    pagenum_t page_num = find_leaf( root_page_num, key_start, verbose );

    NodePage_t node_page;
    file_read_page(page_num, PAGE_ADDRESS(node_page));

    if (page_num == KEY_DO_NOT_EXISTS) return 0;
    for (i = 0; i < node_page.num_key && node_page.lf_record[i].key < key_start; i++) ;
    if (i == node_page.num_key) return 0;

    while (page_num != RIGHTMOST_LEAF) {
        file_read_page(page_num, PAGE_ADDRESS(node_page));
        returned_page_nums[num_leaf++] = page_num;

        for ( ; i < node_page.num_key && node_page.lf_record[i].key <= key_end; i++) {
            returned_keys[num_found] = node_page.lf_record[i].key;
            num_found++;
        }        

        // Change value into right sibling node page
        page_num = node_page.right_page_num;
        i = 0;
    }
    return num_found;
}

// NEED TO BE CHANGED
/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
pagenum_t find_leaf( pagenum_t root_page_num, keyval_t key, bool verbose ) {
    int i = 0;
    NodePage_t node_page;
    pagenum_t page_num = root_page_num;
    
    if (root_page_num == NO_ROOT_NODE) {
        if (verbose) 
            printf("Empty tree.\n");
        return KEY_DO_NOT_EXISTS;
    }
    
    file_read_page(page_num, PAGE_ADDRESS(node_page)); 
    while (!node_page.is_leaf) {

        if (verbose) {
            printf("[");
            for (i = 0; i < node_page.num_key - 1; i++)
                printf("%ld ", node_page.in_record[i].key);
            printf("%ld] ", node_page.in_record[i].key);
        }
        i = 0;
        while (i < node_page.num_key) {
            if (key >= node_page.in_record[i].key){
                i++;
            }
            else break;
        }
        
        if (verbose)
            printf("%d ->\n", i);


        if(!i){
            page_num = node_page.extra_page_num;
        }
        else{
            page_num = node_page.in_record[i - 1].page_num;
        }

        file_read_page(page_num, PAGE_ADDRESS(node_page));               
    }
    if (verbose) {
        printf("Leaf [");
        for (i = 0; i < node_page.num_key - 1; i++)
            printf("%ld ", node_page.lf_record[i].key);
        printf("%ld] ->\n", node_page.lf_record[i].key);
    }
    return page_num;
}

// NEED TO BE CHANGED
/* Finds and returns the record to which
 * a key refers.
 */
record find( pagenum_t root_page_num, keyval_t key, bool verbose ) {
    int i = 0;
    pagenum_t page_num = find_leaf( root_page_num, key, verbose );
    NodePage_t node_page;
    record new_record;

    if (page_num == KEY_DO_NOT_EXISTS){
        new_record.is_null = true;
        return new_record;
    }

    file_read_page(page_num, PAGE_ADDRESS(node_page));
    for (i = 0; i < node_page.num_key; i++)
        if (node_page.lf_record[i].key == key) break;
    if (i == node_page.num_key) {
        new_record.is_null = true;
        return new_record;
    }
    else{
        new_record.is_null = false;
        strcpy(new_record.value, node_page.lf_record[i].value);
        return new_record;
    }
}


/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


// INSERTION

/* Creates a new record to hold the value
 * to which a key refers.
 */
record make_record(char * value) {
    record new_record;
    strcpy(new_record.value, value);
    return new_record;
}


/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
pagenum_t make_node( void ) {
    NodePage_t new_node;
    pagenum_t new_page_num = file_alloc_page();

    new_node.is_leaf = false;
    new_node.num_key = 0;
    new_node.parent_page_num = NO_PARENT;

    file_write_page(new_page_num, PAGE_ADDRESS(new_node));
    return new_page_num;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
pagenum_t make_leaf( void ) {
    NodePage_t new_node;
    pagenum_t new_page_num = file_alloc_page();

    new_node.is_leaf = true;
    new_node.num_key = 0;
    new_node.parent_page_num = NO_PARENT;

    file_write_page(new_page_num, PAGE_ADDRESS(new_node));
    return new_page_num;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */
int get_left_index(NodePage_t parent, pagenum_t left) {

    int left_index = 0;
    if (parent.extra_page_num == left) return 0;
    while (left_index < parent.num_key && parent.in_record[left_index].page_num != left)
        left_index++;
    return left_index + 1;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
pagenum_t insert_into_leaf( pagenum_t leaf_page_num, NodePage_t leaf, keyval_t key, record pointer ) {

    int i, insertion_point;
    insertion_point = 0;
    while (insertion_point < leaf.num_key && leaf.lf_record[insertion_point].key < key)
        insertion_point++;

    for (i = leaf.num_key; i > insertion_point; i--) {
        leaf.lf_record[i].key = leaf.lf_record[i - 1].key;
        strcpy(leaf.lf_record[i].value, leaf.lf_record[i - 1].value);
    }
    leaf.lf_record[insertion_point].key = key;
    strcpy(leaf.lf_record[insertion_point].value, pointer.value);
    leaf.num_key++;

    file_write_page(leaf_page_num, PAGE_ADDRESS(leaf));
    return leaf_page_num;
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
pagenum_t insert_into_leaf_after_splitting(pagenum_t root_page_num, pagenum_t leaf_page_num, keyval_t key, record pointer) {

    pagenum_t new_leaf_page_num;
    NodePage_t leaf, new_leaf;
    keyval_t * temp_keys, new_key;
    record * temp_records;
    int insertion_index, split, i, j;

    new_leaf_page_num = make_leaf();
    file_read_page(leaf_page_num, PAGE_ADDRESS(leaf));
    file_read_page(new_leaf_page_num, PAGE_ADDRESS(new_leaf));



    temp_keys = (keyval_t *)malloc( order * sizeof(keyval_t) );
    if (temp_keys == NULL) {
        perror("Temporary keys array.");
        exit(EXIT_FAILURE);
    }

    temp_records = (record *)malloc( order * sizeof(record) );
    if (temp_records == NULL) {
        perror("Temporary records array.");
        exit(EXIT_FAILURE);
    }

    insertion_index = 0;
    while (insertion_index < order - 1 && leaf.lf_record[insertion_index].key < key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf.num_key; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = leaf.lf_record[i].key;
        strcpy(temp_records[j].value, leaf.lf_record[i].value);
    }

    temp_keys[insertion_index] = key;
    strcpy(temp_records[insertion_index].value, pointer.value);

    leaf.num_key = 0;


    split = cut(order - 1);

    for (i = 0; i < split; i++) {
        leaf.lf_record[i].key = temp_keys[i];
        strcpy(leaf.lf_record[i].value, temp_records[i].value);
        leaf.num_key++;
    }

    for (i = split, j = 0; i < order; i++, j++) {
        new_leaf.lf_record[j].key = temp_keys[i];
        strcpy(new_leaf.lf_record[j].value, temp_records[i].value);
        new_leaf.num_key++;
    }



    free(temp_records);
    free(temp_keys);

    // right sibling node connection
    new_leaf.right_page_num = leaf.right_page_num;
    leaf.right_page_num = new_leaf_page_num;

    /*
    for (i = leaf->num_key; i < order - 1; i++)
        leaf->pointers[i] = NULL;
    for (i = new_leaf->num_key; i < order - 1; i++)
        new_leaf->pointers[i] = NULL;
    */

    new_leaf.parent_page_num = leaf.parent_page_num;


    file_write_page(new_leaf_page_num, PAGE_ADDRESS(new_leaf));
    file_write_page(leaf_page_num, PAGE_ADDRESS(leaf));

    new_key = new_leaf.lf_record[0].key;
    //printf("insert_into_parent(%ld, %ld, %ld, %ld) called.\n", root_page_num, leaf_page_num, new_key, new_leaf_page_num);
    return insert_into_parent(root_page_num, leaf_page_num, new_key, new_leaf_page_num);
}


/* Inserts a new key and record to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
pagenum_t insert_into_node(pagenum_t root_page_num, pagenum_t parent_page_num,
    int left_index, keyval_t key, pagenum_t right_page_num){

    int i;
    NodePage_t parent, right;
    file_read_page(parent_page_num, PAGE_ADDRESS(parent));


    for (i = parent.num_key; i > left_index; i--) {
        parent.in_record[i].key = parent.in_record[i - 1].key;
        parent.in_record[i].page_num = parent.in_record[i - 1].page_num;
    }
    parent.in_record[left_index].key = key;   
    parent.in_record[left_index].page_num = right_page_num;
    
    parent.num_key++;

    //printf("[Inside insert_into_node()] parent node info: ");
    //print_node(parent, parent_page_num);

    file_write_page(parent_page_num, PAGE_ADDRESS(parent));
    return root_page_num;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
pagenum_t insert_into_node_after_splitting(pagenum_t root_page_num,
pagenum_t old_node_page_num, int left_index, keyval_t key, pagenum_t right_page_num) {

    int i, j, split;
    pagenum_t new_node_page_num, child_page_num;
    NodePage_t old_node, right, new_node, child;
    keyval_t * temp_keys, k_prime;
    pagenum_t * temp_records;

    file_read_page(old_node_page_num, PAGE_ADDRESS(old_node));
    file_read_page(right_page_num, PAGE_ADDRESS(right));
    /*printf("old node page info: ");
    print_node(old_node, old_node_page_num);
    printf("right page info: ");
    print_node(right, right_page_num);*/
	
    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    temp_records = (pagenum_t *)malloc( (order + 1) * sizeof(pagenum_t) );
    if (temp_records == NULL) {
        perror("Temporary pointers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    temp_keys = (keyval_t *)malloc( order * sizeof(keyval_t) );
    if (temp_keys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_node.num_key + 1; i++, j++) {
        if (j == left_index + 1) j++;
        if(!i) temp_records[j] = old_node.extra_page_num;
        else temp_records[j] = old_node.in_record[i - 1].page_num;
    }

    for (i = 0, j = 0; i < old_node.num_key; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_node.in_record[i].key;
    }

    temp_records[left_index + 1] = right_page_num;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  

    split = cut(order);
    new_node_page_num = make_node();
    file_read_page(new_node_page_num, PAGE_ADDRESS(new_node));


    old_node.num_key = 0;



    for (i = 0; i < split - 1; i++) {
        if(!i) old_node.extra_page_num = temp_records[i];
        else old_node.in_record[i-1].page_num = temp_records[i];
        old_node.in_record[i].key = temp_keys[i];
        old_node.num_key++;
    }

    old_node.in_record[i-1].page_num = temp_records[i];
    file_write_page(old_node_page_num, PAGE_ADDRESS(old_node));

    k_prime = temp_keys[split - 1];

    for (++i, j = 0; i < order; i++, j++) {
        if(!j) new_node.extra_page_num = temp_records[i];
        else new_node.in_record[j-1].page_num = temp_records[i];
        new_node.in_record[j].key = temp_keys[i];
        new_node.num_key++;
    }
    new_node.in_record[j-1].page_num = temp_records[i];

    free(temp_records);
    free(temp_keys);

    new_node.parent_page_num = old_node.parent_page_num;
    file_write_page(new_node_page_num, PAGE_ADDRESS(new_node));
    //printf("new node page info: ");
    //print_node_page(new_node_page_num);
 
    pagenum_t temp;
    for (i = 0; i <= new_node.num_key; i++) {
        if(!i) temp = new_node.extra_page_num;
        else temp = new_node.in_record[i-1].page_num;
        file_read_page(temp, PAGE_ADDRESS(child));

        child.parent_page_num = new_node_page_num;

        file_write_page(temp, PAGE_ADDRESS(child));
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */
    //printf("call insert_into_parent(%ld, %ld, %ld, %ld)\n", root_page_num, old_node_page_num, k_prime, new_node_page_num);
    return insert_into_parent(root_page_num, old_node_page_num, k_prime, new_node_page_num);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
pagenum_t insert_into_parent(pagenum_t root_page_num,
    pagenum_t left_page_num, keyval_t key, pagenum_t right_page_num) {

    int left_index;
    NodePage_t left, parent;
    pagenum_t parent_page_num, temp;

    file_read_page(left_page_num, PAGE_ADDRESS(left));
    parent_page_num = left.parent_page_num;

    /* Case: new root. */

    if (parent_page_num == NO_PARENT){
        //printf("new root have to be created. insert_into_new_root(%ld, %ld, %ld) called.\n", left_page_num, key, right_page_num);
        temp = insert_into_new_root(left_page_num, key, right_page_num);
        left.parent_page_num = temp;
        file_write_page(left_page_num, PAGE_ADDRESS(left));

        //printf("new root information: ");
        //print_node_page(temp);
        return temp;
    }

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    file_read_page(parent_page_num, PAGE_ADDRESS(parent));
    //print_node(parent, parent_page_num);

    /* Find the parent's pointer to the left 
     * node.
     */

    left_index = get_left_index(parent, left_page_num);
    //printf("left index: %d\n", left_index);


    /* Simple case: the new key fits into the node. 
     */

    if (parent.num_key < order - 1){
        //printf("insert_into_node(%ld, %ld, %d, %ld, %ld) called.\n", root_page_num, parent_page_num, left_index, key, right_page_num);
        return insert_into_node(root_page_num, parent_page_num, left_index, key, right_page_num);
    }

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */
    //printf("insert_into_node_after_splitting() called.\n");
    return insert_into_node_after_splitting(root_page_num, parent_page_num, left_index, key, right_page_num);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
pagenum_t insert_into_new_root(pagenum_t left_page_num, keyval_t key, pagenum_t right_page_num) {

    pagenum_t root_page_num = file_alloc_page();
    NodePage_t root, right;
 
    root.parent_page_num = NO_PARENT;
    root.is_leaf = false;
    root.num_key = 1;
    root.extra_page_num = left_page_num;
    root.in_record[0].key = key;
    root.in_record[0].page_num = right_page_num;

    file_read_page(right_page_num, PAGE_ADDRESS(right));
    right.parent_page_num = root_page_num;

    file_write_page(root_page_num, PAGE_ADDRESS(root));
    file_write_page(right_page_num, PAGE_ADDRESS(right));
    return root_page_num;
}



/* First insertion:
 * start a new tree.
 */
pagenum_t start_new_tree(keyval_t key, record pointer) {

    pagenum_t root_page_num = make_leaf();
    NodePage_t root;
    file_read_page(root_page_num, PAGE_ADDRESS(root));

    root.lf_record[0].key = key;
    strcpy(root.lf_record[0].value, pointer.value);
    root.right_page_num = RIGHTMOST_LEAF;
    root.parent_page_num = NO_PARENT;
    root.num_key++;

    file_write_page(root_page_num, PAGE_ADDRESS(root));
    return root_page_num;
}



/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 * 
 * Insert input ‘key/value’ (record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */ 
int db_insert(keyval_t key, char * value ) {

    pagenum_t root_page_num = header.root_page_num;
    pagenum_t leaf_page_num, temp_page_num;
    NodePage_t leaf;
    record new_record;

    /* The current implementation ignores
     * duplicates.
     */

    record temp = find(root_page_num, key, false);
    if (temp.is_null == false){
        // insertion failure
        // root page number doesn't change
        return KEY_ALREADY_EXISTS;
    }

    /* Create a new record for the
     * value.
     */
    new_record = make_record(value);


    /* Case: the tree does not exist yet.
     * Start a new tree.
     */
    // new root page has been allocated!
    // root page number change
    if (root_page_num == NO_ROOT_NODE){
        temp_page_num = start_new_tree(key, new_record);
        update_header(header.free_page_num, temp_page_num, header.num_page);
        //printf("start_new_tree() called.\n");
        return SUCCESS;
    }

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    leaf_page_num = find_leaf(root_page_num, key, false);
    file_read_page(leaf_page_num, PAGE_ADDRESS(leaf));

    /* Case: leaf has room for key and value.
     */
    // nothing changes
    if (leaf.num_key < order - 1){
        insert_into_leaf(leaf_page_num, leaf, key, new_record);
        //printf("insert_into_leaf() called.\n");
        return SUCCESS;
    }

    /* Case:  leaf must be split.
     */
    //printf("insert_into_leaf_after_splitting() called.\n");
    pagenum_t new_root_page_num = insert_into_leaf_after_splitting(root_page_num, leaf_page_num, key, new_record);
    

    // if root page number was changed, header page need to be updated
    if(new_root_page_num != root_page_num)
        update_header(header.free_page_num, new_root_page_num, header.num_page);
    return SUCCESS;
}

// Find the record containing input ‘key’.
// If found matching ‘key’, store matched ‘value’ string in ret_val and return 0. Otherwise, return non-zero value.
// Memory allocation for record structure(ret_val) should occur in caller function.
int db_find (keyval_t key, char * ret_val){
    int i = 0;
    pagenum_t page_num = find_leaf( header.root_page_num, key, false );
    NodePage_t node_page;
    record new_record;

    if (page_num == KEY_DO_NOT_EXISTS){
        return 1;
    }

    file_read_page(page_num, PAGE_ADDRESS(node_page));
    for (i = 0; i < node_page.num_key; i++)
        if (node_page.lf_record[i].key == key) break;
    if (i == node_page.num_key) {
        return 1;
    }
    else {
        strcpy(ret_val, node_page.lf_record[i].value);
        return SUCCESS;
    }
}

node * before_delete(node * root, int key) {

    node * key_leaf;
    record * key_record;

    key_record = find(root, key, false);
    key_leaf = find_leaf(root, key, false);
    if (key_record != NULL && key_leaf != NULL) {
        root = delete_entry(root, key_leaf, key, key_record);
        free(key_record);
    }
    return root;
}

pagenum_t delete( pagenum_t root_page_num, keyval_t key ){

    pagenum_t key_page_num, leaf_page_num;
    record key_record;

    key_record = find(root_page_num, key, false);
    leaf_page_num = find_leaf(root_page_num, key, false);
    if(!key_record.is_null && leaf_page_num != KEY_DO_NOT_EXISTS){
        root_page_num = delete_entry(root_page_num, leaf_page_num, key, key_record);
    }

    return root_page_num;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
pagenum_t delete_entry( pagenum_t root, pagenum_t n, keyval_t key, record pointer ) {

    int min_keys;
    node * neighbor;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, pointer);

    /* Case:  deletion from the root. 
     */

    if (n == root) 
        return adjust_root(root);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (n->num_keys >= min_keys)
        return root;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    neighbor_index = get_neighbor_index( n );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = n->parent->keys[k_prime_index];
    neighbor = neighbor_index == -1 ? n->parent->pointers[1] : 
        n->parent->pointers[neighbor_index];

    capacity = n->is_leaf ? order : order - 1;

    /* Coalescence. */

    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */

    else
        return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}

