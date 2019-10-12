#include "bpt.h"

int main(){
    int table_id = open_table("dat");

    bool terminate = false;
    char cmd;
    pagenum_t temp;
    keyval_t key, i, last;
    char msg[120] = "MSG";

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
            scanf("%ld", &key);
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
        case 'p':
        {
            scanf("%ld", &temp);
            print_node_page(temp);
        }
            break;
        case 'e':
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