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

char filename[] = "nums.txt";
void *samples[MAX_SAMPLES];

int main(int argc, char *argv[])
{
    ddsrt_avl_ctree_t tree_root;
    ddsrt_avl_cinit(&domaintree_def, &tree_root);

    printf("sizeof node is %ld\n", sizeof(struct domain));

    FILE *f = fopen(filename, "r");
    int num;

    dds_time_t starttime, endtime, sumtime=0;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        samples[i] = malloc(sizeof(struct domain));
        fscanf(f, "%d ", &num);
        ((struct domain *)samples[i])->key = num;
    }

    endtime = dds_time();

    sumtime += endtime-starttime;

    fclose(f);

    printf("alloc %d time is %ld ms\n", MAX_SAMPLES, (endtime-starttime)/DDS_NSECS_IN_MSEC);
    
    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        ddsrt_avl_cinsert(&domaintree_def, &tree_root, samples[i]);
    }

    endtime = dds_time();

    sumtime += endtime-starttime;

    printf("lookup all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);


    ddsrt_avl_node_t *node;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        samples[i] = ddsrt_avl_clookup(&domaintree_def, &tree_root, &i);
        assert(samples[i] != NULL);
    }
    
    endtime = dds_time();

    sumtime += endtime-starttime;

    printf("lookup all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);


    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        ddsrt_avl_cdelete(&domaintree_def, &tree_root, samples[i]);
    }

    endtime = dds_time();

    sumtime += endtime-starttime;

    printf("delete all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);


    printf("all time is %ld ms\n", sumtime/DDS_NSECS_IN_MSEC);
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
