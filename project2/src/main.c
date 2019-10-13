#include "bpt.h"

int main(){
    int table_id = open_table("datafile.txt");

    bool terminate = false;
    char cmd;
    Pagenum_t temp;
    keyval_t key, i, last;
    char msg[120] = "MSG", buf[120];

    int input_status, number;

    while(1){

        if(terminate) break;

        scanf("%c", &cmd);

        switch (cmd) {
        case 'h':
        {
            print_header_page();
        }
            break;
        case 'i':
        {
            scanf("%ld %s", &key, msg);
            input_status = db_insert(key, msg);
            if(input_status) printf("Insertion failed\n");
            else printf("Insertion success.\n");
        }
            break;
        case 'm':
        {
            scanf("%ld %ld", &key, &last);
            for(i = key; i < last; i++){
                db_insert(i, msg);
            }  
            printf("massive insertion from %ld to %ld finished\n", key, last);          
        }
            break;
        case 'd':
        {
            scanf("%ld", &key);
            input_status = db_delete(key);
            if(input_status) printf("delete %ld failed. Key doesn't exist in the tree.\n", key);
            else printf("delete %ld success.\n", key);               
        }
            break;
        case 'f':
        {
            scanf("%ld", &key);
            input_status = db_find(key, buf);
            if(input_status) printf("find %ld failed. Key doesn't exist in the tree.\n", key);
            else printf("find %ld success. value: %s\n", key, buf);               
        }
            break;
        case 'n':
        {
            scanf("%ld", &temp);
            print_node_page(temp);
        }
            break;
        case 'e':
        {
            scanf("%ld", &temp);
            printf("%ld enqueued\n", temp);
            enqueue(temp);
        }
            break;
        case 'u':
        {
            printf("%ld dequeued\n", dequeue());
        }
            break;
        case 'p':
        {
            print_tree(header.root_page_num);
        }
            break;
        case 'l':
        {
            print_leaves(header.root_page_num);
        }
            break;
        case 'q':
        {
            terminate = true;
        }
            break;
        default:
            break;
        }
    }
    return 0;
}

