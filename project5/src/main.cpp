#include "join.hpp"
#include "transaction.hpp"

void print_instructions(){
    printf( "Welcome to disk-based B+ tree implementation!\n\nInstructions---------------\n\n");
    printf("?: Review the instructions\n");
    printf("o [pathname] : Open table file located at given pathname.\n");
    printf("c [table ID] : Close table and flush all data into file corresponding to table ID.\n");
    printf("t : Print the current opened tables.\n");
    printf("i [table ID] [key] [value]: Insert (key, value) record into table corresponding to ID.\n");
    printf("m [table ID] [first] [last] : Massively insert keys(first <= key < last) into table corresponding to ID.\n");
    printf("f [table ID] [key] : Find if key exists in table corresponding to ID. If it exists, print value.\n");
    printf("d [table ID] [key] : Delete key in table corresponding to ID.\n");
    printf("l [table ID] : Print the leaves of current tree.\n");
    printf("p [table ID] : Print the shape of current tree.\n");
    printf("b : Print the current status of buffer.\n");
    printf("n [table ID] [page number] : Print the information of the page that page number is pointing at.\n");
    printf("s : Close all of tables currently opened and flush data of them into disk.\n");
    printf("q : exit program.\n\n--------------------------------\n");
}

int main(){

    bool terminate = false;
    Pagenum_t temp;
    keyval_t key, i, last;
    char cmd, msg[120] = "MSG", buf[120], path[512];
    unsigned int buffer_num = 0, input_status, number, table_id;

    bool input_valid = false;
    while(!input_valid){
        printf("Enter the number of frame that will be used in buffer> ");
        scanf("%d", &buffer_num);
        if(((signed int)buffer_num) < 1){
            cin.clear();
            buffer_num = 0;
        }
        else{
            init_db(buffer_num);
            input_valid = true;           
        }
    }
    

    print_instructions();

    while(1){

        if(terminate) break;

        cin >> cmd;

        if (cmd == '?'){
            print_instructions();
        }
        else if (cmd == 'o'){
            cout << "Enter the path of table to open> ";
            cin >> path;
            table_id = open_table(path);

            if ((signed int)table_id < 0){
                printf("Unable to open file at %s.\n", path);
            }
            else{
                printf("Table file opend at table ID %d.\n", table_id);
            }
        }
        else if (cmd == 'c'){
            cin >> number;
            cout << "Closing table " << number << "." << endl;
            close_table(number);          
        }
        else if (cmd == 'c'){
            cin >> number;
            close_table(number);          
        }
        else if (cmd == 't'){
            print_all_tables();            
        }
        else if (cmd == 'i'){
            cin >> number >> key >> msg;
            input_status = db_insert(number, key, msg);
            if(input_status) cout << "Insertion failed" << endl;
            else cout << "Insertion success." << endl;          
        }
        else if (cmd == 'm'){
            cin >> number >> key >> last;
            for(i = key; i < last; i++){
                db_insert(number, i, msg);
            }  
            printf("massive insertion from %ld to %ld finished\n", key, last);
        }
        else if (cmd == 'f'){
            cin >> number >> key;
            input_status = db_find(number, key, buf);
            if(input_status) printf("find %ld failed. Key doesn't exist in the tree(ID: %d).\n", key, number);
            else printf("find %ld at %d success. value: %s\n", key, number, buf);
        }
        else if (cmd == 'd'){
            cin >> number >> key;
            input_status = db_delete(number, key);
            if(input_status) printf("delete %ld failed. Key doesn't exist in the tree(ID: %d).\n", key, number);
            else printf("delete %ld success.\n", key);
        }
        else if (cmd == 'j'){
            cin >> number >> table_id >> path;
            input_status = join_table(number, table_id, path);
            if(input_status) printf("join table ID: %d and %d failed.\n", number, table_id);
            else {
                printf("join table ID: %d and %d success.\n", number, table_id);
                cout << "result is stored at " << path << "." << endl;
            }
        }
        else if (cmd == 'l'){
            cin >> number;
            print_leaves(number);
        }
        else if (cmd == 'p'){
            cin >> number;
            print_tree(number);
        }
        else if (cmd == 'b'){
            buffer_print_all();
        }
        else if (cmd == 'n'){
            cin >> number >> temp;
            print_node_from_disk(temp, tables.fd[number]);
        }
        else if (cmd == 's'){
            print_all_tables();
            shutdown_db();
            cout << "Buffer manager destroyed." << endl;
            cout << "Terminating the program." << endl;
            break;
        }
        else if (cmd == 'q'){
            cout << "You are terminating the DB without flushing data into disk!" << endl;
            break;
        }
        else{
            continue;
        }  
    }
    return 0;
}

