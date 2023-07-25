
#ifndef __RBTREERBT_H
#define __RBTREERBT_H
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

typedef int (*ddsrt_rbt_compare_t) (const void *a, const void *b);
typedef int (*ddsrt_rbt_compare_r_t) (const void *a, const void *b, void *arg);
typedef void (*ddsrt_rbt_augment_t) (void *node, const void *left, const void *right);
typedef void (*ddsrt_rbt_walk_t) (void *node, void *arg);
typedef void (*ddsrt_rbt_const_walk_t ) (const void *node, void *arg);

#define DDSRT_RBT_TREEDEF_FLAG_INDKEY       (1<<0)
#define DDSRT_RBT_TREEDEF_FLAG_R            (1<<1)
#define DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS    (1<<2)

#define DDSRT_RBT_MAX_TREEHEIGHT    (12 * sizeof(void *))

typedef struct ddsrt_rbt_node {
    struct ddsrt_rbt_node *cs[2];
    unsigned long  __rb_parent_color;
} ddsrt_rbt_node_t;

typedef struct ddsrt_rbt_treedef {
    size_t rbtnodeoffset;
    size_t keyoffset;

    union {
        ddsrt_rbt_compare_t comparekk;
        ddsrt_rbt_compare_r_t comparekk_r;
    } u;
    ddsrt_rbt_augment_t augment;
    uint32_t flags;
    void *cmp_arg;
} ddsrt_rbt_treedef_t;

typedef struct ddsrt_rbt_tree {
    ddsrt_rbt_node_t *root;
    size_t count;
} ddsrt_rbt_tree_t;

typedef struct ddsrt_rbt_path {
    ddsrt_rbt_node_t *parent;
    ddsrt_rbt_node_t **pnode;
} ddsrt_rbt_path_t;

typedef ddsrt_rbt_path_t ddsrt_rbt_ipath_t;
typedef ddsrt_rbt_path_t ddsrt_rbt_dpath_t;

typedef struct ddsrt_rbt_iter {
    const ddsrt_rbt_treedef_t *td;
    ddsrt_rbt_node_t *right;
    ddsrt_rbt_node_t **todop;
    ddsrt_rbt_node_t *todo[1 + DDSRT_RBT_MAX_TREEHEIGHT];
} ddsrt_rbt_iter_t;

#define RB_RED                      0
#define RB_BLACK                    1

#define rb_parent(node)            ((ddsrt_rbt_node_t*)((node)->__rb_parent_color & ~3))

#define __rb_parent(pc)             ((ddsrt_rbt_node_t *)(pc & ~3));
#define __rb_color(pc)              ((pc) & 1)
#define __rb_is_black(pc)           __rb_color(pc)
#define __rb_is_red(pc)             (!__rb_color(pc))
#define rb_color(rb)                __rb_color((rb)->__rb_parent_color)
#define rb_is_red(rb)               __rb_is_red((rb)->__rb_parent_color)
#define rb_is_black(rb)             __rb_is_black((rb)->__rb_parent_color)


const ddsrt_rbt_node_t *cnode_from_onode (const ddsrt_rbt_treedef_t *td, const char *onode);

char *onode_from_node (const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node);

const char *conode_from_node (const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_node_t *node);

ddsrt_rbt_node_t *node_from_onode_nonnull (const ddsrt_rbt_treedef_t *td, char *onode);

const ddsrt_rbt_node_t *cnode_from_onode_nonnull (const ddsrt_rbt_treedef_t *td, const char *onode);

char *onode_from_node_nonnull (const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node);

const char *conode_from_node_nonnull (const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_node_t *node);


#define DDSRT_RBT_TREEDEF_INITIALIZER(rbtnodeoffset, keyoffset, comparekk_, augment) { (rbtnodeoffset), (keyoffset), { .comparekk = (comparekk_) }, (augment), 0, 0 }
#define DDSRT_RBT_TREEDEF_INITIALIZER_INDKEY(rbtnodeoffset, keyoffset, comparekk_, augment) { (rbtnodeoffset), (keyoffset), { .comparekk = (comparekk_) }, (augment), DDSRT_RBT_TREEDEF_FLAG_INDKEY, 0 }
#define DDSRT_RBT_TREEDEF_INITIALIZER_ALLOWDUPS(rbtnodeoffset, keyoffset, comparekk_, augment) { (rbtnodeoffset), (keyoffset), { .comparekk = (comparekk_) }, (augment), DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS, 0 }
#define DDSRT_RBT_TREEDEF_INITIALIZER_INDKEY_ALLOWDUPS(rbtnodeoffset, keyoffset, comparekk_, augment) { (rbtnodeoffset), (keyoffset), { .comparekk = (comparekk_) }, (augment), DDSRT_RBT_TREEDEF_FLAG_INDKEY|DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS, 0 }
#define DDSRT_RBT_TREEDEF_INITIALIZER_R(rbtnodeoffset, keyoffset, comparekk_, cmparg, augment) { (rbtnodeoffset), (keyoffset), { .comparekk_r = (comparekk_) }, (augment), DDSRT_RBT_TREEDEF_FLAG_R, (cmparg) }
#define DDSRT_RBT_TREEDEF_INITIALIZER_INDKEY_R(rbtnodeoffset, keyoffset, comparekk_, cmparg, augment) { (rbtnodeoffset), (keyoffset), { .comparekk_r = (comparekk_) }, (augment), DDSRT_RBT_TREEDEF_FLAG_INDKEY|DDSRT_RBT_TREEDEF_FLAG_R, (cmparg) }
#define DDSRT_RBT_TREEDEF_INITIALIZER_R_ALLOWDUPS(rbtnodeoffset, keyoffset, comparekk_, cmparg, augment) { (rbtnodeoffset), (keyoffset), { .comparekk_r = (comparekk_) }, (augment), DDSRT_RBT_TREEDEF_FLAG_R|DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS, (cmparg) }
#define DDSRT_RBT_TREEDEF_INITIALIZER_INDKEY_R_ALLOWDUPS(rbtnodeoffset, keyoffset, comparekk_, cmparg, augment) { (rbtnodeoffset), (keyoffset), { .comparekk_r = (comparekk_) }, (augment), DDSRT_RBT_TREEDEF_FLAG_INDKEY|DDSRT_RBT_TREEDEF_FLAG_R|DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS, (cmparg) }


void ddsrt_rbt_init(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree);
void ddsrt_rbt_free(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void (*freefun) (void *node));
void ddsrt_rbt_free_arg(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void (*freefun) (void *node, void *arg), void *arg);

void *ddsrt_rbt_root(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree);

void *ddsrt_rbt_lookup(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key);
void *ddsrt_rbt_lookup_ipath(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key, ddsrt_rbt_ipath_t *path);
void *ddsrt_rbt_lookup_dpath(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key, ddsrt_rbt_dpath_t *path);
void *ddsrt_rbt_lookup_pred_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key);
void *ddsrt_rbt_lookup_succ_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key);
void *ddsrt_rbt_lookup_pred(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key);
void *ddsrt_rbt_lookup_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key);


void ddsrt_rbt_insert(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode);
void ddsrt_rbt_delete(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode);
void ddsrt_rbt_insert_ipath(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode, ddsrt_rbt_ipath_t *path);
void ddsrt_rbt_delete_dpath(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *node, ddsrt_rbt_dpath_t *path);
void ddsrt_rbt_swap_node(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, void *oldn, void *newn);

int ddsrt_rbt_is_empty(const ddsrt_rbt_tree_t *tree);
size_t ddsrt_rbt_count(const ddsrt_rbt_tree_t *tree);

void *ddsrt_rbt_find_min(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree);
void *ddsrt_rbt_find_max(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree);
void *ddsrt_rbt_find_pred(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *vnode);
void *ddsrt_rbt_find_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *vnode);

void ddsrt_rbt_walk(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, ddsrt_rbt_walk_t f, void *a);
void ddsrt_rbt_const_walk(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_const_walk_t f, void *a);
void ddsrt_rbt_walk_range(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *min, const void *max, ddsrt_rbt_walk_t f, void *a);
void ddsrt_rbt_walk_range_reverse(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *min, const void *max, ddsrt_rbt_walk_t f, void *a);
void ddsrt_rbt_const_walk_range_reverse(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *min, const void *max, ddsrt_rbt_const_walk_t f, void *a);

void *ddsrt_rbt_iter_first(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_iter_t *iter);
void *ddsrt_rbt_iter_succ_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_iter_t *iter, const void *key);
void *ddsrt_rbt_iter_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_iter_t *iter, const void *key);
void *ddsrt_rbt_iter_next(ddsrt_rbt_iter_t *iter);

#endif
