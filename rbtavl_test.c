#include <stdio.h>
#include <stdlib.h>
#include <dds/dds.h>
#include "rbtreeavl.h"


struct domain {
    ddsrt_rbt_node_t node;
    int key;
};

#define MAX_SAMPLES 10000000

int max(int a, int b)
{
    return a>b ? a:b;
}

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

int height(ddsrt_rbt_node_t *node)
{
    if(!node) return 0;
    return max(height(node->cs[0]), height(node->cs[1])) + 1;
}

ddsrt_rbt_treedef_t treedef = DDSRT_RBT_TREEDEF_INITIALIZER(offsetof(struct domain, node), offsetof(struct domain, key), compare_int, 0);

void *samples[MAX_SAMPLES];

int main(int argc, char *argv[])
{
    ddsrt_rbt_tree_t tree_root;
    ddsrt_rbt_init(&treedef, &tree_root);

    for(int i=0;i<MAX_SAMPLES;i++) 
    {
        samples[i] = malloc(sizeof(struct domain));
        ((struct domain *)samples[i])->key = i;
    }

    dds_time_t starttime, endtime;
    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        ddsrt_rbt_insert(&treedef, &tree_root, samples[i]);
        // printf("insert key %d, addr %lu\n", i, (unsigned long)d_ptr);
    }

    endtime = dds_time();

    printf("time is %ld\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

    // printf("rbtree's height is %d\n", height(tree_root.root));

    // printf("rbt count is %ld\n", tree_root.count);

    // ddsrt_rbt_walk(&treedef, &tree_root, walk_func, NULL);

    return 0;
}