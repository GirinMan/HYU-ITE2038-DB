.SUFFIXES: .c .o

CC=gcc

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.c
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)diskmanage.c $(SRCDIR)buffer.c $(SRCDIR)bpt_insert.c $(SRCDIR)bpt_delete.c $(SRCDIR)bpt_utils.c 
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.c=.o)

CFLAGS+= -g -fPIC -I $(INC)

TARGET=main

all: diskmanage buffer bpt $(TARGET)

diskmanage:
	$(CC) $(CFLAGS) -o $(SRCDIR)diskmanage.o -c $(SRCDIR)diskmanage.c

buffer:
	$(CC) $(CFLAGS) -o $(SRCDIR)buffer.o -c $(SRCDIR)buffer.c

bpt:
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt_insert.o -c $(SRCDIR)bpt_insert.c
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt_delete.o -c $(SRCDIR)bpt_delete.c
	$(CC) $(CFLAGS) -o $(SRCDIR)bpt_utils.o -c $(SRCDIR)bpt_utils.c

$(TARGET): $(TARGET_OBJ)
	make static_library
	$(CC) $(CFLAGS) -o $@ $^ -L $(LIBS) -lbpt

clean:
	rm $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)libbpt.a

library:
	gcc -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)
