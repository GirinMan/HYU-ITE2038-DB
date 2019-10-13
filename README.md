# ITE2038 Database Management System Wiki

#### by 2018008904 Lee Seong Jin

<br> <br>

## [Project #2 - On-disk B+ tree 구현]

### _About the Project_
---
- Project #2: 2019년 정형수 교수님 데이터베이스시스템설계 두 번째 과제

- 이 프로젝트의 목표는 메모리 기반 B+ tree 구현체를 이용하여 insert, delete, split 그리고 merge 등의 다양한 operation이 일어나는 과정에 대해 분석한 후, 이를 바탕으로 Database Management System의 Disk level 관리를 해 줄 disk-based B+ tree를 개발하는 것이다.

<br>

### _Milestone 1: Analyzing the B+ tree code_
---
#### 1.1. Possible call path of the insert/delete operation
- **Insert operation**
    - Insert operation이 시행되기 위해서는 `insert(root, key, value)` 함수를 호출하고 return value를 root에 다시 대입해야 한다. insert 함수는 다음과 같이 상황에 따라 세부적인 작업을 하는 다른 함수들을 호출한 결과값을 return 한다.
    - B+ tree는 중복된 key 값을 허용하지 않으므로, root를 기반으로 하는 B+ tree 내에 key 값을 가지는 leaf node가 존재하는지 확인하기 위해 `find` 함수를 호출한다. 만약 `find` 함수의 return 값이 `NULL`이 아닐 경우 즉 key 값이 이미 tree 안에 존재할 경우 insert 함수는 `NULL`값을 return 한다.
    - Tree 안에 key 값이 존재하지 않는 경우, 새로운 key와 value를 insert해야 하므로 value가 담기기 위한 record를 `make_record` 함수를 호출하여 생성한다.
    - 만약 root 가 `NULL`즉 빈 tree일 경우 `start_new_tree` 함수를 호출하여 root 노드에 새로운 값을 담아 return 한다.
    - 만약 이미 tree가 존재할 경우, key 값이 들어갈 만한 적절한 leaf 노드를 `find_leaf` 함수를 이용해서 찾는다. 이때 노드에 기존 key가 `ORDER – 1` 개 미만 존재한다면, `insert_into_leaf` 함수를 호출하여 해당 노드에 record를 insert한다.
    - key 값이 들어갈 만한 leaf 노드에 더 이상 새로운 key가 insert 될 수 없는 경우, `insert_into_leaf_after_splitting` 함수를 호출하여 용량이 한계에 다다른 leaf 노드를 split 하는 과정을 거친 뒤 record를 insert한다.

    ---
    <br>
- **Delete operation**
    - Delete operation이 실행되기 위해서는 Insert operation과 유사하게 root 변수에 `delete(root, key)` 함수를 호출한 return 값을 대입하면 된다. `delete` 함수가 호출되면 우선 key 값을 포함하는 record와 leaf 노드를 각각 `find` 및 `find_leaf` 함수를 호출하여 찾는다. 
    - 이때 record 또는 `find_leaf`가 `NULL`인 경우는 tree 안에 해당 key 값이 존재하지 않는다는 뜻이므로 `delete`가 진행되지 않고 root를 그대로 다시 return 한다.
    - 만약 해당 record 와 노드가 둘 다 `NULL`이 아니라면, key 값이 tree 내에 존재하는 것이므로 해당 노드에 대해 `delete_entry` 함수를 호출한다.
    - `delete_entry` 함수에서는 `remove_entry_from_node` 함수를 호출하여 해당 노드에서 record를 제거한 다음 노드가 root일 경우 `adjust_root` 함수를 호출한 return 값을 return 하고, 그렇지 않은 경우 적절히 노드들을 재배치한다.

    ---
</br>

#### 1.2. Detail flow of the structure modification (split, merge)
주어진 B+ tree 소스 코드에서 새로운 record가 insert되거나 기존에 존재하던 record가 delete되는 경우, 일부 상황에서 B+ tree의 속성을 유지하기 위해 split 또는 merge 와 같은 structure modification이 발생하기도 한다. 이러한 modification이 발생하는 상황과 구체적인 과정은 다음과 같다.
<br><br>

- **Detailed process of node split during insertion**
    >어떤 key를 insert 해야하는 leaf 노드에 이미 `ORDER – 1` 개만큼의 key가 존재할 경우, 해당 노드에는 더 이상 새로운 key를 insert할 수 없으므로 새로운 노드를 생성하여 record를 분산하여 나누어 가지는 split operation이 일어난다.
    <br></br>
    >이 과정에서 기존 노드에 있던 record 중 중앙에 있던 record는 부모 노드에 insert된다. 만약 이때 부모 노드에 더 이상 새로운 record가 들어갈 자리가 없을 경우, 부모 노드에서도 split operation이 발생한다. 이는 더 이상 부모 노드가 split 되지 않을 때 까지 반복된다.

    - 새로운 record가 insert 되고 노드가 split 되는 과정에서 다음과 같은 함수들이 호출된다.
        - B+ tree에 새로운 record를 insert하는 과정에서 `insert` 함수가 호출되는데, 그 과정에서 `1.1`에서 표시되어있는 것과 같이 record가 insert되는 노드에 이미 `ORDER - 1`개의 record가 존재할 경우, `insert` 함수는 `insert_into_leaf_after_splitting` 을 호출한 return 값을 return하게 된다.
        - `insert_into_leaf_after_splitting`이 호출되면 우선 새로운 leaf 노드를 `make_leaf` 함수를 호출하여 만든 뒤, 기존에 순서대로 저장되어 있던 key와 value를 임시로 저장할 array를 만들어 이를 이용해 기존의 노드의 key의 절반을 새로운 노드에 저장하고 이를 부모 노드에 연결한다. 그 다음에 노드의 첫 번째 key를 부모에 삽입하는 `insert_into_parent`함수를 호출한다.
        - 중간에 존재하는 key 값에 대해 `insert_into_parent` 함수가 호출되면 left 노드의 부모가 NULL 인지 확인하여(split이 일어난 노드가 원래는 root 노드였다는 것을 의미한다) `insert_into_new_root` 함수 호출을 return한다.
        - 만약 left 노드가 root 노드가 아닌 leaf 또는 일반 노드일 경우, 부모 노드가 존재하므로 부모 노드에 존재하는 key 개수를 확인하여 `ORDER - 1` 보다 적은 경우 `insert_into_node` 함수를 호출하여 부모 노드에 key를 insert한다.
        - 만약 부모 노드에 더 이상 새로운 key가 insert될 자리가 없을 경우, `insert_into_node_after_splitting`을 호출하여 부모 노드에서 또 다시 split이 일어나도록 한다. 이 경우, `insert_into_leaf_after_splitting` 함수와 유사하게 기존의 노드에 포함되어 있던 record들을 임시로 저장할 array를 만들고, 새로운 노드를 생성한 다음 절반씩 나누어 record가 insert되도록 한 다음 중간의 key 하나를 부모에 insert하도록 `insert_into_parent` 함수를 다시 호출한다.
        - 위와 같은 과정은 더 이상 split 되지 않는 부모 노드를 만날 때 까지 계속해서 반복된다. 만약 root 노드에서 split이 일어날 경우, tree의 height가 변경될 수 있다.

<br>

- **Detailed process of node merge during deletion**
    > 어떤 key를 delete하는 과정에서 `delete` 함수가 호출되면 `1.1`에 나와있는 것과 같이 해당 노드에서 `delete_entry` 함수를 호출한다. 해당 함수 내부에서는 우선 `remove_entry_from_node` 를 호출하는데, 만약 deletion이 root가 아닌 노드에서 일어났는데 deletion이 일어난 뒤에 해당 노드에 남은 key 개수가 한 노드가 가져야 하는 최소 key 개수 미만일 경우, coalescence 또는 redistribution을 통해 B+ tree의 조건을 만족하도록 각 노드들을 수정해 주어야 한다.

    - 기존의 record가 delete 되었을 때 노드의 key의 개수가 부족해지면서 coalescence 또는 redistribution이 일어나는 과정에서 다음과 같은 함수들이 호출된다.
        - deletion 과정에서 노드의 재배치가 필요한 경우는 deletion이 일어난 노드가 root 노드가 아니면서 deletion이 일어난 이후 노드에 남아있는 key의 개수가 최소 개수보다 작은 경우이다.
        - delete 과정 중 `delete_entry` 함수에서 `remove_entry_from_node` 함수가 호출된 이후 해당 노드가 root가 아닐 경우 다음과 같은 방법을 통해 현재 노드 n에 존재해야 하는 최소 노드의 개수 `min_keys`를 구한다.

            ```c
            min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;
            ```

        - 만약 deletion이 일어난 노드에 남은 key의 개수가 min_keys 이상일 경우 그대로 deletion operation이 종료되지만, 그렇지 않은 경우는 `get_neighbor_index` 함수를 호출하고 이를 바탕으로 가장 가까운 이웃 leaf 노드인 neighbor 노드를 찾는다. 또한 deletion이 일어난 노드에 포함될 수 있는 key의 개수의 최대값 + 1인 `capacity`를 다음과 같이 구한다.

            ```c
            capacity = n->is_leaf ? order : order - 1;
            ```

        - neighbor 노드에 존재하는 key의 개수와 deletion이 일어난 노드 n에 존재하는 key의 개수의 합이 capacity보다 작은 경우, `coalesce_nodes` 함수를 호출하여 deletion이 일어난 노드와 이웃 노드를 merge하는 작업이 시행된다.
        - 반대로 두 노드에 존재하는 key의 개수의 합이 capacity 이상인 경우, merge가 불가능하므로 `redistribute_nodes` 함수를 호출하여 각 노드들에 포함되어 있는 record들을 적절히 재배치한다.

        ---
</br>

#### 1.3. Designs or required changes for building on-disk B+ tree(Naive)
주어진 B+ tree 소스 코드는 int 형식 key를 B+ tree의 규칙에 따라 저장할 수 있는 Memory-based B+ tree의 C 구현체이다.
해당 코드는 각 노드가 메모리 위에 존재하므로, 프로그램이 실행 중이 아닌 경우에는 데이터가 남아있지 않으며 최대 용량에 한계가 있다. 이를 DBMS에서 활용하기 위해서는 엄청나게 많은 key-value pair들을 disk level에서 직접 다룰 수 있도록 하는 disk-based B+ tree로 재구성해주어야 한다.

우선 B+ tree가 disk-based, 즉 하드디스크 내에 저장되어 있는 파일 위에 존재해야 하므로 B+ tree가 저장될 파일을 페이지 단위로 abstraction을 시켜주는 것이 필요하다. 각 페이지는 4KB의 용량을 가지며, 다음과 같은 네 가지 종류의 페이지들로 파일이 구성되도록 한다.

>    1. Header page: 다른 페이지들에 대한 설명이 포함되어 있다.
>    1. Free page: 현재 사용되고 있지 않은 페이지로, free page들은 리스트로 묶여서 관리된다.
>    1. Leaf page: 8B key + 120B value로 이루어진 record들이 저장되어 있는 페이지이다.
>    1. Internal page: 다른 internal page 및 leaf page들을 indexing한다.


Disk-based B+ tree는 파일의 첫 페이지인 Header page를 먼저 읽어 현재 파일에 존재하는 Free page number, Root page number, 그리고 Number of pages 등을 파악한 뒤 이를 바탕으로 나머지 페이지들을 로드하여 사용할 수 있다. Internal/Leaf 페이지들은 페이지의 맨앞 128 byte 부분을 페이지 헤더로 사용하며, 페이지 헤더에는 해당 페이지의 부모의 페이지 번호와 Leaf 페이지인지의 여부, 그리고 페이지에 저장된 key의 개수와 같은 정보가 포함되어 있어야 한다.

---

또한 memory 위에 저장되어 있는 노드 기반의 B+ 트리를 Disk-based tree로 만들기 위해 트리의 기본 단위인 노드의 구조를 수정할 필요가 있다.

Memory-based b+ tree의 노드는 다음과 같은 구조로 이루어져 있다:

```c
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next;
} node; 
```

각 노드는 `node *` 형식의 포인터 변수에 노드의 위치가 저장되고 이를 바탕으로 노드의 정보에 접근하는데, 메모리 위가 아닌 파일 내부에 저장되어 있는 노드에 접근하는 방법이 필요하다.

이를 위해 파일에 저장되어있는 각 페이지에 고유한 페이지 번호를 부여하여 이를 메모리 기반 시스템의 포인터와 같이 사용할 수 있을 것이다. 이를 위해선 특정 페이지 번호가 가리키는 페이지의 내용을 불러오거나, 수정하거나, 삭제하는 operation을 구현할 필요가 있다. 이와 같은 기능을 구현하기 위해, 파일 안에 존재하는 한 페이지를 읽어서 그 정보를 메모리 위에 임시로 저장할 수 있도록 하는 page 구조체와 함수들을 다음과 같이 정의할 수 있다:

```c
typedef uint64_t pagenum_t;
struct page_t {
// in-memory page structure
};

// Free page list에 있는 한 free page에 대해 allocation을 진행한다.
pagenum_t file_alloc_page();

// 페이지 번호에 해당되는 페이지를 free page로 전환한다.
void file_free_page(pagenum_t pagenum);

// On-disk 기반 페이지에 속해있는 정보를 In-memory 구조체에 불러온다.
void file_read_page(pagenum_t pagenum, page_t* dest);

// In-memory 구조체에 저장되어 있는 페이지 정보를 On-disk 기반 페이지에(실제 파일에) 저장한다.
void file_write_page(pagenum_t pagenum, const page_t* src);
```

위의 구조체 정의와 함수 정의를 바탕으로 다음 네 가지 operation을 재구성하여, On-memory 노드 기반 B+ tree를 On-disk 페이지 기반 B+ 트리로 변경할 수 있을 것이다.

>
>**1. open [pathname]** - [pahtname] 위치에 존재하는 파일을 열거나 존재하지 않을 경우 새로 생성한다. 아래 세 가지 operation 모두 open operation이 시행된 뒤에만 실행될 수 있다.
>
>**2. insert [key] [value]** - [key]와 [value]가 짝으로 이루어진 record를 On-disk tree에 insert한다. 같은 key가 동시에 insert될 수는 없다.
>
>**3. find [key]** - 입력된 [key] 값에 해당되는 value를 찾아 return한다.
>
>**4. delete [key]** - [key] 값에 대응하는 record를 찾고 만약 존재한다면 해당 record를 tree에서 delete한다.
>

<br>

### _Milestone 2: Details about implemeting On-disk B+ tree_
---

#### 2.1. Page structures

- 기존의 In-memory B+ tree와 다르게, On-disk B+ tree는 데이터가 저장되어 있는 파일을 4096바이트 단위로 나누어서 페이지로 관리한다.
- 따라서, 파일을 페이지 단위로 입력하고 출력하며 파일에 저장된 데이터를 메모리 위로 불러와 작업하는 것들이 모두 가능하도록 하는 In-memory structure들을 설계했다.
- 각 페이지는 용량은 모두 같으나 오프셋에 저장되는 데이터의 크기와 종류, 개수 등이 다르므로, 각각의 페이지(Header page, Free page, Leaf page, Internal page)에 해당하는 In-memory structure 들을 정의할 필요가 있었다.
- 그런데 Leaf와 Internal page들은 (key, value) record들을 저장하는 방식을 제외하면 모든 데이터의 오프셋과 크기 등이 같으므로, 두 페이지 형식을 표현할 수 있는 공통된 structure 하나를 정의하고 ```union``` 키워드와 ```boolean``` is_leaf를 활용하여 Leaf와 Internal을 구분하여 사용할 수 있도록 하였다.

각 구조체는 다음과 같이 정의되어 있다:

```c
typedef struct HeaderPage_t {

    // points the first free pages
    // 0 if there's no free page left
    Pagenum_t free_page_num; 

    // points the root page within the data file
    Pagenum_t root_page_num;

    // denote the number of pages existing in this data file now
    Pagenum_t num_page;

    // unused bytes of header page
    char reserved[4072];

} HeaderPage_t;
```

```c
typedef struct FreePage_t {

    // points the next free page
    // 0 if the page is the end of the free page list
    Pagenum_t next_free_page_num;

    // unused bytes of header page
    char reserved[4088];
    
} FreePage_t;
```

```c
typedef struct NodePage_t {

    // Page Header ------------------------------
    
    // points the position of parent page
    Pagenum_t parent_page_num;

    // indicate whether it is a leaf page or not
    int is_leaf;

    // denote the number of keys within this page
    int num_key; 

    // unused bytes of node pade
    char reserved[104];

    // ------------------------------------------

    union
    {
    // an extra page number for interpreting key range
    // pointing at the leftmost child
    Pagenum_t extra_page_num;

    // stores the page's right sibling page's page number
    // 0 if the page is a rightmost page
    Pagenum_t right_page_num;
    };

    union
    {
    // array storing records in the page
    // maximum 248 records per page(branching factor = 249)
    InternalRecord in_record[248];
       
    // array storing records in the page
    // maximum 31 records per page(branching factor = 32)
    LeafRecord lf_record[31];
    };

} NodePage_t;
```

```c
typedef union Page_t{
    HeaderPage_t header_page;
    FreePage_t free_page;
    NodePage_t node_page;
} Page_t;
```

<br>

#### 2.2.  File I/O functions

- 기존 In-memory B+ tree는 Node structure 하나를 메모리 위에 잡아두고 포인터를 활용하여 해당 노드에 입 출력을 하는 구조로 이루어져 있었다.
- 메모리가 아니라 디스크 위에 저장된 페이지로 노드를 대체하기 위해서는, 포인터 변수를 통해 노드에 접근하고 데이터를 읽고 썼던 것과 비슷한 페이지에 접근할 수 있는 방법이 필요하다.
- 따라서, 메모리의 특정 주소로 접근하는 방식인 포인터와 유사하게 특정 페이지 넘버를 입력하면 해당 페이지 넘버가 가리키는 페이지의 데이터를 읽거나 쓸 수 있도록 하는 ```file_read_page```와 ```file_write_page``` 함수를 설계했다.
- 기존의 tree가 ```malloc``` 함수를 호출하여 새로운 메모리 영역에 새로운 노드를 생성하고 ```free``` 함수를 이용하여 메모리에서 제거한 것 처럼, 현재 사용중이 아닌 free page list에서 한 페이지를 allocation해주는 ```file_alloc_page``` 함수와 페이지 하나를 free page list로 free 해주는 ```file_free_page``` 함수 역시 설계했다.
- 만약 ```file_alloc_page```가 호출되었는데 더 이상 free page가 남아있지 않은 경우, 미리 정해져있는 `DEFAULT_NEW_PAGE_COUNT` 만큼의 새로운 free 페이지를 생성한 후 페이지를 할당해 주어 새로운 패이지가 생성되는 시간을 줄이고자 했다.
- 또한 ```open_table``` 함수 내에서 사용할 수 있도록 입력된 path에 파일이 존재하며 읽고 쓰기가 가능할 경우 파일을 열고, 그렇지 않은 경우 파일을 새로 생성하는 ```file_open_if_exist``` 함수를 생성한다.
- 테이블 전체를 관리하는 Header page는 전역 변수 header로 관리하도록 했으며, In-memory header와 On-disk header의 데이터가 일치하도록 해주는 ```update_header``` 함수까지 설계했다.

각 함수 선언은 다음과 같다:

```c
#define PAGE_SIZE 4096
#define DEFAULT_NEW_PAGE_COUNT 128
#define HEADER_PAGE_NUMBER 0

struct HeaderPage_t header;

// Allocate an on-disk page from the free page list
Pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(Pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(Pagenum_t pagenum, Page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(Pagenum_t pagenum, const Page_t* src);

// Write some multiple pages stored in an array into the file
void file_write_multi_pages(Pagenum_t pagenum, const Page_t* src, const int num_page);

// If the file exists in given pathname, open it in fd and return 1
// If the file doesn't exist or has error, return 0
int file_open_if_exist(const char * pathname);

// Update information of both in-memory and on-disk header pages with given values
void update_header(Pagenum_t free_page_num, Pagenum_t root_page_num, Pagenum_t num_page);
```

<br>

#### 2.3.  Implementing B+ tree operations

- 기존 In-memory B+ tree를 operate하는 함수 정의들을 바탕으로, 새로운 On-disk page가 노드로 사용되는 B+ tree와 다음 함수들을 구현했다.
- 기존 함수들이 Node structure를 가리키는 포인터 주소를 return하는 경우나 paramater에 요구하는 경우, Page number로 대체했다. 포인터 주소로 메모리의 노드에 접근하는 것 처럼, File I/O를 이용해 특정 Page number가 가리키는 페이지에 접근할 수 있기 때문이다.
- 기존 함수들에서 노드 포인터 변수를 선언하고 해당 변수를 통해서 노드의 데이터 입 출력을 하는 경우 ```Pagenum_t``` 형식 변수와 ```NodePage_t``` 형식 변수를 하나씩 선언하여 포인터 변수의 활용을 대체했다.
- 또한 새로운 메모리 영역을 할당하는 ```malloc``` 부분은 ```file_alloc_page```로 대체하였고, ```free``` 부분은 ```file_free_page```로 대체하였다.
- ```file_read_page``` 함수를 호출하여 페이지의 데이터를 읽고 수정한 후에는 반드시 실제 디스크 위의 파일과 일치하도록 변경된 내용을 ```file_write_page``` 함수를 호출하여 파일에 쓰도록 했다.
- 특정 경로를 바탕으로 파일을 여는 ```open_table``` 함수는 우선 ```file_open_if_exist```로 파일이 이미 존재하는지 확인하여 만약 존재할 경우 헤더페이지의 내용을 In-memory header에 읽어온 뒤 테이블 ID를 return한다.
- 만약 파일이 존재하지 않을 경우 새로운 파일을 생성한 뒤 헤더 페이지의 루트 노드를 0, 페이지 개수를 1, 그리고 free page list의 첫 번째 페이지 넘버를 0 즉 헤더페이지 외에는 아무 페이지도 없는 상태로 초기화 시킨다.

각 함수 선언은 다음과 같다:

```c
typedef Keyval_t int64_t

/* *Open existing data file using ‘pathname’ or create one if not existed.
If success, return the unique table id, which represents the own table in this database. Otherwise, return negative value. (This table id will be used for future assignment.) */
int open_table (char * pathname);

/*Insert input ‘key/value’ (record) to data file at the right place.
If success, return 0. Otherwise, return non-zero value. */
int db_insert (Keyval_t key, char * value);

/* Find the record containing input ‘key’.
 If found matching ‘key’, store matched ‘value’ string in ret_val and return 0. Otherwise, return non-zero value.
Memory allocation for record structure(ret_val) should occur in caller function. */
int db_find (Keyval_t key, char * ret_val);

/* Find the matching record and delete it if found.
If success, return 0. Otherwise, return non-zero value. */
int db_delete (Keyval_t key);
```

<br>

#### 2.4. Delayed merge

- 기존의 ```delete``` 함수를 호출하면 우선 paramater로 들어간 key 값이 tree 내부에 존재하는지 찾은 다음, 해당 key가 존재하는 leaf 노드를 찾아 key를 지우고 key에 해당되는 record들을 B+ tree의 성질이 유지되도록 재배치한다.
- key가 어떤 노드에서 삭제되고 난 뒤에, 해당 함수는 현재 노드에 남아있는 키의 개수가 min_key 보다 크거나 같은지 확인하는데, 만약 min_key 이상의 key를 가지고 있을 경우 아무 작업도 실시하지 않아 tree의 형태에 변형이 없다.
- 그런데 만약 현재 노드에서 key 1개를 삭제하고 난 뒤 남은 key의 개수가 min_key 미만이라면, tree의 각 key들을 재배치하여 더 효율적인 구조가 되도록 한다.
- 이웃한 노드와 병합이 가능하면 ```coalesce_nodes```, 불가능할 경우 이웃에게 key를 하나 가져오는 ```redistribute_nodes```를 호출한다.
- 기본적으로 min_key는 해당 노드가 가지고 있을 수 있는 key의 개수의 절반으로 설정되어 있다.

```c
min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

/* Case:  node stays at or above minimum.
 * (The simple case.)
 */

if (n->num_keys >= min_keys)
    return root;

```

```c

/* Case:  node falls below minimum.
 * Either coalescence or redistribution
* is needed.
*/

/* Coalescence. */

if (neighbor->num_keys + n->num_keys < capacity)
    return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

/* Redistribution. */

else
    return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);

```

- 그런데 On-disk B+ tree에서 매번 key가 절반 이하로 줄어들 때 마다 페이지 병합이나 키 교환등을 진행하기에는 디스크 I/O에 드는 비용이 너무 크기 때문에, 이러한 operation들이 진행되기 위한 조건을 바꿀 필요가 있다.
- 만약 현재 페이지에서 key deletion이 일어난 이후로 단 한개의 key라도 존재한다면, merge operation이 발생하는 것을 최소화하기 위해 아무런 작업을 진행하지 않는다.
- 단 한 개의 key만 남아있는 노드에서 deletion이 일어날 경우에만, 즉 해당 노드가 empty 되었을 경우에만 coalesce 또는 redistritubte operation을 실행하여 disk I/O에 드는 비용을 최소화할 수 있다.

이를 위해서는 기존의 코드에서 다음과 같이 min_key를 1로 설정하면 된다. 그러면 삭제 이후에 해당 노드에 key가 전혀 존재하지 않는 경우를 제외하면 key의 재배치가 일어나지 않게 된다.

그런데 Internal page의 capacity는 Internal order - 1 이지만 Leaf page의 capacity는 Leaf order 이므로, 한 리프 노드에서 모든 key가 삭제된 경우에는 이웃한 리프 노드의 키가 최대치여도 두 리프 노드의 key 개수의 합이 capacity 보다 작다. 

따라서, Leaf page에서는 redistribute operation이 일어나지 않는다. 오직 Internal Page에서만 redistribute operation이 일어날 수 있다.

```c
min_keys = 1;
```
