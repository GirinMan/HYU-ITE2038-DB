#include "bpt.h"

void print_instructions(){
    printf( "Welcome to disk-based B+ tree implementation!\n\nInstructions---------------\n\n");
    printf("?: Review the instructions\n");
    printf("o [pathname] : Open table file located at given pathname.\n");
    printf("i [key] [value] : Insert (key, value) record into current tree.\n");
    printf("m [first] [last] : Massively insert keys(first <= key < last) into current tree.\n");
    printf("f [key] : Find if key exists in the tree. If it exists, print value.\n");
    printf("d [key] : Delete key in current tree.\n");
    printf("p : Print the shape of current tree.\n");
    printf("l : Print the leaves of current tree.\n");
    printf("h : Print the information of header page of current tree.\n");
    printf("p : Print the shape of current tree.\n");
    printf("n [page number] : Print the information of the page that page number is pointing at.\n");
    printf("q : exit program.\n\n--------------------------------\n");
}

int main(){

    bool terminate = false;
    Pagenum_t temp;
    keyval_t key, i, last;
    char cmd, msg[120] = "MSG", buf[120];
    int table_id = -1, input_status, number;

    while(table_id == -1){
        printf("Enter your file path: ");
        scanf("%s", buf);
        table_id = open_table(buf);
        if(table_id == -1) printf("Invalid path.\n");
        else printf("The file [%s] has been opened. [Table ID: %d]\n", buf, table_id);
    }

    print_instructions();

    while(1){

        if(terminate) break;

        scanf("%c", &cmd);

        switch (cmd) {
        case 'h':
            print_header_page();
            break;
        case 'i':
            scanf("%ld %s", &key, msg);
            input_status = db_insert(key, msg);
            if(input_status) printf("Insertion failed\n");
            else printf("Insertion success.\n");
            break;
        case 'o':
            scanf("%s", buf);
            table_id = open_table(buf);
            if(table_id == -1) printf("Invalid path.\n");
            else printf("The file [%s] has been opened. [Table ID: %d]\n", buf, table_id);
            break;
        case 'm':
            scanf("%ld %ld", &key, &last);
            for(i = key; i < last; i++){
                db_insert(i, msg);
            }  
            printf("massive insertion from %ld to %ld finished\n", key, last);      
            break;    
        case 'd':
            scanf("%ld", &key);
            input_status = db_delete(key);
            if(input_status) printf("delete %ld failed. Key doesn't exist in the tree.\n", key);
            else printf("delete %ld success.\n", key);               
            break;
        case 'f':
            scanf("%ld", &key);
            input_status = db_find(key, buf);
            if(input_status) printf("find %ld failed. Key doesn't exist in the tree.\n", key);
            else printf("find %ld success. value: %s\n", key, buf);      
            break;  
        case 'n':
            scanf("%ld", &temp);
            print_node_page(temp);
            break;
        case 'p':
            print_tree(header.root_page_num);
            break;
        case 'l':
            print_leaves(header.root_page_num);
            break;
        case 'q':
            terminate = true;
            break;
        case '?':
            print_instructions();
            break;
        default:
        break;
        }
    }
    return 0;
}

