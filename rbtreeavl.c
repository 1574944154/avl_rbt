#include "rbtreeavl.h"


#define LOAD_DIRKEY(rbtnode, tree)              (((char *) (rbtnode)) - (tree)->rbtnodeoffset + (tree)->keyoffset)
#define LOAD_INDKEY(rbtnode, tree) (*((char **) (((char *) (rbtnode)) - (tree)->rbtnodeoffset + (tree)->keyoffset)))

static int comparenk (const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_node_t *a, const void *b)
{
    const void *ka;
    if (td->flags & DDSRT_RBT_TREEDEF_FLAG_INDKEY) {
        ka = LOAD_INDKEY (a, td);
    } else {
        ka = LOAD_DIRKEY (a, td);
    }
    if (td->flags & DDSRT_RBT_TREEDEF_FLAG_R) {
        return td->u.comparekk_r (ka, b, td->cmp_arg);
    } else {
        return td->u.comparekk (ka, b);
    }
}

const ddsrt_rbt_node_t *node_from_onode(const ddsrt_rbt_treedef_t *td, const char onode)
{
    if(onode == NULL) {
        return NULL;
    } else {
        return (const ddsrt_rbt_node_t *) (onode + td->rbtnodeoffset);
    }
}

char *onode_from_node(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node)
{
    if(node == NULL)
        return NULL;
    else
        return (char *) node - td->rbtnodeoffset;
}

ddsrt_rbt_node_t *node_from_onode_nonnull (const ddsrt_rbt_treedef_t *td, char *onode)
{
    return (ddsrt_rbt_node_t *) (onode + td->rbtnodeoffset);
}

char *onode_from_node_nonnull (const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node)
{
    return (const char *) node - td->rbtnodeoffset;
}

void ddsrt_rbt_init(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree)
{
    tree->root = NULL;
    (void) td;
}

static void treedestroy(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *n, void (*freefun) (void *node))
{
    if(n) {
        n->__rb_parent_color = (unsigned long)NULL;
        treedestroy(td, n->cs[0], freefun);
        treedestroy(td, n->cs[1], freefun);
        n->cs[0] = NULL;
        n->cs[1] = NULL;
        freefun(onode_from_node(td, n));
    }
}

static void treedestroy_arg(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *n, void (*freefun) (void *node, void *arg), void *arg)
{
    if(n) {
        n->__rb_parent_color = (unsigned long)NULL;
        treedestroy_arg(td, n->cs[0], freefun, arg);
        treedestroy_arg(td, n->cs[1], freefun, arg);
        n->cs[0] = NULL;
        n->cs[1] = NULL;
        freefun(onode_from_node(td, n), arg);
    }
}

void ddsrt_rbt_free(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void (*freefun) (void *node))
{
    if(freefun) {
        treedestroy(td, tree->root, freefun);
    }
    tree->root = NULL;
    tree->count = 0;
}

void ddsrt_rbt_free_arg(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void (*freefun) (void *node, void *arg), void *arg)
{
    if(freefun) {
        treedestroy_arg(td, tree->root, freefun, arg);
    }
    tree->count = 0;
    tree->root = NULL;
}


void *ddsrt_rbt_root(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree)
{
    return (void *) onode_from_node(td, tree->root);
}

static const ddsrt_rbt_node_t *fixup_predsucceq(const ddsrt_rbt_treedef_t *td, const void *key, const ddsrt_rbt_node_t *tmp, const ddsrt_rbt_node_t *cand, int dir)
{
    if(tmp == NULL) {
        return cand;
    } else if(!(td->flags & DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS)) {
        return tmp;
    } else {
        cand = tmp;
        tmp = tmp->cs[1-dir];
        while(tmp) {
            if(comparenk(td, tmp, key) != 0) {
                tmp = tmp->cs[dir];
            } else {
                cand = tmp;
                tmp = tmp->cs[1-dir];
            }
        }
        return cand;
    }
}

static const ddsrt_rbt_node_t *lookup_predeq (const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    const ddsrt_rbt_node_t *tmp = tree->root;
    const ddsrt_rbt_node_t *cand = NULL;
    int c;
    while(tmp && (c = comparenk(td, tmp, key)) != 0) {
        if(c < 0) {
            cand = tmp;
            tmp = tmp->cs[1];
        } else {
            tmp = tmp->cs[0];
        }
    }
    return fixup_predsucceq(td, key, tmp, cand, 0);
}

static const ddsrt_rbt_node_t *lookup_succeq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    const ddsrt_rbt_node_t *tmp = tree->root;
    const ddsrt_rbt_node_t *cand = NULL;
    int c;
    while(tmp && (c = comparenk(td, tmp, key)) != 0) {
        if(c > 0) {
            cand = tmp;
            tmp = tmp->cs[0];
        } else {
            tmp = tmp->cs[1];
        }
    }
    return fixup_predsucceq(td, key, tmp, cand, 1);
}

static const ddsrt_rbt_node_t *fixup_predsucc(const ddsrt_rbt_treedef_t *td, const void *key, const ddsrt_rbt_node_t *tmp, const ddsrt_rbt_node_t *cand, int dir)
{
    if(tmp == NULL || tmp->cs[dir] == NULL) {
        return cand;
    } else if(!(td->flags & DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS)) {
        tmp = tmp->cs[dir];
        while(tmp->cs[1-dir]) {
            tmp = tmp->cs[1-dir];
        }
        return tmp;
    } else {
        tmp = tmp->cs[dir];
        while(tmp) {
            if(comparenk(td, tmp, key) != 0) {
                cand = tmp;
                tmp = tmp->cs[1-dir];
            } else {
                tmp = tmp->cs[dir];
            }
        }
        return cand;
    }
}

static const ddsrt_rbt_node_t *lookup_pred(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    const ddsrt_rbt_node_t *tmp = tree->root;
    const ddsrt_rbt_node_t *cand = NULL;
    int c;
    while(tmp && (c = comparenk(td, tmp, key)) != 0) {
        if(c < 0) {
            cand = tmp;
            tmp = tmp->cs[1];
        } else {
            tmp = tmp->cs[0];
        }
    }
    return fixup_predsucc(td, key, tmp, cand, 0);
}

static const ddsrt_rbt_node_t *lookup_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    const ddsrt_rbt_node_t *tmp = tree->root;
    const ddsrt_rbt_node_t *cand = NULL;
    int c;
    while(tmp && (c = comparenk(td, tmp, key)) != 0) {
        if(c > 0) {
            cand = tmp;
            tmp = tmp->cs[0];
        } else {
            tmp = tmp->cs[1];
        }
    }
    return fixup_predsucc(td, key, tmp, cand, 1);
}

void *ddsrt_rbt_lookup(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    const ddsrt_rbt_node_t *cursor = tree->root;
    int c;
    while (cursor && (c = comparenk(td, cursor, key)) != 0) {
        const int dir = (c <= 0);
        cursor = cursor->cs[dir];
    }
    return (void *)onode_from_node(td, cursor);
}

void *ddsrt_rbt_lookup_pred_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *) onode_from_node(td, lookup_predeq(td, tree, key));
}

void *ddsrt_rbt_lookup_succ_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *) onode_from_node(td, lookup_succeq(td, tree, key));
}

void *ddsrt_rbt_lookup_pred(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *)onode_from_node(td, lookup_pred(td, tree, key));
}

void *ddsrt_rbt_lookup_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *)onode_from_node(td, lookup_succ(td, tree, key));
}

void ddsrt_rbt_insert(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, void *node);
void ddsrt_rbt_delete(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, void *node);

static ddsrt_rbt_node_t *find_neighbour(const ddsrt_rbt_node_t *n, int dir)
{
    // dir=0 => pred;   dir=1 => succ
    if(n->cs[dir]) {
        n = n->cs[dir];
        while(n->cs[1-dir]) {
            n = n->cs[1-dir];
        }
        return (ddsrt_rbt_node_t *)n;
    } else {
        const ddsrt_rbt_node_t *p = rb_parent(n);
        while(p && n==p->cs[dir]) {
            n = p;
            p = rb_parent(p);
        }
        return (ddsrt_rbt_node_t *)p;
    }
}

void ddsrt_rbt_swap_node(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, void *oldn, void *newn);

int ddsrt_rbt_is_empty(const ddsrt_rbt_tree_t *tree)
{
    return tree->root == NULL;
}
size_t ddsrt_rbt_count(const ddsrt_rbt_tree_t *tree)
{
    return tree->count;
}

static ddsrt_rbt_node_t *find_extremum(const ddsrt_rbt_tree_t *tree, int dir)
{
    const ddsrt_rbt_node_t *n = tree->root;
    if(n) {
        while(n->cs[dir]) {
            n = n->cs[dir];
        }
    }
    return (ddsrt_rbt_node_t *)n;
}

void *ddsrt_rbt_find_min(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree)
{
    return (void *)onode_from_node(td, find_extremum(tree, 0));
}

void *ddsrt_rbt_find_max(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree)
{
    return (void *)onode_from_node(td, find_extremum(tree, 1));
}

void *ddsrt_rbt_find_pred(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *vnode)
{
    const ddsrt_rbt_node_t *n = node_from_onode(td, vnode);
    if(n == NULL) {
        return ddsrt_rbt_find_max(td, tree);
    }
    return (void *)onode_from_node(td, find_neighbour(n, 0));
}

void *ddsrt_rbt_find_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *vnode)
{
    const ddsrt_rbt_node_t *n = node_from_onode(td, vnode);
    if(n == NULL) {
        return ddsrt_rbt_find_min(td, tree);
    }
    return (void *)onode_from_node(td, find_neighbour(n, 1));
}

void ddsrt_rbt_walk(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_walk_t f, void *a)
{
    const ddsrt_rbt_node_t *todo[1+DDSRT_RBT_MAX_TREEHEIGHT];   // 数组模拟栈 
    const ddsrt_rbt_node_t **todop = todo + 1;
    *todop = tree->root;
    while(*todop) {                            // 中序遍历
        ddsrt_rbt_node_t *right, *n;

        n = (*todop)->cs[0];
        while(n) {
            *++todop = n;     // 入栈
            n = n->cs[0];
        }

        do {
            right = (*todop)->cs[1];
            f((void *)onode_from_node_nonnull(td, *todop), a);
        } while(todop-- > todo+1 && right == NULL);    // 出栈 判断栈空 

        *++todop = right;
    }
}

void ddsrt_rbt_const_walk(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_const_walk_t f, void *a)
{
    ddsrt_rbt_walk(td, (ddsrt_rbt_tree_t *)tree, (ddsrt_rbt_walk_t)f, a);
}

void ddsrt_rbt_walk_range(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *min, const void *max, ddsrt_rbt_walk_t f, void *a)
{
    ddsrt_rbt_node_t *n, *nn;
    n = (ddsrt_rbt_node_t *)lookup_succeq(td, tree, min);
    while(n && comparenk(td, n, max) <= 0) {
        nn = find_neighbour(n, 1);
        f(onode_from_node_nonnull(td, n), a);
        n = nn;
    }
}

void ddsrt_rbt_walk_range_reverse(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *min, const void *max, ddsrt_rbt_walk_t f, void *a)
{
    ddsrt_rbt_node_t *n, *nn;
    n = (ddsrt_rbt_node_t *)lookup_predeq(td, tree, max);
    while(n && comparenk(td, n, min) >= 0) {
        nn = find_neighbour(n, 0);
        f(onode_from_node_nonnull(td, n), a);
        n = nn;
    }
}

void ddsrt_rbt_const_walk_range_reverse(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *min, const void *max, ddsrt_rbt_const_walk_t f, void *a)
{
    ddsrt_rbt_walk_range_reverse(td, (ddsrt_rbt_tree_t *)tree, min, max, (ddsrt_rbt_walk_t)f, a);
}

