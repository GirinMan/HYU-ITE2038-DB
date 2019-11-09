#include <join.hpp>

Join::Join(const char * pathname, int left_table_id, int right_table_id) : left(NULL), right(NULL) {

    output.open(pathname);
    if (output.fail() || !tables.in_use[left_table_id] || !tables.in_use[right_table_id]){
        is_valid = false;
    }
    else {
        is_valid = true;
    }

    this->left_table_id = left_table_id;
    this->right_table_id = right_table_id;

}

Join::~Join(){

    if (left != NULL){
        buffer_unpin_page(left);
    }

    if (right != NULL){
        buffer_unpin_page(right);
    }

    if (output.is_open()){
        output << endl;
        output.flush();
        output.close();
    }
}

void Join::write_line(keyval_t key, int left_idx, int right_idx){

    output << key   << COMMA << left->frame.node_page.lf_record[left_idx].value 
    << COMMA << key << COMMA << right->frame.node_page.lf_record[right_idx].value << endl;
}

void Join::set_block(Pagenum_t page_num, int location){
    if (location != LEFT && location != RIGHT){
        return;
    }

    if (location == LEFT){
        
        if (left != NULL){
            buffer_unpin_page(left, 1);
        }
        left = buffer_read_page(left_table_id, page_num);
    }

    
    if (location == RIGHT){
        
        if (right != NULL){
            buffer_unpin_page(right, 1);
        }
        right = buffer_read_page(right_table_id, page_num);
    }
}

int Join::join_two_blocks(){

    int i, j, left_num_keys, right_num_keys;
    LeafRecord * left_records, * right_records;
    
    left_num_keys = left->frame.node_page.num_key;
    right_num_keys = right->frame.node_page.num_key;

    left_records = left->frame.node_page.lf_record;
    right_records = right->frame.node_page.lf_record;

    i = j = 0;
    while(i < left_num_keys){

        while (left_records[i].key < right_records[j].key) {
            i++;
            if (i == left_num_keys){
                return LEFT;
            }
        }
        while (left_records[i].key > right_records[j].key) {
            j++;
            if (j == right_num_keys){
                return RIGHT;
            }
        }

        /* DEBUG */ cout << "Left key: " << left_records[i].key << " / Right Key: " << right_records[j].key << endl;
        write_line(left_records[i].key, i, j);
        i++; 
    }
    return LEFT;
}

Pagenum_t Join::get_next_leaf(int location){

    if (location == LEFT){
        return left->frame.node_page.right_page_num;
    }
    else if (location == RIGHT){
        return right->frame.node_page.right_page_num;
    }
    else{
        /* DEBUG */ cout << "뭔가 잘못됨. Join::get_next_leaf 에서 발생" << endl;
        return FAILURE;
    }
}

void Join::proceed(Pagenum_t left_leaf, Pagenum_t right_leaf){

    while(left_leaf != 0 && right_leaf != 0){

        set_block(left_leaf, LEFT);
        set_block(right_leaf, RIGHT);

        int result = join_two_blocks();

        if (result == LEFT){
            left_leaf = get_next_leaf(LEFT);
        }

        if (result == RIGHT){
            right_leaf = get_next_leaf(RIGHT);
        }
    }

    buffer_unpin_page(left);
    buffer_unpin_page(right);

    left = right = NULL;
}

// Do natural join with given two tables and write result table to the file using given pathname.
// Return 0 if success, otherwise return non-zero value.
// Two tables should have been opened earlier.
int join_table(int table_id_1, int table_id_2, char * pathname){

    bool is_finished = false;
    Join join(pathname, table_id_1, table_id_2);
    if (!join.is_valid){
        return FAILURE;
    }

    Pagenum_t left_leaf, right_leaf;
    find_leftmost_page_num(table_id_1, table_id_2, &left_leaf, &right_leaf);

    if (left_leaf == 0 || right_leaf == 0){
        return SUCCESS;
    }

    join.proceed(left_leaf, right_leaf);

    return SUCCESS;    
}

void find_leftmost_page_num(int table_id_1, int table_id_2, Pagenum_t * left_leaf, Pagenum_t * right_leaf){
    
    int table_id[2] = {table_id_1, table_id_2};
    Pagenum_t * leaf_number[2] = {left_leaf, right_leaf};

    BufferBlock_t * header, * node;
    Pagenum_t root_page_num, left_page_num;

    for (int i = 0; i < 2; i++){

        header = buffer_read_page(table_id[i], HEADER_PAGE_NUMBER);

        if ((root_page_num = header->frame.header_page.root_page_num) == NO_ROOT_NODE){
            *leaf_number[i] = NO_ROOT_NODE;
        }
        else{
            node = buffer_read_page(table_id[i], root_page_num);
            left_page_num = root_page_num;
    
            while(!node->frame.node_page.is_leaf){
                left_page_num = node->frame.node_page.extra_page_num;

                buffer_unpin_page(node);
                node = buffer_read_page(table_id[i], left_page_num);
            }

            *leaf_number[i] = left_page_num;
            buffer_unpin_page(node);
        }

        buffer_unpin_page(header);
    }
    
}

