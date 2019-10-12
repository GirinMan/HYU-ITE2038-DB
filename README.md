# ITE2038 Database Management System Wiki

## Project #2 - On-disk B+ tree 구현

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