#include "bpt.hpp"



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

// Find the record containing input ‘key’.
// If found matching ‘key’, store matched ‘value’ string in ret_val and return 0. Otherwise, return non-zero value.
// Memory allocation for record structure(ret_val) should occur in caller function.
int db_find (int table_id, keyval_t key, char * ret_val){

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
        strcpy(ret_val, node_page.lf_record[i].value);
        result = SUCCESS;
    }

    buffer_unpin_page(header_frame, 1);
    buffer_unpin_page(node_page_frame, 1);

    return result;
}

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

void print_tree( int table_id) {

    NodePage_t node, parent;
    Pagenum_t temp;
    int i = 0;
    int rank = 0;
    int new_rank = 0;
    BufferBlock_t * node_frame, * parent_frame;
    BufferBlock_t * header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);
    HeaderPage_t header = header_frame->frame.header_page;
    Pagenum_t root_page_num = header.root_page_num;
    buffer_unpin_page(header_frame, 1);

    if (root_page_num == NO_ROOT_NODE) {
        printf("Empty tree.\n");
        return;
    }
    queue = NULL;
    enqueue(root_page_num);
    while( queue != NULL ) {
        temp = dequeue();

        node_frame = buffer_read_page(table_id, temp);
        node = PAGE_CONTENTS(node_frame);

        if (node.parent_page_num != NO_PARENT) {

            parent_frame = buffer_read_page(table_id, node.parent_page_num);
            parent = PAGE_CONTENTS(parent_frame);
            
            if(temp == parent.extra_page_num){
                new_rank = path_to_root( table_id, root_page_num, temp );
                if (new_rank != rank) {
                    rank = new_rank;
                    printf("\n");
                }
            }
            
            buffer_unpin_page(parent_frame, 1);
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
        if (!node.is_leaf){
            for (i = 0; i <= node.num_key; i++){
                enqueue(INTERNAL_VAL(node, i));
            }
        }
        printf("| ");
        buffer_unpin_page(node_frame, 1);
    }
    printf("\n");
}

void print_leaves(int table_id){

    NodePage_t node_page;
    Pagenum_t page_num, left_page_num, right_sibling_num;
    BufferBlock_t * header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);
    HeaderPage_t header = header_frame->frame.header_page;
    Pagenum_t root_page_num = header.root_page_num;
    buffer_unpin_page(header_frame, 1);

    if(root_page_num == NO_ROOT_NODE){
        printf("This tree is an empty tree.\n");
        return;
    }

    BufferBlock_t * node_page_frame;
    node_page_frame = buffer_read_page(table_id, root_page_num);
    node_page = PAGE_CONTENTS(node_page_frame);
    
    while(!node_page.is_leaf){
        left_page_num = node_page.extra_page_num;

        buffer_unpin_page(node_page_frame, 1);
        node_page_frame = buffer_read_page(table_id, left_page_num);
        node_page = PAGE_CONTENTS(node_page_frame);
    }

    while(node_page.right_page_num != RIGHTMOST_LEAF){
        for(int i = 0; i < node_page.num_key; ++i){
            printf("%ld ", node_page.lf_record[i].key);
        }
        printf("| ");

        right_sibling_num = node_page.right_page_num;

        buffer_unpin_page(node_page_frame, 1);
        node_page_frame = buffer_read_page(table_id, right_sibling_num);
        node_page = PAGE_CONTENTS(node_page_frame);
    }

    for(int i = 0; i < node_page.num_key; ++i){
            printf("%ld ", node_page.lf_record[i].key);
    }
    buffer_unpin_page(node_page_frame, 1);
    printf("\n");
    return;
}

/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int height( int table_id, Pagenum_t root_page_num ) {
    int h = 0;
    BufferBlock_t * c_frame;

    // Create temporary in-memory node structure c
    NodePage_t c;
    c_frame = buffer_read_page(table_id, root_page_num);
    c = PAGE_CONTENTS(c_frame);

    // Read the first child of node c until the node is leaf node
    while (!c.is_leaf) {
        buffer_unpin_page(c_frame, 1);
        c_frame = buffer_read_page(table_id, c.extra_page_num);
        c = PAGE_CONTENTS(c_frame);
        h++;
    }
    buffer_unpin_page(c_frame, 1);
    return h;
}


/* Utility function to give the length in edges
 * of the path from any node to the root.
 */
int path_to_root( int table_id, Pagenum_t root_page_num, Pagenum_t child_page_num ) {
    int length = 0;

    // Create in-memory child node structure
    NodePage_t c;
    BufferBlock_t * c_frame = buffer_read_page(table_id, child_page_num);
    c = PAGE_CONTENTS(c_frame);

    // Create page number variable to store parent's page number
    Pagenum_t c_page_num = child_page_num;

    // Read parent page of the node page until the page becomes root page
    while (c_page_num != root_page_num) {
        c_page_num = c.parent_page_num;
        buffer_unpin_page(c_frame, 1);
        c_frame = buffer_read_page(table_id, c.parent_page_num);
        c = PAGE_CONTENTS(c_frame);
        length++;
    }
    buffer_unpin_page(c_frame, 1);
    return length;
}


/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
Pagenum_t find_leaf( int table_id, Pagenum_t root_page_num, keyval_t key, bool verbose ) {
    // printf("find_leaf called.\n");
    int i = 0;
    NodePage_t node_page;
    Pagenum_t page_num = root_page_num;
    if (tables.in_use[table_id] == false){
        return 0;
    }
    
    if (root_page_num == NO_ROOT_NODE) {
        if (verbose) 
            printf("Empty tree.\n");
        return KEY_DO_NOT_EXISTS;
    }
    
    BufferBlock_t * node_page_frame = buffer_read_page(table_id, page_num);
    // LINE(286) buffer_print_all();
    node_page = PAGE_CONTENTS(node_page_frame);
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

        page_num = INTERNAL_VAL(node_page, i);

        buffer_unpin_page(node_page_frame, 1);
        node_page_frame = buffer_read_page(table_id, page_num);
        node_page = PAGE_CONTENTS(node_page_frame);               
    }   

    buffer_unpin_page(node_page_frame, 1);
    // LINE(315) buffer_print_all();
    return page_num;
}


/* Finds and returns the record to which
 * a key refers.
 */
Record_t find( int table_id, Pagenum_t root_page_num, keyval_t key, bool verbose ) {
    // printf("find() called.\n");
    int i = 0;
    
    Pagenum_t page_num = find_leaf( table_id, root_page_num, key, verbose );
    //printf("// LINE #326: "); buffer_print_all();
    NodePage_t node_page;
    Record_t new_record;
    if (page_num == KEY_DO_NOT_EXISTS){
        new_record.is_null = true;
        return new_record;
    }

    BufferBlock_t * node_page_frame = buffer_read_page(table_id, page_num);
    // LINE(337) buffer_print_all();

    node_page = PAGE_CONTENTS(node_page_frame);
    for (i = 0; i < node_page.num_key; i++)
        if (node_page.lf_record[i].key == key) break;
    if (i == node_page.num_key) {
        new_record.is_null = true;
    }
    else{
        new_record.is_null = false;
        strcpy(new_record.value, node_page.lf_record[i].value);
    }
    buffer_unpin_page(node_page_frame, 1);
    // LINE(338) buffer_print_all();
    return new_record;
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
