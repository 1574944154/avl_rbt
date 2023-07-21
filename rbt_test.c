#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rbtree.h"


struct domain 
{
    struct rb_node node;
    int key;
};

struct domain *my_search(struct rb_root *root, int key)
{
    struct rb_node *node = root->rb_node;

    while(node)
    {
        struct domain *data = container_of(node, struct domain, node);

        if(data->key > key) node = node->rb_left;
        else if(data->key < key) node = node->rb_right;
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
        if(data->key < this->key) new = &((*new)->rb_left);
        else if(data->key > this->key) new = &((*new)->rb_right);
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

    preorder(node->rb_left);
    printf("%d ", cnode->key);
    preorder(node->rb_right);
}

int main(int argc, char *argv[])
{
    struct rb_root tree_root = RB_ROOT;

    struct domain *d_ptr = NULL;

    void *samples[4];

    for(int i=0;i<=3;i++)
    {
        samples[i] = malloc(sizeof(struct domain));
        ((struct domain *)samples[i])->key = i;

        my_insert(&tree_root, samples[i]);
    }

    rb_erase(samples[0], &tree_root);

    return 0;
}