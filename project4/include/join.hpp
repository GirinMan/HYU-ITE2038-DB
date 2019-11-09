#include <bpt.hpp>

class Join{

private:

    int left_table_id, right_table_id;
    ofstream output;
    BufferBlock_t * left, * right;

    int join_two_blocks();

    Pagenum_t get_next_leaf(int location);

public:

    bool is_valid;

    Join(const char * pathname, int left_table_id, int right_table_id);
    ~Join();

    void write_line(keyval_t key, int left_idx, int right_idx);

    void set_block(Pagenum_t, int location);

    void proceed(Pagenum_t left_leaf, Pagenum_t right_leaf);

};

// Do natural join with given two tables and write result table to the file using given pathname.
// Return 0 if success, otherwise return non-zero value.
// Two tables should have been opened earlier.
int join_table(int table_id_1, int table_id_2, char * pathname);

void find_leftmost_page_num(int table_id_1, int table_id_2, Pagenum_t * left_leaf, Pagenum_t * right_leaf);
