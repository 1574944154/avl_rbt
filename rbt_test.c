#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rbtree.h"


struct domain 
{
    struct rb_node node;
    int key;
};

int max(int a, int b)
{
    return a>b ? a:b;
}

struct domain *my_search(struct rb_root *root, int key)
{
    struct rb_node *node = root->rb_node;

    while(node)
    {
        struct domain *data = container_of(node, struct domain, node);

        if(data->key > key) node = node->cs[0];
        else if(data->key < key) node = node->cs[1];
        else return data;
        
    }
    return NULL;
}

int my_insert(struct rb_root *root, struct domain *data)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    while(*new)
    {
        struct domain *this = container_of(*new, struct domain, node);

        parent = *new;
        if(data->key < this->key) new = &((*new)->cs[0]);
        else if(data->key > this->key) new = &((*new)->cs[1]);
        else return false;
    }

    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);
    return true;
}

void preorder(struct rb_node *node)
{
    if(!node) return;

    struct domain *cnode = container_of(node, struct domain, node);

    preorder(node->cs[0]);
    printf("%d ", cnode->key);
    preorder(node->cs[1]);
}

int height(struct rb_node *node)
{
    if(!node) return 0;
    return max(height(node->cs[0]), height(node->cs[1])) + 1;
}

int main(int argc, char *argv[])
{
    struct rb_root tree_root = RB_ROOT;

    struct domain *d_ptr = NULL;

    printf("sizeof domain is %ld\n", sizeof(struct domain));

    for(int i=0;i<=300000;i++)
    {
        d_ptr = malloc(sizeof(struct domain));
        d_ptr->key = i;

        my_insert(&tree_root, d_ptr);
    }

    printf("rbtree's height is %d\n", height(tree_root.rb_node));
    // rb_erase(samples[0], &tree_root);

    return 0;
}