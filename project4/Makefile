.SUFFIXES: .cpp .o

CC=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.cpp)
# OBJS:=$(SRCS:.cpp=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)diskmanage.cpp $(SRCDIR)buffer.cpp $(SRCDIR)bpt_insert.cpp $(SRCDIR)bpt_delete.cpp $(SRCDIR)bpt_utils.cpp $(SRCDIR)join.cpp 
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

CFLAGS+= -g -fPIC -I $(INC) -std=c++14

TARGET=main

all: diskmanage buffer bpt join m $(TARGET)

diskmanage:
	$(CC) $(CFLAGS) -o $(SRCDIR)diskmanage.o -c $(SRCDIR)diskmanage.cpp

buffer:
	$(CC) $(CFLAGS) -o $(SRCDIR)buffer.o -c $(SRCDIR)buffer.cpp

bpt:
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt_insert.o -c $(SRCDIR)bpt_insert.cpp
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt_delete.o -c $(SRCDIR)bpt_delete.cpp
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt_utils.o -c $(SRCDIR)bpt_utils.cpp

join:
	$(CC) $(CFLAGS) -o $(SRCDIR)join.o -c $(SRCDIR)join.cpp

m:
	$(CC) $(CFLAGS) -c -o $(SRCDIR)main.o -c $(SRCDIR)main.cpp

$(TARGET): $(TARGET_OBJ)
	make static_library
	$(CC) $(CFLAGS) -o $@ $^ -L $(LIBS) -lbpt

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)libbpt.a

library:
	gcc -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)
