#ifndef _BST_H
#define _BST_H

typedef struct BSTNode {
    void *data;
    struct BSTNode *left;
    struct BSTNode *right;
    struct BSTNode *parent;
} BSTNode;

typedef struct {
    BSTNode *root;
    int (*comp_func)(const void *, const void *);
} BST;

BST *bst_create(int (*comp_func)(const void *, const void *));
void bst_insert(BST *tree, void *data);

void    *bst_delete(BST* tree, void* data, void (*free_func)(void *));
BSTNode *bst_search(BST* tree, void* data);
BSTNode *bst_next_node(BST *tree, BSTNode *node);
void     bst_destroy(BST *tree, void (*free_func)(void *));

#endif