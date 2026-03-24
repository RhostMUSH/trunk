#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bst.h"
/*
* Simple implementation of a binary search tree (BST)
*
* This implementation can store nodes of any type.
*
* The user must provide a comparison function to compare two nodes.
* The comparison function should return:
*   a negative number if the first node is less than the second node
*   a positive number if the first node is greater than the second node
*   0 if the two nodes are equal
*
* This implementation does not handle duplicates. More complex use cases may
* require additional consideration.
*
* PRIMARY FUNCTIONS:
*
* bst_create()      // Create a new BST
* bst_insert()      // Insert a new node into the BST
* bst_next_node()   // Get the next node in the in-order sequence
* bst_search()      // Search for a node in the BST, returning the node if found
* bst_delete()      // Delete a node from the BST
* bst_destroy()     // Destroy the BST
*
* EXAMPLES:
*
* bst_create(): Creates a new BST using the provided comparison function.
*   int compare(const void *a, const void *b) {
*       return (*(int*)a - *(int*)b);
*   }
*   BST* tree_of_ints = bst_create(compare);
*
* bst_insert(): Inserts a new node into the BST.
*   int value = 7;
*   bst_insert(tree_of_ints, &value);
*
* bst_next_node(): Gets the next node in the in-order sequence.
*
*   int x = 11;
*   int y = 13;
*   int x = 7;
*   bst_insert(tree_of_ints, &x);
*   bst_insert(tree_of_ints, &y);
*   bst_insert(tree_of_ints, &z);
*   BSTNode *node = bst_next_node(tree_of_ints, NULL);
*   printf("value %d\n", *(int*)node->data);    // "value 7"
*   node = bst_next_node(tree_of_ints, node);
*   printf("value %d\n", *(int*)node->data);    // "value 11"
*   node = bst_next_node(tree_of_ints, node);
*   printf("value %d\n", *(int*)node->data);    // "value 13"
*
* bst_search(): Searches for a node in the BST.
*
*   BSTNode *node = bst_search(tree_of_ints, &value);
*   printf("value %d\n", *(int*)node->data);    // "value 7"
*
*
* See test/bst_test.c for more examples.
*/

// local helper function declarations
static BSTNode *_create_bst_node(void *data);
static void _bst_destroy(BSTNode* node, void (*free_func)(void *));

// bst_create creates a new binary search tree.  The supplied comparison
// function will be used for inserts, deletes, and searches.
BST *bst_create(int (*comp_func)(const void *, const void *)) {
    if (!comp_func) { return NULL; }

    BST *tree = malloc(sizeof(BST));
    if (!tree) { return NULL; }

    tree->root = NULL;
    tree->comp_func = comp_func;
    return tree;
}

// bst_insert inserts a new node into the BST
void bst_insert(BST *tree, void *data) {
    if (!tree || !tree->comp_func || !data) {
        return;
    }

    BSTNode *node = _create_bst_node(data);
    if (!node) { return; };

    if (tree->root == NULL) {
        tree->root = node;
        return;
    }

    BSTNode *current = tree->root;
    BSTNode *parent = NULL;

    while (current != NULL) {
        parent = current;
        if (tree->comp_func(data, current->data) < 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    node->parent = parent;

    if (!parent) {
        // should never happen, but just in case
        return;
    }

    if (tree->comp_func(data, parent->data) < 0) {
        parent->left = node;
    } else {
        parent->right = node;
    }
}

// bst_search searches for a node in the BST
BSTNode *bst_search(BST* tree, void* data) {
    int comparison;

    if (!tree || !tree->comp_func || !tree->root || !data) {
        return NULL;
    }

    BSTNode *current = tree->root;
    while (current != NULL && (comparison = tree->comp_func(data, current->data)) != 0) {
        if (comparison < 0) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    return current ? current->data : NULL;
}

// bst_delete looks for a node based on the given data and deletes it.
// If the optional free_func is provided, it will be called on the data of the
// freed node and bst_delete will return NULL.
// If the optional free_func is is NULL, bst_delete will return a pointer to
// the data that was in the deleted node.
void *bst_delete(BST *tree, void *data, void (*free_func)(void *)) {
    if (!tree || !tree->root) {
        return NULL;
    }

    BSTNode *current = tree->root;
    BSTNode *parent = NULL;
    int cmp;

    // Find the node to delete
    while (current != NULL && (cmp = tree->comp_func(data, current->data)) != 0) {
        parent = current;
        current = cmp < 0 ? current->left : current->right;
    }

    if (current == NULL) {
        // Node not found
        return NULL;
    }

    // save this so we can return to the calling function
    void *deleted_data = current->data;

    if (current->left != NULL && current->right != NULL) {
        // Node with two children
        BSTNode *successor = current->right;
        BSTNode *successor_parent = current;

        // Find the in-order successor (leftmost child of right subtree)
        while (successor->left != NULL) {
            successor_parent = successor;
            successor = successor->left;
        }

        // Swap data
        current->data = successor->data;
        current = successor; // Now delete the successor
        parent = successor_parent;
    }

    // Now current has at most one child
    BSTNode *child = (current->left != NULL) ? current->left : current->right;

    if (parent == NULL) {
        // Deleting the root node
        tree->root = child;
    } else if (parent->left == current) {
        parent->left = child;
    } else {
        parent->right = child;
    }

    if (child != NULL) {
        child->parent = parent;
    }

    if (free_func && deleted_data) {
        free_func(deleted_data);
        deleted_data = NULL;
    }
    free(current);

    return deleted_data;
}

// bst_destroy takes a BST * and destroys the entire BST via _bst_destroy.
// the optional 'free_func' is called on each node's data before the node is freed.
void bst_destroy(BST *tree, void (*free_func)(void *)) {
    if (!tree) {
        return;
    }
    _bst_destroy(tree->root, free_func);
    free(tree);
}

// bst_next returns the next node in the in-order sequence
BSTNode* bst_next_node(BST *bst, BSTNode *node)
{
    if (!bst) {
        return NULL;
    }

    if(!node)
    {
        // return the leftmost/smallest node
        node = bst->root;
        while(node && node->left) { node = node->left; }
        return node;
    }

    // Finding the next node in the in-order sequence: the successor
    if(node->right)
    {
        node = node->right;
        while(node && node->left) { node = node->left; }
        return node;
    }

    BSTNode* y = node->parent;
    while (y && node == y->right)
    {
        node = y;
        if (node) {
            y = node->parent;
        }
    }
    return y;
}


/*
* Helper Functions
*/

// Helper function to create a new BST node
static BSTNode *_create_bst_node(void *data) {
    BSTNode *node = malloc(sizeof(BSTNode));
    if (!node) { return NULL; };

    memset(node, 0, sizeof(BSTNode));
    node->data = data;
    node->left = NULL;
    node->right = NULL;

    return node;
}

// bst_destroy destroys the entire BST
static void _bst_destroy(BSTNode* node, void (*free_func)(void *)) {
    if(node == NULL) {
        // Base case
        return;
    }

    // first delete subtrees (children)
    _bst_destroy(node->left, free_func);
    _bst_destroy(node->right, free_func);

    if (free_func) {
        free_func(node->data);
    }

    // then delete the node
    free(node);
}
