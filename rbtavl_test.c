#include <stdio.h>
#include <stdlib.h>

#include "rbtreeavl.h"


struct domain {
    ddsrt_rbt_node_t node;
    int key;
};

int compare_int(const void *a, const void *b)
{
    int ra = *((int*)a), rb = *((int*)b);
    if(ra > rb) return 1;
    if(ra < rb) return -1;
    return 0;
}

void walk_func(void *node, void *arg)
{
    struct domain *d = node;
    (void)arg;

    printf("access key is %d\n", d->key);
}

ddsrt_rbt_treedef_t treedef = DDSRT_RBT_TREEDEF_INITIALIZER(offsetof(struct domain, node), offsetof(struct domain, key), compare_int, 0);


int main(int argc, char *argv[])
{
    ddsrt_rbt_tree_t tree_root;
    ddsrt_rbt_init(&treedef, &tree_root);

    struct domain *d_ptr;

    for(int i=0;i<5;i++) 
    {
        d_ptr = malloc(sizeof(struct domain));
        d_ptr->key = i;
        ddsrt_rbt_insert(&treedef, &tree_root, d_ptr);
    }

    ddsrt_rbt_walk(&treedef, &tree_root, walk_func, NULL);

    return 0;
}