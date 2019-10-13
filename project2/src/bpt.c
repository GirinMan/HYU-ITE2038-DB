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
int lf_order = LEAF_ORDER;
int in_order = INTERNAL_ORDER;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = true;


struct QNode * queue = NULL;

// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES
void enqueue( Pagenum_t new_node ) {

    Pagenum_t c;
    QNode * node, *pointer;
    node = (QNode *)malloc(sizeof(QNode));
    node->page_num = new_node;
    
    if (queue == NULL) {
        queue = node;
        queue->next = NULL;
    }
    else {
        pointer = queue;
        while(pointer->next != NULL) {
            pointer = pointer->next;
        }
        pointer->next = node;
        node->next = NULL;
    }
}

Pagenum_t dequeue( void ) {

    QNode * pointer;
    Pagenum_t page_num = -1;

    if(queue != NULL) {
        pointer = queue->next;
        page_num = queue->page_num;
        free(queue);
        queue = pointer;
    }
    return page_num;
}

void print_tree( Pagenum_t root_page_num ) {

    NodePage_t node, parent;
    Pagenum_t temp;
    int i = 0;
    int rank = 0;
    int new_rank = 0;

    if (root_page_num == NO_ROOT_NODE) {
        printf("Empty tree.\n");
        return;
    }
    queue = NULL;
    enqueue(root_page_num);
    while( queue != NULL ) {
        temp = dequeue();
        file_read_page(temp, PAGE_ADDRESS(node));
        if (node.parent_page_num != NO_PARENT) {
            file_read_page(node.parent_page_num, PAGE_ADDRESS(parent));
            if(temp == parent.extra_page_num){
                new_rank = path_to_root( root_page_num, temp );
                if (new_rank != rank) {
                    rank = new_rank;
                    printf("\n");
                }
            }
            
        }
        printf("[%ld] ", temp);
        for (i = 0; i < node.num_key; i++) {
            if(node.is_leaf){
                printf("%ld ", node.lf_record[i].key);
            }
            else {
                printf("%ld ", node.in_record[i].key);
            }
        }
        if (!node.is_leaf)
            for (i = 0; i <= node.num_key; i++)
                enqueue(INTERNAL_VAL(node, i));
        printf("| ");
    }
    printf("\n");
}

void print_leaves(Pagenum_t root_page_num){

    NodePage_t node_page;
    Pagenum_t page_num, left_page_num, right_sibling_num;

    if(root_page_num == NO_ROOT_NODE){
        printf("This tree is an empty tree.\n");
        return;
    }

    file_read_page(root_page_num, PAGE_ADDRESS(node_page));
    while(!node_page.is_leaf){
        left_page_num = node_page.extra_page_num;
        file_read_page(left_page_num, PAGE_ADDRESS(node_page));
    }

    while(node_page.right_page_num != RIGHTMOST_LEAF){
        for(int i = 0; i < node_page.num_key; ++i){
            printf("%ld ", node_page.lf_record[i].key);
        }
        printf("| ");

        right_sibling_num = node_page.right_page_num;
        file_read_page(right_sibling_num, PAGE_ADDRESS(node_page));
    }

    for(int i = 0; i < node_page.num_key; ++i){
            printf("%ld ", node_page.lf_record[i].key);
    }
    printf("\n");
}

/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int height( Pagenum_t root_page_num ) {
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
int path_to_root( Pagenum_t root_page_num, Pagenum_t child_page_num ) {
    int length = 0;

    // Create in-memory child node structure
    NodePage_t c;
    file_read_page(child_page_num, PAGE_ADDRESS(c));

    // Create page number variable to store parent's page number
    Pagenum_t c_page_num = child_page_num;

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
int find_range( Pagenum_t root_page_num, keyval_t key_start, keyval_t key_end, 
    bool verbose, keyval_t returned_keys[], Pagenum_t returned_page_nums[]){

    int i, num_found, num_leaf;
    num_found = 0;
    num_leaf = 0;

    // find out the leaf page that contains key_start
    Pagenum_t page_num = find_leaf( root_page_num, key_start, verbose );

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
Pagenum_t find_leaf( Pagenum_t root_page_num, keyval_t key, bool verbose ) {
    int i = 0;
    NodePage_t node_page;
    Pagenum_t page_num = root_page_num;
    
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
Record_t find( Pagenum_t root_page_num, keyval_t key, bool verbose ) {
    int i = 0;
    Pagenum_t page_num = find_leaf( root_page_num, key, verbose );
    NodePage_t node_page;
    Record_t new_record;

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
Record_t make_record(char * value) {
    Record_t new_record;
    strcpy(new_record.value, value);
    return new_record;
}


/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
Pagenum_t make_node( void ) {
    NodePage_t new_node;
    Pagenum_t new_page_num = file_alloc_page();

    new_node.is_leaf = false;
    new_node.num_key = 0;
    new_node.parent_page_num = NO_PARENT;

    file_write_page(new_page_num, PAGE_ADDRESS(new_node));
    return new_page_num;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
Pagenum_t make_leaf( void ) {
    NodePage_t new_node;
    Pagenum_t new_page_num = file_alloc_page();

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
int get_left_index(NodePage_t parent, Pagenum_t left) {

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
Pagenum_t insert_into_leaf( Pagenum_t leaf_page_num, NodePage_t leaf, keyval_t key, Record_t pointer ) {

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
Pagenum_t insert_into_leaf_after_splitting(Pagenum_t root_page_num, Pagenum_t leaf_page_num, keyval_t key, Record_t pointer) {

    Pagenum_t new_leaf_page_num;
    NodePage_t leaf, new_leaf;
    keyval_t * temp_keys, new_key;
    Record_t * temp_records;
    int insertion_index, split, i, j;

    new_leaf_page_num = make_leaf();
    file_read_page(leaf_page_num, PAGE_ADDRESS(leaf));
    file_read_page(new_leaf_page_num, PAGE_ADDRESS(new_leaf));



    temp_keys = (keyval_t *)malloc( lf_order * sizeof(keyval_t) );
    if (temp_keys == NULL) {
        perror("Temporary keys array.");
        exit(EXIT_FAILURE);
    }

    temp_records = (Record_t *)malloc( lf_order * sizeof(Record_t) );
    if (temp_records == NULL) {
        perror("Temporary records array.");
        exit(EXIT_FAILURE);
    }

    insertion_index = 0;
    while (insertion_index < lf_order - 1 && leaf.lf_record[insertion_index].key < key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf.num_key; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = leaf.lf_record[i].key;
        strcpy(temp_records[j].value, leaf.lf_record[i].value);
    }

    temp_keys[insertion_index] = key;
    strcpy(temp_records[insertion_index].value, pointer.value);

    leaf.num_key = 0;


    split = cut(lf_order - 1);

    for (i = 0; i < split; i++) {
        leaf.lf_record[i].key = temp_keys[i];
        strcpy(leaf.lf_record[i].value, temp_records[i].value);
        leaf.num_key++;
    }

    for (i = split, j = 0; i < lf_order; i++, j++) {
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
Pagenum_t insert_into_node(Pagenum_t root_page_num, Pagenum_t parent_page_num,
    int left_index, keyval_t key, Pagenum_t right_page_num){

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
Pagenum_t insert_into_node_after_splitting(Pagenum_t root_page_num,
Pagenum_t old_node_page_num, int left_index, keyval_t key, Pagenum_t right_page_num) {

    int i, j, split;
    Pagenum_t new_node_page_num, child_page_num;
    NodePage_t old_node, right, new_node, child;
    keyval_t * temp_keys, k_prime;
    Pagenum_t * temp_records;

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

    temp_records = (Pagenum_t *)malloc( (in_order + 1) * sizeof(Pagenum_t) );
    if (temp_records == NULL) {
        perror("Temporary pointers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    temp_keys = (keyval_t *)malloc( in_order * sizeof(keyval_t) );
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

    split = cut(in_order);
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

    for (++i, j = 0; i < in_order; i++, j++) {
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
 
    Pagenum_t temp;
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
Pagenum_t insert_into_parent(Pagenum_t root_page_num,
    Pagenum_t left_page_num, keyval_t key, Pagenum_t right_page_num) {

    int left_index;
    NodePage_t left, parent;
    Pagenum_t parent_page_num, temp;

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

    if (parent.num_key < in_order - 1){
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
Pagenum_t insert_into_new_root(Pagenum_t left_page_num, keyval_t key, Pagenum_t right_page_num) {

    Pagenum_t root_page_num = file_alloc_page();
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
Pagenum_t start_new_tree(keyval_t key, Record_t pointer) {

    Pagenum_t root_page_num = make_leaf();
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

    Pagenum_t root_page_num = header.root_page_num;
    Pagenum_t leaf_page_num, temp_page_num;
    NodePage_t leaf;
    Record_t new_record;

    /* The current implementation ignores
     * duplicates.
     */

    Record_t temp = find(root_page_num, key, false);
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
    if (leaf.num_key < lf_order - 1){
        insert_into_leaf(leaf_page_num, leaf, key, new_record);
        //printf("insert_into_leaf() called.\n");
        return SUCCESS;
    }

    /* Case:  leaf must be split.
     */
    //printf("insert_into_leaf_after_splitting() called.\n");
    Pagenum_t new_root_page_num = insert_into_leaf_after_splitting(root_page_num, leaf_page_num, key, new_record);
    

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
    Pagenum_t page_num = find_leaf( header.root_page_num, key, false );
    NodePage_t node_page;
    Record_t new_record;

    if (page_num == KEY_DO_NOT_EXISTS){
        return FAILURE;
    }

    file_read_page(page_num, PAGE_ADDRESS(node_page));
    for (i = 0; i < node_page.num_key; i++)
        if (node_page.lf_record[i].key == key) break;
    if (i == node_page.num_key) {
        return FAILURE;
    }
    else {
        strcpy(ret_val, node_page.lf_record[i].value);
        return SUCCESS;
    }
}

// Find the matching record and delete it if found.
// If success, return 0. Otherwise, return non-zero value.
int db_delete( keyval_t key ){

    Pagenum_t root_page_num = header.root_page_num;
    Pagenum_t key_page_num, leaf_page_num;
    Record_t key_record;

    key_record = find(root_page_num, key, false);
    leaf_page_num = find_leaf(root_page_num, key, false);
    if(!key_record.is_null && leaf_page_num != KEY_DO_NOT_EXISTS){
        root_page_num = delete_entry(root_page_num, leaf_page_num, key);

        // if root was changed, update header
        if(root_page_num != header.root_page_num)
            update_header(header.free_page_num, root_page_num, header.num_page);
        return SUCCESS;
    }
    else return FAILURE;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
Pagenum_t delete_entry( Pagenum_t root_page_num, Pagenum_t node_page_num, keyval_t key) {
    //printf("delete_entry(%ld, %ld, %ld) called.\n", root_page_num, node_page_num, key);
    //print_tree(header.root_page_num);

    int min_keys;

    Pagenum_t neighbor_page_num;
    NodePage_t neighbor, node_page, parent;

    int neighbor_index;
    int k_prime_index;

    keyval_t k_prime;
    int capacity;

    // Remove key and pointer from node.
    //printf("Remove key and pointer from node.\n");
    node_page_num = remove_entry_from_node(node_page_num, key);
    //print_tree(header.root_page_num);

    /* Case:  deletion from the root. 
     */

    if (node_page_num == root_page_num){        
        return adjust_root(root_page_num);
    } 

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */
    file_read_page(node_page_num, PAGE_ADDRESS(node_page));

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (node_page.num_key >= min_keys)
        return root_page_num;

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
    neighbor_index = get_neighbor_index( node_page_num );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

    file_read_page(node_page.parent_page_num, PAGE_ADDRESS(parent));
    k_prime = parent.in_record[k_prime_index].key;


    switch (neighbor_index)
    {
    case LEFTMOST_LEAF:
        neighbor_page_num = parent.in_record[0].page_num;
        break;
    case 0:
        neighbor_page_num = parent.extra_page_num;
        break;    
    default:
        neighbor_page_num = parent.in_record[neighbor_index - 1].page_num;
        break;
    }
    capacity = node_page.is_leaf ? lf_order : in_order - 1;

    //printf("Capacity: %d\n", capacity);
    file_read_page(neighbor_page_num, PAGE_ADDRESS(neighbor));

    // Coalescence.
    if (neighbor.num_key + node_page.num_key < capacity)
        return coalesce_nodes(root_page_num, node_page_num, neighbor_page_num, neighbor_index, k_prime);

    //Redistribution.
    else return redistribute_nodes(root_page_num, node_page_num, neighbor_page_num, neighbor_index, k_prime_index, k_prime);
}


Pagenum_t remove_entry_from_node(Pagenum_t node_page_num, keyval_t key) {

    int i, j, num_pointers;
    NodePage_t node_page;

    file_read_page(node_page_num, PAGE_ADDRESS(node_page));

    // Remove the key and shift other keys accordingly.
    i = 0;

    if (node_page.is_leaf){
        while (node_page.lf_record[i].key != key)
            i++;
        for (++i; i < node_page.num_key; i++){
            node_page.lf_record[i - 1].key = node_page.lf_record[i].key ;
            strcpy(node_page.lf_record[i - 1].value, node_page.lf_record[i].value);
        }
    }
    else{
        while (node_page.in_record[i].key != key)
            i++;
        for (++i; i < node_page.num_key; i++){
            node_page.in_record[i - 1].key = node_page.in_record[i].key;
            node_page.in_record[i - 1].page_num = node_page.in_record[i].page_num;
        }
    }

    // One key fewer.
    node_page.num_key--;
    file_write_page(node_page_num, PAGE_ADDRESS(node_page));

    return node_page_num;
}

Pagenum_t adjust_root(Pagenum_t root_page_num) {

    //printf("adjust_root(%ld) called.\n", root_page_num);

    Pagenum_t new_root_num;
    NodePage_t root, new_root;

    file_read_page(root_page_num, PAGE_ADDRESS(root));

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root.num_key > 0)
        return root_page_num;

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.
    
    if (!root.is_leaf) {
        //printf("the root become empty, so promote the first child as the new root.\n");
        new_root_num = root.extra_page_num;
        file_read_page(new_root_num, PAGE_ADDRESS(new_root));
        new_root.parent_page_num = NO_PARENT;
        file_write_page(new_root_num, PAGE_ADDRESS(new_root));
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else{
        new_root_num = NO_ROOT_NODE;
        file_free_page(root_page_num);
    }        
    //print_tree(new_root_num);

    return new_root_num;
}

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( Pagenum_t node_page_num ) {
    // printf("get_neighbor_index(%ld) called.\n",node_page_num);

    int i;
    Pagenum_t parent_page_num;
    NodePage_t node_page, parent;

    file_read_page(node_page_num, PAGE_ADDRESS(node_page));
    parent_page_num = node_page.parent_page_num;
    file_read_page(parent_page_num, PAGE_ADDRESS(parent));

    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */

    if(parent.extra_page_num == node_page_num) return -1;
    else
        for (i = 0; i < parent.num_key; i++) {
            if (parent.in_record[i].page_num == node_page_num)
                return i;
        }

    // Error state.
    //printf("Search for nonexistent pointer to node in parent.\n");
    // printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
Pagenum_t coalesce_nodes(   Pagenum_t root_page_num, Pagenum_t node_page_num, Pagenum_t neighbor_page_num, 
                            int neighbor_index, keyval_t k_prime) {

    //printf("calesce_nodes(%ld, %ld, %ld, %d, %ld) called.\n", root_page_num, node_page_num, neighbor_page_num, neighbor_index, k_prime);

    int i, j, neighbor_insertion_index, n_end;
    Pagenum_t temp_page_num;
    NodePage_t node_page, neighbor, temp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */
    //printf("node page num: %ld\nneighbor_page_num: %ld\n", node_page_num, neighbor_page_num);
    if (neighbor_index == -1) {
        //printf("because the node was leftmost node, it was swapped with its neighbor.\n");
        temp_page_num = neighbor_page_num;
        neighbor_page_num = node_page_num;
        node_page_num = temp_page_num;
        
    }
    //printf("node page num: %ld\nneighbor_page_num: %ld\n", node_page_num, neighbor_page_num);
    file_read_page(node_page_num, PAGE_ADDRESS(node_page));
    file_read_page(neighbor_page_num, PAGE_ADDRESS(neighbor));

    //print_tree(root_page_num);

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor.num_key;
    //printf("neighbor insertion index: %d\n", neighbor_insertion_index);

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!node_page.is_leaf) {



        /* Append k_prime.
         */
        //printf("node page: ");
        //print_node(node_page, node_page_num);
        //printf("neighbor page: ");
        //print_node(neighbor, neighbor_page_num);

        neighbor.in_record[neighbor_insertion_index].key = k_prime;
        neighbor.num_key++;


        n_end = node_page.num_key;

        for (i = neighbor_insertion_index, j = 0; j < n_end; i++, j++) {
            neighbor.in_record[i + 1].key = node_page.in_record[j].key;
            neighbor.in_record[i].page_num = INTERNAL_VAL(node_page, j);;
            neighbor.num_key++;
            node_page.num_key--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor.in_record[i].page_num = INTERNAL_VAL(node_page, j);

        //print_node(neighbor, neighbor_page_num);
        //print_node(node_page, node_page_num);

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor.num_key + 1; i++) {
            temp_page_num = INTERNAL_VAL(neighbor, i);
            file_read_page(temp_page_num, PAGE_ADDRESS(temp));

            //print_node_page(temp_page_num);
            
            temp.parent_page_num = neighbor_page_num;
            file_write_page(temp_page_num, PAGE_ADDRESS(temp));
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */
    else {
        //printf("node page:"); print_node(node_page, node_page_num);
        //printf("neighbor page:"); print_node(neighbor, neighbor_page_num);
        for (i = neighbor_insertion_index, j = 0; j < node_page.num_key; i++, j++) {
            neighbor.lf_record[i].key = node_page.lf_record[j].key;
            strcpy(neighbor.lf_record[i].value, node_page.lf_record[j].value);
            neighbor.num_key++;
        }
        neighbor.right_page_num = node_page.right_page_num;
    }

    file_write_page(neighbor_page_num, PAGE_ADDRESS(neighbor));
    file_free_page(node_page_num);


    root_page_num = delete_entry(root_page_num, node_page.parent_page_num, k_prime);


    return root_page_num;
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
Pagenum_t redistribute_nodes(   Pagenum_t root_page_num, Pagenum_t node_page_num, Pagenum_t neighbor_page_num,
                                int neighbor_index, int k_prime_index, keyval_t k_prime) {  

    int i;
    Pagenum_t temp_page_num, parent_page_num;
    NodePage_t temp, root, node, neighbor, parent;

    file_read_page(node_page_num, PAGE_ADDRESS(node));
    file_read_page(neighbor_page_num, PAGE_ADDRESS(neighbor));

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!node.is_leaf)
            node.in_record[0].page_num = node.extra_page_num;
        for (i = node.num_key; i > 0; i--) {
            if(node.is_leaf){
                node.lf_record[i].key = node.lf_record[i - 1].key;
                strcpy(node.lf_record[i].value, node.lf_record[i - 1].value);
            }
            else{
                node.in_record[i].key = node.in_record[i - 1].key;
                node.in_record[i].page_num = node.in_record[i - 1].page_num;
            }            
        }
        if (!node.is_leaf) {
            node.extra_page_num = neighbor.in_record[neighbor.num_key - 1].page_num;
            //n->pointers[0] = neighbor->pointers[neighbor->num_keys];

            temp_page_num = node.extra_page_num;
            file_read_page(temp_page_num, PAGE_ADDRESS(temp));

            temp.parent_page_num = node.parent_page_num;

            //tmp = (node *)n->pointers[0];
            //tmp->parent = n;


            node.in_record[0].key = k_prime;
            parent_page_num = node.parent_page_num;
            file_read_page(parent_page_num, PAGE_ADDRESS(parent));
            parent.in_record[k_prime_index].key = neighbor.in_record[neighbor.num_key - 1].key;

            // n->keys[0] = k_prime;
            // n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];

            file_write_page(temp_page_num, PAGE_ADDRESS(temp));
            file_write_page(parent_page_num, PAGE_ADDRESS(parent));
        }
        else {
            
            node.lf_record[0].key = neighbor.lf_record[neighbor.num_key - 1].key;
            strcpy(node.lf_record[0].value, neighbor.lf_record[neighbor.num_key - 1].value);

            //n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            //neighbor->pointers[neighbor->num_keys - 1] = NULL;
            //n->keys[0] = neighbor->keys[neighbor->num_keys - 1];

            parent_page_num = node.parent_page_num;
            file_read_page(parent_page_num, PAGE_ADDRESS(parent));
            parent.in_record[k_prime_index].key = node.lf_record[0].key;

            // n->parent->keys[k_prime_index] = n->keys[0];

            file_write_page(parent_page_num, PAGE_ADDRESS(parent));
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {
        if (node.is_leaf) {
            
            node.lf_record[node.num_key].key = neighbor.lf_record[0].key;
            strcpy(node.lf_record[node.num_key].value, neighbor.lf_record[0].value);

            parent_page_num = node.parent_page_num;
            file_read_page(parent_page_num, PAGE_ADDRESS(parent));
            
            parent.in_record[k_prime_index].key = neighbor.in_record[1].key;

            // n->keys[node.num_key] = neighbor->keys[0];
            // n->pointers[node.num_key] = neighbor->pointers[0];
            // n->parent->keys[k_prime_index] = neighbor->keys[1];

            file_write_page(parent_page_num, PAGE_ADDRESS(parent));
        }
        else {

            node.in_record[node.num_key].key = k_prime;
            node.in_record[node.num_key].page_num = neighbor.extra_page_num;


            temp_page_num = node.in_record[node.num_key].page_num;
            file_read_page(temp_page_num, PAGE_ADDRESS(temp));
            
            temp.parent_page_num = node_page_num;
            file_write_page(temp_page_num, PAGE_ADDRESS(temp));


            parent_page_num = node.parent_page_num;
            file_read_page(parent_page_num, PAGE_ADDRESS(parent));

            parent.in_record[k_prime_index].key = neighbor.in_record[0].key;
            file_write_page(parent_page_num, PAGE_ADDRESS(parent));


            //n->keys[node.num_key] = k_prime;
            //n->pointers[node.num_key + 1] = neighbor->pointers[0];
            //tmp = (node *)n->pointers[node.num_key + 1];
            //tmp->parent = n;
            //n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        if (!node.is_leaf)
            neighbor.extra_page_num = neighbor.in_record[0].page_num;
        for (i = 0; i < neighbor.num_key - 1; i++) {
            if(node.is_leaf){
                neighbor.lf_record[i].key = neighbor.lf_record[i + 1].key;
                strcpy(neighbor.lf_record[i].value, neighbor.lf_record[i + 1].value);
            }
            else{
                neighbor.in_record[i].key = neighbor.in_record[i + 1].key;
                neighbor.in_record[i].page_num = neighbor.in_record[i + 1].page_num;
            }
        }        
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    node.num_key++;
    neighbor.num_key--;

    file_write_page(node_page_num, PAGE_ADDRESS(node));
    file_write_page(neighbor_page_num, PAGE_ADDRESS(neighbor));


    return root_page_num;
}