#include <stdio.h>
#include <stddef.h>

#include "avl.h"
#include "rbtree.h"

struct domain
{
    ddsrt_avl_node_t node;
    int key;
};

int compare_int(const void *a, const void *b)
{
    int ra = *((int *)a), rb = *((int *)b);
    if(ra > rb) return 1;
    if(ra < rb) return -1;
    return 0;
}

// ddsrt_avl_ctreedef_t domaintree_def = 
//                 DDSRT_AVL_CTREEDEF_INITIALIZER (offsetof(struct domain, node), offsetof(struct domain, key), compare_int, NULL);
ddsrt_avl_ctreedef_t domaintree_def = 
                   DDSRT_AVL_CTREEDEF_INITIALIZER_ALLOWDUPS (offsetof(struct domain, node), offsetof(struct domain, key), compare_int, 0);

void walk_func (void *node, void *arg)
{
    struct domain *d = node;
    (void)arg;

    printf("access key is %d \n", d->key);
}



int main(int argc, char *argv[])
{
    ddsrt_avl_ctree_t tree_root;
    ddsrt_avl_cinit(&domaintree_def, &tree_root);

    void *samples[5];
    
    struct domain *d_ptr;


    for(int i=0;i<=10000000;i++)
    {
        d_ptr = malloc(sizeof(struct domain));
        d_ptr->key = i;
        ddsrt_avl_cinsert(&domaintree_def, &tree_root, d_ptr);
        // printf("insert key %d, addr %lu\n", i, (unsigned long)d_ptr);
    }

    // ddsrt_avl_cwalk(&domaintree_def, &tree_root, walk_func, NULL);
    // int l = 100, r = 110;
    // printf("walk range %d to %d\n", l, r);
    // ddsrt_avl_cwalk_range(&domaintree_def, &tree_root, &l, &r, walk_func, NULL);
    // ddsrt_avl_cwalk(&domaintree_def, &tree_root, walk_func, NULL);

    // ddsrt_avl_cdelete(&domaintree_def, &tree_root, samples[3]);

    // pretraverse(&domaintree_def.t, tree_root.t.root, walk_func);
    // printf("\n");
    // intraverse(&domaintree_def.t, tree_root.t.root, walk_func);
    // printf("\n");

    return 0;
}
