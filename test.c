#include <stdio.h>
#include <stddef.h>

#include "avl.h"

int compare_int(const void *a, const void *b)
{
    int ra = *((int *)a), rb = *((int *)b);
    if(ra > rb) return 1;
    if(ra < rb) return -1;
    return 0;
}

struct domain
{
    ddsrt_avl_node_t node;
    int key;
};

void augment_pt(void *node, const void *left, const void *right)
{    
    // printf("node %d, left %d, right %d\n", ((struct domain*)node)->key, ((struct domain*)left)->key, ((struct domain*)right)->key);
    printf("node is %d\n", ((struct domain*)node)->key);
}

int main(int argc, char *argv[])
{
    ddsrt_avl_ctreedef_t treedef = DDSRT_AVL_CTREEDEF_INITIALIZER(offsetof(struct domain, node), offsetof(struct domain, key), compare_int, augment_pt);
    ddsrt_avl_ctree_t tree;
    ddsrt_avl_cinit(&treedef, &tree);

    struct domain *d_ptr;

    for(int i=1;i<=100;i++)
    {
        d_ptr = malloc(sizeof(struct domain));
        d_ptr->key = i;
        ddsrt_avl_cinsert(&treedef, &tree, d_ptr);
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

