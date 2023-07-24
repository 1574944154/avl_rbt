#include <stdio.h>
#include <stddef.h>
#include <dds/dds.h>
#include "avl.h"
#include "rbtree.h"

struct domain
{
    ddsrt_avl_node_t node;
    int key;
};

#define MAX_SAMPLES 10000000

int max(int a, int b)
{
    return a>b ? a:b;
}

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

int height(ddsrt_avl_node_t *node)
{
    if(!node) return 0;
    return max(height(node->cs[0]), height(node->cs[1])) + 1;
}

void *samples[MAX_SAMPLES];

int main(int argc, char *argv[])
{
    ddsrt_avl_ctree_t tree_root;
    ddsrt_avl_cinit(&domaintree_def, &tree_root);

    struct domain *d_ptr;

    printf("sizeof domain is %ld\n", sizeof(struct domain));

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        samples[i] = malloc(sizeof(struct domain));
        ((struct domain *)samples[i])->key = i;
        
        // printf("insert key %d, addr %lu\n", i, (unsigned long)d_ptr);
    }

    dds_time_t starttime, endtime;
    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        ddsrt_avl_cinsert(&domaintree_def, &tree_root, samples[i]);
    }

    endtime = dds_time();

    printf("time is %ld\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

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

    // printf("avl's height is %d\n", height(tree_root.t.root));

    return 0;
}
