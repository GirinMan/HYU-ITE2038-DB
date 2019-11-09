#include "bpt.hpp"

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
Pagenum_t make_node( int table_id, bool is_leaf) {

    BufferBlock_t * new_frame = buffer_allocate_page(table_id);

    NodePage_t new_node;

    new_node.is_leaf = is_leaf;
    new_node.num_key = 0;
    new_node.parent_page_num = NO_PARENT;

    buffer_write_page(new_frame, PAGE_T(new_node));
    buffer_unpin_page(new_frame, 2);

    return new_frame->page_num;
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
Pagenum_t insert_into_leaf(int table_id, Pagenum_t leaf_page_num, keyval_t key, Record_t pointer ) {
    // printf("insert_into_leaf called.\n");

    int i, insertion_point;
    insertion_point = 0;
    BufferBlock_t * leaf_frame = buffer_read_page(table_id, leaf_page_num);
    NodePage_t leaf = PAGE_CONTENTS(leaf_frame);


    while (insertion_point < leaf.num_key && leaf.lf_record[insertion_point].key < key)
        insertion_point++;

    for (i = leaf.num_key; i > insertion_point; i--) {
        leaf.lf_record[i].key = leaf.lf_record[i - 1].key;
        strcpy(leaf.lf_record[i].value, leaf.lf_record[i - 1].value);
    }
    leaf.lf_record[insertion_point].key = key;
    strcpy(leaf.lf_record[insertion_point].value, pointer.value);
    leaf.num_key++;

    buffer_write_page(leaf_frame, PAGE_T(leaf));
    buffer_unpin_page(leaf_frame, 2);

    return leaf_page_num;
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
Pagenum_t insert_into_leaf_after_splitting(int table_id, Pagenum_t root_page_num, Pagenum_t leaf_page_num, keyval_t key, Record_t pointer) {
    // printf("insert_into_leaf_after_splitting called.\n");


    BufferBlock_t * leaf_frame, * new_leaf_frame;

    Pagenum_t new_leaf_page_num, temp;
    NodePage_t leaf, new_leaf;

    keyval_t * temp_keys, new_key;
    Record_t * temp_records;

    int insertion_index, split, i, j;

    new_leaf_page_num = make_node(table_id, true);

    leaf_frame = buffer_read_page(table_id, leaf_page_num);
    new_leaf_frame = buffer_read_page(table_id, new_leaf_page_num);

    leaf = PAGE_CONTENTS(leaf_frame);
    new_leaf = PAGE_CONTENTS(new_leaf_frame);

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

    new_leaf.parent_page_num = leaf.parent_page_num;

    buffer_write_page(leaf_frame, PAGE_T(leaf));
    buffer_write_page(new_leaf_frame, PAGE_T(new_leaf));

    new_key = new_leaf.lf_record[0].key;
    temp = insert_into_parent(table_id, root_page_num, leaf_page_num, new_key, new_leaf_page_num);

    buffer_unpin_page(leaf_frame, 2);
    buffer_unpin_page(new_leaf_frame, 2);

    return temp;
}


/* Inserts a new key and record to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
Pagenum_t insert_into_node(int table_id, Pagenum_t root_page_num, Pagenum_t parent_page_num,
    int left_index, keyval_t key, Pagenum_t right_page_num){
    // printf("insert_into_node called.\n");

    BufferBlock_t * parent_frame;

    int i;
    NodePage_t parent;

    parent_frame = buffer_read_page(table_id, parent_page_num);
    parent = PAGE_CONTENTS(parent_frame);

    for (i = parent.num_key; i > left_index; i--) {
        parent.in_record[i].key = parent.in_record[i - 1].key;
        parent.in_record[i].page_num = parent.in_record[i - 1].page_num;
    }
    parent.in_record[left_index].key = key;   
    parent.in_record[left_index].page_num = right_page_num;
    
    parent.num_key++;

    buffer_write_page(parent_frame, PAGE_T(parent));
    buffer_unpin_page(parent_frame, 2);

    return root_page_num;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
Pagenum_t insert_into_node_after_splitting(int table_id, Pagenum_t root_page_num,
Pagenum_t old_node_page_num, int left_index, keyval_t key, Pagenum_t right_page_num) {
    // printf("insert_into_node_after_splitting called.\n");
    BufferBlock_t * old_node_frame, * right_frame, * new_node_frame, * child_frame;

    int i, j, split;
    Pagenum_t new_node_page_num, child_page_num;
    NodePage_t old_node, new_node, child;
    keyval_t * temp_keys, k_prime;
    Pagenum_t * temp_records;

    old_node_frame = buffer_read_page(table_id, old_node_page_num);

    old_node = PAGE_CONTENTS(old_node_frame);

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

    new_node_page_num = make_node(table_id, false);
    new_node_frame = buffer_read_page(table_id, new_node_page_num);
    new_node = PAGE_CONTENTS(new_node_frame);

    old_node.num_key = 0;

    for (i = 0; i < split - 1; i++) {
        if(!i) old_node.extra_page_num = temp_records[i];
        else old_node.in_record[i-1].page_num = temp_records[i];
        old_node.in_record[i].key = temp_keys[i];
        old_node.num_key++;
    }

    old_node.in_record[i-1].page_num = temp_records[i];
    buffer_write_page(old_node_frame, PAGE_T(old_node));

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
    buffer_write_page(new_node_frame, PAGE_T(new_node));

    //printf("new node page info: ");
    //print_node_page(new_node_page_num);
 
    Pagenum_t temp;
    for (i = 0; i <= new_node.num_key; i++) {
        if(!i) temp = new_node.extra_page_num;
        else temp = new_node.in_record[i-1].page_num;

        BufferBlock_t * temp_frame = buffer_read_page(table_id, temp);
        child = PAGE_CONTENTS(temp_frame);
        child.parent_page_num = new_node_page_num;

        buffer_write_page(temp_frame, PAGE_T(child));
        buffer_unpin_page(temp_frame, 2);
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */
    //printf("call insert_into_parent(%ld, %ld, %ld, %ld)\n", root_page_num, old_node_page_num, k_prime, new_node_page_num);
    temp = insert_into_parent(table_id, root_page_num, old_node_page_num, k_prime, new_node_page_num);

    buffer_unpin_page(old_node_frame, 2);
    buffer_unpin_page(new_node_frame, 2);

    return temp;
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
Pagenum_t insert_into_parent(int table_id, Pagenum_t root_page_num,
    Pagenum_t left_page_num, keyval_t key, Pagenum_t right_page_num) {
        // printf("insert_into_parent called.\n");

    BufferBlock_t * left_frame, * parent_frame;

    int left_index;
    NodePage_t left, parent;
    Pagenum_t parent_page_num, temp;

    left_frame = buffer_read_page(table_id, left_page_num);
    left = PAGE_CONTENTS(left_frame);
    parent_page_num = left.parent_page_num;

    /* Case: new root. */

    if (parent_page_num == NO_PARENT){
        //printf("new root have to be created. insert_into_new_root(%ld, %ld, %ld) called.\n", left_page_num, key, right_page_num);
        temp = insert_into_new_root(table_id, left_page_num, key, right_page_num);
        left.parent_page_num = temp;

        buffer_write_page(left_frame, PAGE_T(left));
        buffer_unpin_page(left_frame, 2);

        //printf("new root information: ");
        //print_node_page(temp);
        return temp;
    }

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    parent_frame = buffer_read_page(table_id, parent_page_num);
    parent = PAGE_CONTENTS(parent_frame);
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
        temp = insert_into_node(table_id, root_page_num, parent_page_num, left_index, key, right_page_num);        
        buffer_unpin_page(left_frame, 1);
        buffer_unpin_page(parent_frame, 1);
        return temp;
    }

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */

    //printf("insert_into_node_after_splitting() called.\n");
    temp = insert_into_node_after_splitting(table_id, root_page_num, parent_page_num, left_index, key, right_page_num);
    buffer_unpin_page(left_frame, 1);
    buffer_unpin_page(parent_frame, 1);

    return temp;
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
Pagenum_t insert_into_new_root(int table_id, Pagenum_t left_page_num, keyval_t key, Pagenum_t right_page_num) {
    // printf("insert_into_new_root called.\n");
    BufferBlock_t * right_frame, * root_frame;

    Pagenum_t root_page_num = make_node(table_id, false);
    NodePage_t root, right;

    root_frame = buffer_read_page(table_id, root_page_num);
 
    root.parent_page_num = NO_PARENT;
    root.is_leaf = false;
    root.num_key = 1;
    root.extra_page_num = left_page_num;
    root.in_record[0].key = key;
    root.in_record[0].page_num = right_page_num;

    right_frame = buffer_read_page(table_id, right_page_num);
    right = PAGE_CONTENTS(right_frame);
    right.parent_page_num = root_page_num;

    buffer_write_page(root_frame, PAGE_T(root));
    buffer_write_page(right_frame, PAGE_T(right));

    buffer_unpin_page(root_frame, 2);
    buffer_unpin_page(right_frame, 2);
    
    return root_page_num;
}



/* First insertion:
 * start a new tree.
 */
Pagenum_t start_new_tree(int table_id, keyval_t key, Record_t pointer) {
    // printf("start new tree called.\n");
    BufferBlock_t * root_frame;
    Pagenum_t root_page_num = make_node(table_id, true);
    NodePage_t root;
    root_frame = buffer_read_page(table_id, root_page_num);
    root = PAGE_CONTENTS(root_frame);
    root.lf_record[0].key = key;
    strcpy(root.lf_record[0].value, pointer.value);
    root.right_page_num = RIGHTMOST_LEAF;
    root.parent_page_num = NO_PARENT;
    root.num_key++;

    buffer_write_page(root_frame, PAGE_T(root));

    buffer_unpin_page(root_frame, 2);
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
int db_insert(int table_id, keyval_t key, char * value ) {
    // printf("db_insert called.\n");
    Pagenum_t root_page_num;
    Pagenum_t leaf_page_num, temp_page_num;
    HeaderPage_t header;
    NodePage_t leaf;
    Record_t new_record;

    BufferBlock_t * header_frame, * leaf_frame;

    if (tables.in_use[table_id] == false){
        printf("Required table is not opened yet!\n");
        return FAILURE;
    }

    header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);
    header = header_frame->frame.header_page;
    root_page_num = header.root_page_num;

    /* The current implementation ignores
     * duplicates.
     */

    Record_t temp = find(table_id, root_page_num, key, false);
    
    if (temp.is_null == false){
        // insertion failure
        // root page number doesn't change
        buffer_unpin_page(header_frame, 1);
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
        temp_page_num = start_new_tree(table_id, key, new_record);

        //header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);
        header = header_frame->frame.header_page;

        //update_header(header.free_page_num, temp_page_num, header.num_page);

        header.root_page_num = temp_page_num;
        buffer_write_page(header_frame, PAGE_T(header));
        buffer_unpin_page(header_frame, 2);

        //printf("start_new_tree() called.\n");
        return SUCCESS;
    }

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    leaf_page_num = find_leaf(table_id, root_page_num, key, false);

    leaf_frame = buffer_read_page(table_id, leaf_page_num);
    leaf = PAGE_CONTENTS(leaf_frame);

    /* Case: leaf has room for key and value.
     */
    // nothing changes
    if (leaf.num_key < lf_order - 1){
        insert_into_leaf(table_id, leaf_page_num, key, new_record);
        //printf("insert_into_leaf() called.\n");

        buffer_unpin_page(header_frame, 1);
        buffer_unpin_page(leaf_frame, 1);

        return SUCCESS;
    }

    /* Case:  leaf must be split.
     */
    //printf("insert_into_leaf_after_splitting() called.\n");
    Pagenum_t new_root_page_num = insert_into_leaf_after_splitting(table_id, root_page_num, leaf_page_num, key, new_record);
    

    // if root page number was changed, header page need to be updated
    if(new_root_page_num != root_page_num){
        header = header_frame->frame.header_page;
        header.root_page_num = new_root_page_num;
        buffer_write_page(header_frame, PAGE_T(header));
        buffer_unpin_page(header_frame, 1);
    }
    
    buffer_unpin_page(header_frame, 1);
    buffer_unpin_page(leaf_frame, 1);
    
    return SUCCESS;
}