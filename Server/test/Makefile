CC=gcc
CFLAGS=-c -g

# add header directory of ../hdrs to include path
CFLAGS+=-I../hdrs

SRC_DIR=../src
HDR_DIR=../hdrs

# directory to drop object files
OBJ_DIR=obj

all: $(OBJ_DIR) bst_test

test: $(OBJ_DIR) bst_test
	./bst_test

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

#
# bst_test
#
bst_test: $(OBJ_DIR)/bst_test.o $(OBJ_DIR)/bst.o
	$(CC) $(OBJ_DIR)/bst_test.o $(OBJ_DIR)/bst.o -o bst_test

$(OBJ_DIR)/bst_test.o: bst_test.c $(HDR_DIR)/bst.h
	$(CC) $(CFLAGS) bst_test.c -o $(OBJ_DIR)/bst_test.o

$(OBJ_DIR)/bst.o: $(SRC_DIR)/bst.c $(HDR_DIR)/bst.h
	$(CC) $(CFLAGS) $(SRC_DIR)/bst.c -o $(OBJ_DIR)/bst.o

clean:
	rm -rf $(OBJ_DIR)/*.o bst_test