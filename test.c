#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

#include "rbtreeavl.h"

#define MAX_SAMPLES 2

int compare_int(const void *a, const void *b)
{
    int ra = *((int *)a), rb = *((int *)b);
    if(ra > rb) return 1;
    if(ra < rb) return -1;
    return 0;
}

struct domain
{
    ddsrt_rbt_node_t node;
    int key;
};

void augment_pt(void *node, const void *left, const void *right)
{    
    // printf("node %d, left %d, right %d\n", ((struct domain*)node)->key, ((struct domain*)left)->key, ((struct domain*)right)->key);
    printf("node is %d\n", ((struct domain*)node)->key);
}

void *samples[MAX_SAMPLES];

int main(int argc, char *argv[])
{
    ddsrt_rbt_treedef_t treedef = DDSRT_RBT_TREEDEF_INITIALIZER(offsetof(struct domain, node), offsetof(struct domain, key), compare_int, 0);
    ddsrt_rbt_tree_t tree;
    ddsrt_rbt_init(&treedef, &tree);

    struct domain *d_ptr;

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        samples[i] = d_ptr = malloc(sizeof(struct domain));
        d_ptr->key = i;
        ddsrt_rbt_insert(&treedef, &tree, d_ptr);
    }

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        ddsrt_rbt_delete(&treedef, &tree, samples[MAX_SAMPLES-i-1]);
    }



    return 0;
}




















// char filename[] = "nums.txt";

// int main(int argc, char *argv[])
// {
//     FILE *f = fopen(filename, "r");
//     int num, i=0;

//     while(fscanf(f, "%d", &num)!=EOF) 
//     {
//         printf("%d ", num);
//         i ++;
//     }
//     printf("nums size is %d\n", i);

//     return 0;
// }

