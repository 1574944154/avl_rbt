
#include <stddef.h>

#include "rbtree.h"
#include "rbtreeavl.h"


struct domain {
    ddsrt_rbt_node_t node_new;
    struct rb_node node_old;
    int key;
};

static int compare_int(const void *a, const void *b)
{
    int ra = *((int*)a), rb = *((int*)b);
    if(ra > rb) return 1;
    if(ra < rb) return -1;
    return 0;
}

int main(int argc, char *argv[])
{
    struct rb_root oldtree = RB_ROOT;

    ddsrt_rbt_treedef_t treedef = DDSRT_RBT_TREEDEF_INITIALIZER(offsetof(struct domain, node_new), offsetof(struct domain, key), compare_int, 0);

    struct domain *d_ptr;

    for(int i=0;i<10000;i++) {
        d_ptr = malloc(sizeof(struct domain));
        d_ptr->key = i;
        
    }

    return 0;
}

