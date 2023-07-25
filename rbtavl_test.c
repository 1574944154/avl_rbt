#include <stdio.h>
#include <stdlib.h>
#include <dds/dds.h>

#include "rbtreeavl.h"

struct domain {
    ddsrt_rbt_node_t node;
    int key;
};

#define MAX_SAMPLES 10

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

char filename[] = "/home/dev/code/cppProject/nums.txt";
void *samples[MAX_SAMPLES];

int main(int argc, char *argv[])
{
    ddsrt_rbt_tree_t tree_root;
    ddsrt_rbt_init(&treedef, &tree_root);

    dds_time_t starttime, endtime, sumtime=0;

    FILE *f = fopen(filename, "r");
    int num;

    printf("sizeof node is %ld\n", sizeof(struct domain));

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++) 
    {
        samples[i] = malloc(sizeof(struct domain));
        fscanf(f, "%d ", &num);
        ((struct domain *)samples[i])->key = num;
    }

    endtime = dds_time();

    printf("alloc %d time is %ld ms\n", MAX_SAMPLES, (endtime-starttime)/DDS_NSECS_IN_MSEC);

    fclose(f);

    sumtime += endtime-starttime;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        // printf("%d ", ((struct domain*)samples[i])->key);
        ddsrt_rbt_insert(&treedef, &tree_root, samples[i]);
        // printf("insert key %d, addr %lu\n", i, (unsigned long)d_ptr);
    }

    endtime = dds_time();

    printf("insert all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

    sumtime += endtime-starttime;

    ddsrt_rbt_node_t *node;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        samples[i] = ddsrt_rbt_lookup(&treedef, &tree_root, &i);
        // assert(samples[i] != NULL);
    }

    endtime = dds_time();

    printf("lookup all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

    sumtime += endtime-starttime;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        ddsrt_rbt_delete(&treedef, &tree_root, samples[MAX_SAMPLES-i-1]);
    }

    endtime = dds_time();

    sumtime += endtime-starttime;

    printf("delete all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

    printf("all time is %ld ms\n", sumtime/DDS_NSECS_IN_MSEC);

    // printf("rbtree's height is %d\n", height(tree_root.root));

    // printf("rbt count is %ld\n", tree_root.count);

    // ddsrt_rbt_walk(&treedef, &tree_root, walk_func, NULL);

    return 0;
}