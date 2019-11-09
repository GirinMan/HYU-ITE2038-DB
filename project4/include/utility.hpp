#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include <iostream>
#include <string>
#include <fstream>
#include <map>

#define PAGE_SIZE 4096
#define DEFAULT_NEW_PAGE_COUNT 128
#define HEADER_PAGE_NUMBER 0

#define SUCCESS 0
#define FAILURE -1

#define NO_MORE_FREE_PAGE 0
#define NO_ROOT_NODE 0
#define NO_PARENT 0

#define RIGHTMOST_LEAF 0
#define LEFTMOST_LEAF -1

#define KEY_DO_NOT_EXISTS 0
#define KEY_ALREADY_EXISTS 1

#define MAX_TABLE_NUMBER 10

#define LEAF_ORDER 32
#define INTERNAL_ORDER 249

#define COMMA ','

#define LEFT 0
#define RIGHT 1

#define PAGE_OFFSET(pagenum) (pagenum) * PAGE_SIZE
#define PAGE_ADDRESS(page) (Page_t *)&(page)
#define PAGE_T(page) *(Page_t *)&page
#define PAGE_CONTENTS(block) block->frame.node_page

#define INTERNAL_VAL(node, i) i ? node.in_record[i - 1].page_num : node.extra_page_num

using namespace std;

typedef uint64_t Pagenum_t;
typedef uint64_t offset_t;
typedef int64_t keyval_t;

#endif /* __UTILITY_H__ */