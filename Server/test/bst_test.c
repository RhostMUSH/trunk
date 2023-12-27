#include "../hdrs/bst.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

// foo_struct is a dummy struct for acting as a data node and testing the bst
// logic.  It contains a string.
typedef struct {
	char *str;
} foo_struct;

// me is a global string set to 'bst_test' to be used in logging statements
char *me = "bst_test";


int compare_func(const void *a, const void *b);
void short_address(void *addr, char *buf);
void check_tree(BST *tree, char *fruitsAlphabetical[], char dump);
void dump_tree(BST *tree);
char *remove_from_array(char *arr[], int index, int len);
void remove_from_array_by_name(char **fruitsAlphabetical, char *str, int len);
void delete_from_tree(BST *tree, char *str);

#define arr_len(arr) (sizeof(arr) / sizeof(arr[0]))

int main() {
    BST *tree = bst_create(compare_func);
    assert(tree != NULL);

    // values to insert into the tree
    char* fruitsRandom[26] = {
        "Jackfruit", "Watermelon", "Apple", "Raspberry", "Fig",
        "Orange", "Kiwi", "Lemon", "Yellow passionfruit", "Elderberry",
        "Ugli fruit", "Honeydew", "Banana", "Xigua", "Grape",
        "Indian plum", "Cherry", "Strawberry", "Nectarine", "Voavanga",
        "Mango", "Zucchini", "Date", "Papaya", "Quince", "Tomato"
    };

    // the same values, but in alphabetical order.  used to check walking the tree in order.
    char* fruitsAlphabetical[26] = {
        "Apple", "Banana", "Cherry", "Date", "Elderberry",
        "Fig", "Grape", "Honeydew", "Indian plum", "Jackfruit",
        "Kiwi", "Lemon", "Mango", "Nectarine", "Orange",
        "Papaya", "Quince", "Raspberry", "Strawberry", "Tomato",
        "Ugli fruit", "Voavanga", "Watermelon", "Xigua",
        "Yellow passionfruit", "Zucchini"
    };

    // loop over fruitsRandom and insert each into the tree
    printf(":: building BST\n");
    for (int i = 0; i < 26; i++) {
        foo_struct *foo = malloc(sizeof(foo_struct));
        foo->str = fruitsRandom[i];
        bst_insert(tree, foo);
    }

    // dump_tree(tree);
    check_tree(tree, fruitsAlphabetical, 0);

    // remove element 5 from fruitsAlphabetical and the tree
    delete_from_tree(tree, remove_from_array(fruitsAlphabetical, 5,arr_len(fruitsAlphabetical)));

    // dump_tree(tree);
    check_tree(tree, fruitsAlphabetical, 0);

    // remove indexes 3, 12, and 20 from fruitsAlphabetical and the tree
    delete_from_tree(tree, remove_from_array(fruitsAlphabetical, 3,arr_len(fruitsAlphabetical)));
    delete_from_tree(tree, remove_from_array(fruitsAlphabetical, 12,arr_len(fruitsAlphabetical)));
    delete_from_tree(tree, remove_from_array(fruitsAlphabetical, 20,arr_len(fruitsAlphabetical)));

    check_tree(tree, fruitsAlphabetical, 0);

    // try deleting a node that doesn't exist
    printf(":: attempt to delete a node that doesn't exist\n");
    foo_struct foo;
    foo.str = "foo";
    void *old_data = bst_delete(tree, &foo, free);
    assert(old_data == NULL);

    // delete the root node
    printf(":: delete the root node\n");
    old_data = bst_delete(tree, tree->root->data, NULL);
    assert(old_data != NULL);
    remove_from_array_by_name(fruitsAlphabetical, ((foo_struct *)old_data)->str, arr_len(fruitsAlphabetical));
    free(old_data);

    // one more check of the tree for integrity
    check_tree(tree, fruitsAlphabetical, 0);

    // destroy the tree, using free() to free each foo_struct in the tree
    bst_destroy(tree, free);

    printf("\n%s: All tests passed successfully.\n", me);

    return 0;
}

// compare_func is a comparison function for btree logic.  It compares the
// string values in two foo_structs.
int compare_func(const void *a, const void *b) {
	foo_struct *fooA = (foo_struct *)a;
	foo_struct *fooB = (foo_struct *)b;

	return strcmp(fooA->str, fooB->str);
}

// short_address is a helper function for determining the last 8 characters of an address.
// it takes two arguments, a pointer address and a buffer to write the last 8 characters to.
// makes for less verbose logging
void short_address(void *addr, char *buf) {
    char tmpbuf[20];
    int len = 0;

    snprintf(tmpbuf, sizeof(tmpbuf), "%p", addr);
    while(tmpbuf[len] != '\0') {
        len++;
    }

    if (len > 8) {
        strncpy(buf, tmpbuf + len - 8, 8);
        buf[8] = 0;
    } else {
        strncpy(buf, tmpbuf, len);
        buf[len] = 0;
    }
}

// remove_from_array is a helper function for removing an element from an array.
// given an array and an index, it will remove the element at that index and
// shift the remaining elements down. the function will return the string it
// removed.
char *remove_from_array(char *arr[], int index, int len) {
    char *removed = arr[index];
    // shift everything down
    for (int i = index; i < len - 1; i++) {
        arr[i] = arr[i + 1];
    }
    return removed;
}

void remove_from_array_by_name(char **fruitsAlphabetical, char *str, int len) {
    int i = 0;
    while (strcmp(str, fruitsAlphabetical[i]) != 0) {
        i++;
    }
    remove_from_array(fruitsAlphabetical, i, len);
}

void delete_from_tree(BST *tree, char *str) {
    foo_struct foo;
    foo.str = str;

    printf(":: delete %s\n", foo.str);
    bst_delete(tree, &foo, free);
}

// check_tree walks the entire tree and compares the values to the supplied array.
// if dump is true, it will print out the entire tree as it goes.
void check_tree(BST *tree, char *fruitsAlphabetical[], char dump) {
    BSTNode *node;
    int i = 0;

    printf("\n:: CHECK_TREE\n");
    for (node = bst_next_node(tree, NULL);
         node;
         node = bst_next_node(tree, node))
    {
        if (dump) {
            char nodeAddr[9];
            char dataAddr[9];
            char strAddr[9];
            char leftAddr[9];
            char rightAddr[9];
            char parentAddr[9];
            short_address(node, nodeAddr);
            short_address(node->data, dataAddr);
            short_address(((foo_struct *)node->data)->str, strAddr);
            short_address(node->left, leftAddr);
            short_address(node->right, rightAddr);
            short_address(node->parent, parentAddr);
            char *str = ((foo_struct *)node->data)->str;
            printf("::\tnode: %8s, node->data: %8s, node->data->str: %-8.8s, node->left: %8s, node->right: %8s, node->parent: %8s\n", nodeAddr, dataAddr, str, leftAddr, rightAddr, parentAddr);
        }
        // printf(":: check_tree: comparing node value %s to %s\n", ((foo_struct *)node->data)->str, fruitsAlphabetical[i]);
        assert(strcmp(((foo_struct *)node->data)->str, fruitsAlphabetical[i]) == 0);
        i++;
    }
    printf("\n");
}

void dump_tree(BST *tree) {
    BSTNode *node;

    printf("\n:: DUMP_TREE\n");
    for (node = bst_next_node(tree, NULL);
         node;
         node = bst_next_node(tree, node))
    {
        char nodeAddr[9];
        char dataAddr[9];
        char strAddr[9];
        char leftAddr[9];
        char rightAddr[9];
        char parentAddr[9];
        short_address(node, nodeAddr);
        short_address(node->data, dataAddr);
        short_address(((foo_struct *)node->data)->str, strAddr);
        short_address(node->left, leftAddr);
        short_address(node->right, rightAddr);
        short_address(node->parent, parentAddr);
        char *str = ((foo_struct *)node->data)->str;
        printf("::\tnode: %8s, node->data: %8s, node->data->str: %-8.8s, node->left: %8s, node->right: %8s, node->parent: %8s\n", nodeAddr, dataAddr, str, leftAddr, rightAddr, parentAddr);
    }
    printf("\n");
}
