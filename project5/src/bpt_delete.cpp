#include "bpt.hpp"

// DELETION

// Find the matching record and delete it if found.
// If success, return 0. Otherwise, return non-zero value.
int db_delete( int table_id, keyval_t key ){

    if (tables.in_use[table_id] == false){
        return FAILURE;
    }

    BufferBlock_t * header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);
    HeaderPage_t header = header_frame->frame.header_page;

    Pagenum_t root_page_num = header.root_page_num;
    Pagenum_t key_page_num, leaf_page_num;
    Record_t key_record;
    
    key_record = find(table_id, root_page_num, key, false);
    leaf_page_num = find_leaf(table_id, root_page_num, key, false);
    if(!key_record.is_null && leaf_page_num != KEY_DO_NOT_EXISTS){

        // // LINE(24) // buffer_print_all();

        root_page_num = delete_entry(table_id, root_page_num, leaf_page_num, key);

        // // LINE(28) // buffer_print_all();

        // if root was changed, update header
        if(root_page_num != header.root_page_num){
            header = header_frame->frame.header_page;
            header.root_page_num = root_page_num;
            buffer_write_page(header_frame, PAGE_T(header));
            buffer_unpin_page(header_frame, 1);
        }
        buffer_unpin_page(header_frame, 1);
        return SUCCESS;
    }
    else {
        buffer_unpin_page(header_frame, 1);
        return FAILURE;
    }
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
Pagenum_t delete_entry( int table_id, Pagenum_t root_page_num, Pagenum_t node_page_num, keyval_t key) {
    // printf("delete_entry(root_page_num: %ld, node_page_num: %ld, key: %ld) called.\n", root_page_num, node_page_num, key);
    //print_tree(header.root_page_num);

    int min_keys;

    Pagenum_t neighbor_page_num, temp;
    NodePage_t neighbor, node_page, parent;

    int neighbor_index;
    int k_prime_index;

    keyval_t k_prime;
    int capacity;

    // Remove key and pointer from node.
    //printf("Remove key and pointer from node.\n");
    node_page_num = remove_entry_from_node(table_id, node_page_num, key);

    // // LINE(70) // buffer_print_all();

    /* Case:  deletion from the root. 
     */

    if (node_page_num == root_page_num){
        temp = adjust_root(table_id, root_page_num);
        // LINE(77)  buffer_print_all();
        return temp;
    }

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    BufferBlock_t * node_page_frame = buffer_read_page(table_id, node_page_num);
    node_page = PAGE_CONTENTS(node_page_frame);

    // // LINE(88) // buffer_print_all();

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (node_page.num_key >= min_keys){
        buffer_unpin_page(node_page_frame, 1);
        // // LINE(102) // buffer_print_all();
        return root_page_num;
    }

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
    neighbor_index = get_neighbor_index( table_id, node_page_num );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

    BufferBlock_t * parent_frame = buffer_read_page(table_id, node_page.parent_page_num);
    parent = PAGE_CONTENTS(parent_frame);

    // // LINE(123) // buffer_print_all();

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
    BufferBlock_t * neighbor_frame = buffer_read_page(table_id, neighbor_page_num);
    neighbor = PAGE_CONTENTS(neighbor_frame);

    // // LINE(145) // buffer_print_all();

    // Coalescence.
    if (neighbor.num_key + node_page.num_key < capacity){
        temp = coalesce_nodes(table_id, root_page_num, node_page_num, neighbor_page_num, neighbor_index, k_prime);
    }

    //Redistribution.
    else {
        temp = redistribute_nodes(table_id, root_page_num, node_page_num, neighbor_page_num, neighbor_index, k_prime_index, k_prime);
    }

    // LINE(157)  buffer_print_all();

    if(node_page_frame->table_id) buffer_unpin_page(node_page_frame, 1);
    if(parent_frame->table_id) buffer_unpin_page(parent_frame, 1);
    if(neighbor_frame->table_id) buffer_unpin_page(neighbor_frame, 1);

    // // LINE(163) // buffer_print_all();

    return temp;
}


Pagenum_t remove_entry_from_node(int table_id, Pagenum_t node_page_num, keyval_t key) {
    // printf("remove_entry_from_node(node_page_num: %ld, key: %ld) called.\n", node_page_num, key);
    int i, j, num_pointers;
    NodePage_t node_page;

    BufferBlock_t * node_page_frame = buffer_read_page(table_id, node_page_num);
    node_page = PAGE_CONTENTS(node_page_frame);

    // // LINE(177) // buffer_print_all();

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

    buffer_write_page(node_page_frame, PAGE_T(node_page));
    buffer_unpin_page(node_page_frame, 2);

    // // LINE(205) // buffer_print_all();

    return node_page_num;
}

Pagenum_t adjust_root(int table_id, Pagenum_t root_page_num) {

    // printf("adjust_root(root_page_num: %ld) called.\n", root_page_num);

    Pagenum_t new_root_num;
    NodePage_t root, new_root;

    BufferBlock_t * root_frame, * new_root_frame;

    root_frame = buffer_read_page(table_id, root_page_num);
    root = PAGE_CONTENTS(root_frame);


    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root.num_key > 0){
        buffer_unpin_page(root_frame, 1);
        // // LINE(231) // buffer_print_all();
        return root_page_num;
    }

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.
    
    if (!root.is_leaf) {
        printf("the root become empty, so promote the first child as the new root.\n");
        new_root_num = root.extra_page_num;

        new_root_frame = buffer_read_page(table_id, new_root_num);
        new_root = PAGE_CONTENTS(new_root_frame);        
        new_root.parent_page_num = NO_PARENT;
        
        buffer_write_page(new_root_frame, PAGE_T(new_root));
        buffer_unpin_page(new_root_frame, 2);
        // buffer_print_page(new_root_frame); // // LINE(254) // buffer_print_all();
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.
    else{
        new_root_num = NO_ROOT_NODE;
    }

    buffer_free_page(root_frame);        

    BufferBlock_t * header_frame = buffer_read_page(table_id, HEADER_PAGE_NUMBER);
    header_frame->frame.header_page.root_page_num = new_root_num;
    header_frame->is_dirty = true; buffer_unpin_page(header_frame, 1);

    // LINE(266) buffer_print_all();

    return new_root_num;
}

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( int table_id, Pagenum_t node_page_num ) {
    // printf("get_neighbor_index(%ld) called.\n",node_page_num);

    int i;
    Pagenum_t parent_page_num;
    NodePage_t node_page, parent;

    BufferBlock_t * node_page_frame, * parent_page_frame;

    node_page_frame = buffer_read_page(table_id, node_page_num);
    node_page = PAGE_CONTENTS(node_page_frame);

    parent_page_num = node_page.parent_page_num;

    parent_page_frame = buffer_read_page(table_id, parent_page_num);
    parent = PAGE_CONTENTS(parent_page_frame);

    // // LINE(294) // buffer_print_all();

    /* Return the index of the key to the left of the pointer in the parent pointing to n.  
     * If n is the leftmost child, this means return -1.
     */

    if(parent.extra_page_num == node_page_num){
        buffer_unpin_page(node_page_frame, 1);
        buffer_unpin_page(parent_page_frame, 1);        
        return -1;
    } 
    else
        for (i = 0; i < parent.num_key; i++) {
            if (parent.in_record[i].page_num == node_page_num){
                buffer_unpin_page(node_page_frame, 1);
                buffer_unpin_page(parent_page_frame, 1);
                return i;
            }
        }

    // Error state.
    //printf("Search for nonexistent pointer to node in parent.\n");
    exit(EXIT_FAILURE);
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
Pagenum_t coalesce_nodes( int table_id,  Pagenum_t root_page_num, Pagenum_t node_page_num, Pagenum_t neighbor_page_num, 
                            int neighbor_index, keyval_t k_prime) {

    /* printf("coalesce_nodes(root_page_num: %ld, node_page_num: %ld, neighbor_page_num: %ld, neighbor_index: %d, k_prime: %ld) called.\n", 
     *       root_page_num, node_page_num, neighbor_page_num, neighbor_index, k_prime);
     */
    int i, j, neighbor_insertion_index, n_end;
    Pagenum_t temp_page_num, temp_root;
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
    // printf("node page num: %ld\nneighbor_page_num: %ld\n", node_page_num, neighbor_page_num);

    BufferBlock_t * node_page_frame, * neighbor_page_frame, * temp_frame;

    node_page_frame = buffer_read_page(table_id, node_page_num);
    node_page = PAGE_CONTENTS(node_page_frame);

    neighbor_page_frame = buffer_read_page(table_id, neighbor_page_num);
    neighbor = PAGE_CONTENTS(neighbor_page_frame);

    // LINE(355) buffer_print_page(node_page_frame); buffer_print_page(neighbor_page_frame);

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

        // LINE(401) print_node(neighbor, neighbor_page_num);
        // LINE(402) print_node(node_page, node_page_num);

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < (neighbor.num_key + 1); i++) {
            temp_page_num = INTERNAL_VAL(neighbor, i);

            temp_frame = buffer_read_page(table_id, temp_page_num);
            temp = PAGE_CONTENTS(temp_frame);

            temp.parent_page_num = neighbor_page_num;
            // print_node(temp, temp_page_num);
            
            buffer_write_page(temp_frame, PAGE_T(temp));
            buffer_unpin_page(temp_frame, 2); // LINE(417) buffer_print_page(temp_frame);
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

    buffer_write_page(neighbor_page_frame, PAGE_T(neighbor));

    root_page_num = delete_entry(table_id, root_page_num, node_page.parent_page_num, k_prime);



    buffer_free_page(node_page_frame);

    buffer_unpin_page(neighbor_page_frame, 2);

    // LINE(448) buffer_print_page(neighbor_page_frame);

    return root_page_num;
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
Pagenum_t redistribute_nodes(  int table_id, Pagenum_t root_page_num, Pagenum_t node_page_num, Pagenum_t neighbor_page_num,
                                int neighbor_index, int k_prime_index, keyval_t k_prime) {  
    printf("redistribute_nodes(root_page_num: %ld, node_page_num: %ld, neighbor_page_num: %ld, neighbor_index: %d, k_prime_index: %d, k_prime: %ld\n",
            root_page_num, node_page_num, neighbor_page_num, neighbor_index, k_prime_index, k_prime);
    int i;
    Pagenum_t temp_page_num, parent_page_num;
    NodePage_t temp, root, node, neighbor, parent;

    BufferBlock_t * node_frame, * neighbor_page_frame, * temp_frame, * root_frame, * parent_frame;

    node_frame = buffer_read_page(table_id, node_page_num);
    node = PAGE_CONTENTS(node_frame);
    neighbor_page_frame = buffer_read_page(table_id, neighbor_page_num);
    neighbor = PAGE_CONTENTS(neighbor_page_frame);

    // // LINE(473) // buffer_print_all();

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
            temp_frame = buffer_read_page(table_id, temp_page_num);
            temp = PAGE_CONTENTS(temp_frame);

            temp.parent_page_num = node.parent_page_num;

            node.in_record[0].key = k_prime;
            parent_page_num = node.parent_page_num;

            parent_frame = buffer_read_page(table_id, parent_page_num);
            parent = PAGE_CONTENTS(parent_frame);

            parent.in_record[k_prime_index].key = neighbor.in_record[neighbor.num_key - 1].key;

            // n->keys[0] = k_prime;
            // n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];

            buffer_write_page(temp_frame, PAGE_T(temp));
            buffer_write_page(parent_frame, PAGE_T(parent));

            buffer_unpin_page(temp_frame, 2);
            buffer_unpin_page(parent_frame, 2);

            // // LINE(520) // buffer_print_all();
        }
        else {
            
            node.lf_record[0].key = neighbor.lf_record[neighbor.num_key - 1].key;
            strcpy(node.lf_record[0].value, neighbor.lf_record[neighbor.num_key - 1].value);

            //n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            //neighbor->pointers[neighbor->num_keys - 1] = NULL;
            //n->keys[0] = neighbor->keys[neighbor->num_keys - 1];

            parent_page_num = node.parent_page_num;

            parent_frame = buffer_read_page(table_id, parent_page_num);
            parent = PAGE_CONTENTS(parent_frame);

            parent.in_record[k_prime_index].key = node.lf_record[0].key;

            // n->parent->keys[k_prime_index] = n->keys[0];
            buffer_write_page(parent_frame, PAGE_T(parent));
            buffer_unpin_page(parent_frame, 2);

            // // LINE(542) // buffer_print_all();
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
            parent_frame = buffer_read_page(table_id, parent_page_num);
            parent = PAGE_CONTENTS(parent_frame);
            
            parent.in_record[k_prime_index].key = neighbor.in_record[1].key;

            // n->keys[node.num_key] = neighbor->keys[0];
            // n->pointers[node.num_key] = neighbor->pointers[0];
            // n->parent->keys[k_prime_index] = neighbor->keys[1];

            buffer_write_page(parent_frame, PAGE_T(parent));
            buffer_unpin_page(parent_frame, 2);

            // // LINE(571) // buffer_print_all();
        }
        else {

            node.in_record[node.num_key].key = k_prime;
            node.in_record[node.num_key].page_num = neighbor.extra_page_num;


            temp_page_num = node.in_record[node.num_key].page_num;
            temp_frame = buffer_read_page(table_id, temp_page_num);
            temp = PAGE_CONTENTS(temp_frame);
            
            temp.parent_page_num = node_page_num;
            buffer_write_page(temp_frame, PAGE_T(temp));
            buffer_unpin_page(temp_frame, 2);


            parent_page_num = node.parent_page_num;
            parent_frame = buffer_read_page(table_id, parent_page_num);
            parent = PAGE_CONTENTS(parent_frame);

            parent.in_record[k_prime_index].key = neighbor.in_record[0].key;
            buffer_write_page(parent_frame, PAGE_T(parent));
            buffer_unpin_page(parent_frame, 2);

            // // LINE(596) // buffer_print_all();
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

    buffer_write_page(node_frame, PAGE_T(node));
    buffer_unpin_page(node_frame, 2);
    buffer_write_page(neighbor_page_frame, PAGE_T(neighbor));
    buffer_unpin_page(neighbor_page_frame, 2);

    // // LINE(624) // buffer_print_all();

    return root_page_num;
}