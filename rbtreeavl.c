
#include "rbtreeavl.h"

#include <stdio.h>


#define LOAD_DIRKEY(rbtnode, tree)              (((char *) (rbtnode)) - (tree)->rbtnodeoffset + (tree)->keyoffset)
#define LOAD_INDKEY(rbtnode, tree) (*((char **) (((char *) (rbtnode)) - (tree)->rbtnodeoffset + (tree)->keyoffset)))


static inline void rb_set_parent(ddsrt_rbt_node_t *rb, ddsrt_rbt_node_t *p)
{
    rb->__rb_parent_color = rb_color(rb) + (unsigned long)p;
}

static inline void rb_set_black(ddsrt_rbt_node_t *rb)
{
    rb->__rb_parent_color += RB_BLACK;
}

static inline ddsrt_rbt_node_t *rb_red_parent(ddsrt_rbt_node_t *red)
{
    return (ddsrt_rbt_node_t *)red->__rb_parent_color;
}

static inline void rb_set_parent_color(ddsrt_rbt_node_t *rb,
                                    ddsrt_rbt_node_t *p, int color)
{
    rb->__rb_parent_color = (unsigned long)p + color;
}

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

const ddsrt_rbt_node_t *cnode_from_onode (const ddsrt_rbt_treedef_t *td, const char *onode)
{
    if (onode == NULL) {
        return NULL;
    } else {
        return (const ddsrt_rbt_node_t *) (onode + td->rbtnodeoffset);
    }
}

char *onode_from_node (const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node)
{
    if (node == NULL) {
        return NULL;
    } else {
        return (char *) node - td->rbtnodeoffset;
    }
}

const char *conode_from_node (const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_node_t *node)
{
    if (node == NULL) {
        return NULL;
    } else {
        return (const char *) node - td->rbtnodeoffset;
    }
}

ddsrt_rbt_node_t *node_from_onode_nonnull (const ddsrt_rbt_treedef_t *td, char *onode)
{
  return (ddsrt_rbt_node_t *) (onode + td->rbtnodeoffset);
}

const ddsrt_rbt_node_t *cnode_from_onode_nonnull (const ddsrt_rbt_treedef_t *td, const char *onode)
{
  return (const ddsrt_rbt_node_t *) (onode + td->rbtnodeoffset);
}

char *onode_from_node_nonnull (const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node)
{
  return (char *) node - td->rbtnodeoffset;
}

const char *conode_from_node_nonnull (const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_node_t *node)
{
  return (const char *) node - td->rbtnodeoffset;
}

void ddsrt_rbt_init(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree)
{
    tree->root = NULL;
    tree->count = 0;
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

static void augment(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *n)
{
    td->augment(onode_from_node_nonnull(td, n), conode_from_node(td, n->cs[0]), conode_from_node(td, n->cs[1]));
}

static void augment_propagate(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node, ddsrt_rbt_node_t *stop)
{
    while(node != stop) {
        augment(td, node);
        node = rb_parent(node);
    }
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

static const ddsrt_rbt_node_t *lookup_path(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key, ddsrt_rbt_path_t *path)
{
    const ddsrt_rbt_node_t *cursor = tree->root;
    const ddsrt_rbt_node_t *prev = NULL;
    int c;
    // path->depth = 0;
    // path->pnode[0] = (ddsrt_rbt_node_t **) &tree->root;
    path->pnode = (ddsrt_rbt_node_t **) &tree->root;
    while(cursor && (c = comparenk(td, cursor, key)) != 0) {
        const int dir = (c <= 0);
        prev = cursor;
        // path->pnode[++path->depth] = (ddsrt_rbt_node_t **) &cursor->cs[dir];
        path->pnode = (ddsrt_rbt_node_t **) &cursor->cs[dir];
        cursor = cursor->cs[dir];
    }
    // path->pnode = (ddsrt_rbt_node_t **)&cursor;
    path->parent = (ddsrt_rbt_node_t *) prev;
    return cursor;
}

void *ddsrt_rbt_lookup(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    const ddsrt_rbt_node_t *cursor = tree->root;
    int c;
    while (cursor && (c = comparenk(td, cursor, key)) != 0) {
        const int dir = (c <= 0);
        cursor = cursor->cs[dir];
    }
    return (void *)conode_from_node(td, cursor);
}

void *ddsrt_rbt_lookup_ipath(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key, ddsrt_rbt_ipath_t *path)
{
    const ddsrt_rbt_node_t *node = lookup_path(td, tree, key, path);
    if(node) {
        if(!(td->flags & DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS)) {
            path->pnode = NULL;
        } else {
            const ddsrt_rbt_node_t *cursor = node;
            const ddsrt_rbt_node_t *prev;
            int c, dir;
            do {
                c = comparenk(td, cursor, key);
                dir = (c <= 0);
                prev = cursor;
                // path->pnode[++path->depth] = (ddsrt_rbt_node_t **) &cursor->cs[dir];
                cursor = cursor->cs[dir];
            } while(cursor);
            path->pnode = (ddsrt_rbt_node_t **)&cursor;
            path->parent = (ddsrt_rbt_node_t *) prev;
        }
    }
    return (void *) conode_from_node(td, node);
}

void *ddsrt_rbt_lookup_dpath(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key, ddsrt_rbt_dpath_t *path)
{

}

void *ddsrt_rbt_lookup_pred_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *) conode_from_node(td, lookup_predeq(td, tree, key));
}

void *ddsrt_rbt_lookup_succ_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *) conode_from_node(td, lookup_succeq(td, tree, key));
}

void *ddsrt_rbt_lookup_pred(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *) conode_from_node(td, lookup_pred(td, tree, key));
}

void *ddsrt_rbt_lookup_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *key)
{
    return (void *) conode_from_node(td, lookup_succ(td, tree, key));
}

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
    const ddsrt_rbt_node_t *n = cnode_from_onode(td, vnode);
    if(n == NULL) {
        return ddsrt_rbt_find_max(td, tree);
    }
    return (void *) conode_from_node(td, find_neighbour(n, 0));
}

void *ddsrt_rbt_find_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, const void *vnode)
{
    const ddsrt_rbt_node_t *n = cnode_from_onode(td, vnode);
    if(n == NULL) {
        return ddsrt_rbt_find_min(td, tree);
    }
    return (void *) conode_from_node(td, find_neighbour(n, 1));
}

static void rbt_iter_downleft(ddsrt_rbt_iter_t *iter)
{
    if(*iter->todop) {
        ddsrt_rbt_node_t *n;
        n = (*iter->todop)->cs[0];
        while(n) {
            assert((int)(iter->todop - iter->todo) < (int)(sizeof(iter->todo) / sizeof(*iter->todo)));
            *++iter->todop = n;
            n = n->cs[0];
        }
        iter->right = (*iter->todop)->cs[1];
    }
}

void *ddsrt_rbt_iter_first(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_iter_t *iter)
{
    iter->td = td;
    iter->todop = iter->todo+1;
    *iter->todop = (ddsrt_rbt_node_t *)tree->root;
    rbt_iter_downleft(iter);
    return onode_from_node(td, *iter->todop);
}

void *ddsrt_rbt_iter_next(ddsrt_rbt_iter_t *iter)
{
    if(iter->todop-- > iter->todo+1 && iter->right == NULL) {
        iter->right = (*iter->todop)->cs[1];
    } else {
        assert((int)(iter->todop - iter->todo) < (int)(sizeof(iter->todo)/sizeof(*iter->todo)));
        *++iter->todop = iter->right;
        rbt_iter_downleft(iter);
    }
    return onode_from_node(iter->td, *iter->todop);
}

void ddsrt_rbt_walk(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, ddsrt_rbt_walk_t f, void *a)
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
            f((void *) conode_from_node_nonnull(td, *todop), a);
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

void *ddsrt_rbt_iter_succ_eq(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_iter_t *iter, const void *key)
{
    const ddsrt_rbt_node_t *tmp = tree->root;
    int c;
    iter->td = td;
    iter->todop = iter->todo;
    while(tmp && (c = comparenk(td, tmp, key)) != 0) {
        if(c > 0) {
            *++iter->todop = (ddsrt_rbt_node_t *)tmp;
            tmp = tmp->cs[0];
        } else {
            tmp = tmp->cs[1];
        }
    }
    if(tmp != NULL) {
        *++iter->todop = (ddsrt_rbt_node_t *)tmp;
        if(td->flags & DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS) {
            tmp = tmp->cs[0];
            while(tmp) {
                if(comparenk(td, tmp, key) != 0) {
                    tmp = tmp->cs[1];
                } else {
                    *++iter->todop = (ddsrt_rbt_node_t *)tmp;
                    tmp = tmp->cs[0];
                }
            }
        }
    }
    if(iter->todop == iter->todo) {
        return NULL;
    } else {
        iter->right = (*iter->todop)->cs[1];
        return onode_from_node(td, *iter->todop);
    }
}

void *ddsrt_rbt_iter_succ(const ddsrt_rbt_treedef_t *td, const ddsrt_rbt_tree_t *tree, ddsrt_rbt_iter_t *iter, const void *key)
{
    const ddsrt_rbt_node_t *tmp = tree->root;
    int c;
    iter->td = td;
    iter->todop = iter->todo;
    while(tmp && (c = comparenk(td, tmp, key)) != 0) {
        if(c > 0) {
            *++iter->todop = (ddsrt_rbt_node_t *)tmp;
            tmp = tmp->cs[0];
        } else {
            tmp = tmp->cs[1];
        }
    }
    if(tmp != NULL) {
        if(!(td->flags & DDSRT_RBT_TREEDEF_FLAG_ALLOWDUPS)) {
            tmp = tmp->cs[1];
            if(tmp) {
                do {
                    *++iter->todop = (ddsrt_rbt_node_t *)tmp;
                    tmp = tmp->cs[0];
                } while(tmp);
            }
        } else {
            tmp = tmp->cs[1];
            while(tmp) {
                if(comparenk(td, tmp, key) != 0) {
                    *++iter->todop = (ddsrt_rbt_node_t *)tmp;
                    tmp = tmp->cs[0];
                } else {
                    tmp = tmp->cs[1];
                }
            }
        }
    }
    if(iter->todop == iter->todo) {
        return NULL;
    } else {
        iter->right = (*iter->todop)->cs[1];
        return onode_from_node(td, *iter->todop);
    }
}

static inline void rb_link_node(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node, ddsrt_rbt_node_t *parent, 
                            ddsrt_rbt_node_t **rb_link)
{
    node->__rb_parent_color = (unsigned long) parent;
    node->cs[0] = NULL;
    node->cs[1] = NULL;
    if(td->augment)
        augment(td, node);
    *rb_link = node;
}

// 
static inline void __rb_change_child(ddsrt_rbt_node_t *old_node, ddsrt_rbt_node_t *new_node,
                ddsrt_rbt_node_t *parent, ddsrt_rbt_tree_t *tree)
{
    // old_node 是否是根节点
    if(parent) {
        if(parent->cs[0] == old_node)
            parent->cs[0] = new_node;
        else 
            parent->cs[1] = new_node;
    } else
        tree->root = new_node;
}

static inline void __rb_rotate_set_parents(ddsrt_rbt_node_t *old_node, ddsrt_rbt_node_t *new_node,
                                ddsrt_rbt_tree_t *tree, int color)
{
    ddsrt_rbt_node_t *parent = rb_parent(old_node);
    new_node->__rb_parent_color = old_node->__rb_parent_color;
    rb_set_parent_color(old_node, new_node, color);
    __rb_change_child(old_node, new_node, parent, tree);
}

static inline void __rb_change_color(ddsrt_rbt_node_t *tmp, ddsrt_rbt_node_t *node,
                                             ddsrt_rbt_node_t *parent, ddsrt_rbt_node_t *gparent)
{

}

static inline void rb_insert_color(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node, ddsrt_rbt_tree_t *tree)
{
    ddsrt_rbt_node_t *parent = rb_red_parent(node), *gparent, *tmp;

    for(;;) {

        if(parent == NULL) {
            rb_set_parent_color(node, NULL, RB_BLACK);
            break;
        }

        if(rb_is_black(parent))
            break;

        gparent = rb_red_parent(parent);
        tmp = gparent->cs[1];
        if(parent != tmp) {
            if(tmp && rb_is_red(tmp)) {
				/*
				 * Case 1 - node's uncle is red (color flips).
				 *
				 *       G            g
				 *      / \          / \
				 *     p   u  -->   P   U
				 *    /            /
				 *   n            n
				 *
				 * However, since g's parent might be red, and
				 * 4) does not allow this, we need to recurse
				 * at g.
				 */
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->cs[1];
            if(node == tmp) {
				/*
				 * Case 2 - node's uncle is black and node is
				 * the parent's right child (left rotate at parent).
				 *
				 *      G             G
				 *     / \           / \
				 *    p   U  -->    n   U
				 *     \           /
				 *      n         p
				 *
				 * This still leaves us in violation of 4), the
				 * continuation into Case 3 will fix that.
				 */
                tmp = node->cs[0];
                parent->cs[1] = tmp;
                node->cs[0] = parent;
                if(tmp)
                    rb_set_parent_color(tmp, parent, RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);
                if(td->augment) {
                    augment(td, parent);
                    augment(td, node);
                }
                parent = node;
                tmp = node->cs[1];
            }
			/*
			 * Case 3 - node's uncle is black and node is
			 * the parent's left child (right rotate at gparent).
			 *
			 *        G           P
			 *       / \         / \
			 *      p   U  -->  n   g
			 *     /                 \
			 *    n                   U
			 */
            gparent->cs[0] = tmp;
            parent->cs[1] = gparent;
            if(tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, tree, RB_RED);
            if(td->augment) {
                augment(td, gparent);
                augment(td, parent);
            }
            break;
        } else {
            tmp = gparent->cs[0];
            if(tmp && rb_is_red(tmp)) {
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->cs[0];
            if(node == tmp) {
                tmp = node->cs[1];
                parent->cs[0] = tmp;
                node->cs[1] = parent;
                if(tmp)
                    rb_set_parent_color(tmp, parent, RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);

                if(td->augment) {
                    augment(td, parent);
                    augment(td, node);
                }
                parent = node;
                tmp = node->cs[0];
            }

            gparent->cs[1] = tmp;
            parent->cs[0] = gparent;
            if(tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, tree, RB_RED);
            if(td->augment) {
                augment(td, gparent);
                augment(td, parent);
            }
            break;
        }
    }
}

static void rebalance_path(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, ddsrt_rbt_node_t *node)
{
    rb_insert_color(td, node, tree);
}

void ddsrt_rbt_insert_ipath(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode, ddsrt_rbt_ipath_t *path)
{
    ddsrt_rbt_node_t *node = node_from_onode_nonnull(td, vnode);
    
    assert(path->pnode);
    assert((*path->pnode) == NULL);

    rb_link_node(td, node, path->parent, path->pnode);
    rebalance_path(td, tree, node);
}

void ddsrt_rbt_insert(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode)
{
    const void *node = node_from_onode_nonnull(td, vnode);
    const void *key;
    ddsrt_rbt_ipath_t path;
    if(td->flags & DDSRT_RBT_TREEDEF_FLAG_INDKEY) {
        key = LOAD_INDKEY(node, td);
    } else {
        key = LOAD_DIRKEY(node, td);
    }
    ddsrt_rbt_lookup_ipath(td, tree, key, &path);
    ddsrt_rbt_insert_ipath(td, tree, vnode, &path);
    tree->count ++;
}

static inline ddsrt_rbt_node_t *__rb_erase(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *node, ddsrt_rbt_tree_t *tree)
{
    ddsrt_rbt_node_t *child = node->cs[1];
    ddsrt_rbt_node_t *tmp = node->cs[0];
    ddsrt_rbt_node_t *parent, *rebalance;
    unsigned long pc;

    if(!tmp) {   // 被删除节点的左子树为空
		/*
		 * Case 1: node to erase has no more than 1 child (easy!)
		 * Note that if there is one child it must be red due to 5)
		 * and node must be black due to 4). We adjust colors locally
		 * so as to bypass __rb_erase_color() later on.
		 */
        pc = node->__rb_parent_color;
        parent = __rb_parent(pc);

        __rb_change_child(node, child, parent, tree);
        if(child) {
        /*
         *          (n)(black)              (child)(red)
         *          / \             =>       /  \
         *        nil child(red)           nil  nil
         */
            child->__rb_parent_color = pc;
            rebalance = NULL;
        } else {
		/*   only n
         *
         *          (n)
		 */
            rebalance = __rb_is_black(pc) ? parent : NULL;   // n是红色或者 n是黑色且n为根节点
        }
        tmp = parent;
    } else if (!child) {  // child == NULL
        /* Still case 1, but this time the child is node->cs[0] */
        tmp->__rb_parent_color = pc = node->__rb_parent_color;
        parent = __rb_parent(pc);
        __rb_change_child(node, tmp, parent, tree);
        rebalance = NULL;
        tmp = parent;
    } else {
        ddsrt_rbt_node_t *successor = child, *child2;

        tmp = child->cs[0];
        if(!tmp) {   
			/*
			 * Case 2: node's successor is its right child
			 *
			 *    (n)          (s)
			 *    / \          / \
			 *  (x) (s)  ->  (x) (c)
			 *      / \
			 *    nil (c)
			 */
            parent = successor;
            child2 = successor->cs[1];
            if(td->augment)
                augment(td, successor);
        } else {
			/*
			 * Case 3: node's successor is leftmost under
			 * node's right child subtree
			 *
			 *    (n)          (s)
			 *    / \          / \
			 *  (x) (y)  ->  (x) (y)
			 *      /            /
			 *    (p)          (p)
			 *    /            /
			 *  (s)          (c)
			 *    \
			 *    (c)
			 */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->cs[0];
            } while(tmp);
            child2 = successor->cs[1];
            parent->cs[0] = child2;
            successor->cs[1] = child;
            rb_set_parent(child, successor);
            if(td->augment) {
                augment(td, successor);
                augment_propagate(td, parent, successor);
            }
        }

        tmp = node->cs[0];
        successor->cs[0] = tmp;
        rb_set_parent(tmp, successor);

        pc = node->__rb_parent_color;
        tmp = __rb_parent(pc);
        __rb_change_child(node, successor, tmp, tree);

        if(child2) {
            rb_set_parent_color(child2, parent, RB_BLACK);
            rebalance = NULL;
        } else {
            rebalance = rb_is_black(successor) ? parent : NULL;
        }
        successor->__rb_parent_color = pc;
        tmp = successor;
    }

    if(td->augment && tmp)
        augment_propagate(td, tmp, NULL);

    return rebalance;
}

void __rb_erase_color(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_node_t *parent, ddsrt_rbt_tree_t *tree)
{
    ddsrt_rbt_node_t *node = NULL, *sibling, *tmp1, *tmp2;

    for(;;) {
		/*
		 * Loop invariants:
		 * - node is black (or NULL on first iteration)
		 * - node is not the root (parent is not NULL)
		 * - All leaf paths going through parent and node have a
		 *   black node count that is 1 lower than other leaf paths.
		 */
        sibling = parent->cs[1];
        if(node != sibling) {  /* node == parent->cs[0] */
            if(rb_is_red(sibling)) {
				/*
				 * Case 1 - left rotate at parent
				 *
				 *     P               S
				 *    / \             / \
				 *   N   s    -->    p   Sr
				 *      / \         / \
				 *     Sl  Sr      N   Sl
				 */
                tmp1 = sibling->cs[0];
                parent->cs[1] = tmp1;
                sibling->cs[0] = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, tree, RB_RED);
                if(td->augment) {
                    augment(td, parent);
                    augment(td, sibling);
                }
                sibling = tmp1;
            }
            tmp1 = sibling->cs[1];
            if(!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->cs[0];
                if(!tmp2 || rb_is_black(tmp2)) {
					/*
					 * Case 2 - sibling color flip
					 * (p could be either color here)
					 *
					 *    (p)           (p)
					 *    / \           / \
					 *   N   S    -->  N   s
					 *      / \           / \
					 *     Sl  Sr        Sl  Sr
					 *
					 * This leaves us violating 5) which
					 * can be fixed by flipping p to black
					 * if it was red, or by recursing at p.
					 * p is red when coming from Case 1.
					 */
                    rb_set_parent_color(sibling, parent, RB_RED);
                    if(rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if(parent)
                            continue;
                    }
                    break;
                }
				/*
				 * Case 3 - right rotate at sibling
				 * (p could be either color here)
				 *
				 *   (p)           (p)
				 *   / \           / \
				 *  N   S    -->  N   sl
				 *     / \             \
				 *    sl  Sr            S
				 *                       \
				 *                        Sr
				 *
				 * Note: p might be red, and then both
				 * p and sl are red after rotation(which
				 * breaks property 4). This is fixed in
				 * Case 4 (in __rb_rotate_set_parents()
				 *         which set sl the color of p
				 *         and set p RB_BLACK)
				 *
				 *   (p)            (sl)
				 *   / \            /  \
				 *  N   sl   -->   P    S
				 *       \        /      \
				 *        S      N        Sr
				 *         \
				 *          Sr
				 */
                tmp1 = tmp2->cs[1];
                sibling->cs[0] = tmp1;
                tmp2->cs[1] = sibling;
                parent->cs[1] = tmp2;
                if(tmp1)
                    rb_set_parent_color(tmp1, sibling, RB_BLACK);
                if(td->augment) {
                    augment(td, sibling);
                    augment(td, tmp2);
                }
                tmp1 = sibling;
                sibling = tmp2;
            }
			/*
			 * Case 4 - left rotate at parent + color flips
			 * (p and sl could be either color here.
			 *  After rotation, p becomes black, s acquires
			 *  p's color, and sl keeps its color)
			 *
			 *      (p)             (s)
			 *      / \             / \
			 *     N   S     -->   P   Sr
			 *        / \         / \
			 *      (sl) sr      N  (sl)
			 */
            tmp2 = sibling->cs[0];
            parent->cs[1] = tmp2;
            sibling->cs[0] = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if(tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, tree, RB_BLACK);
            if(td->augment) {
                augment(td, parent);
                augment(td, sibling);
            }
            break;
        } else {
            sibling = parent->cs[0];
            if(rb_is_red(sibling)) {
                tmp1 = sibling->cs[1];
                parent->cs[0] = tmp1;
                sibling->cs[1] = parent;
                rb_set_parent_color(tmp1, parent, RB_BLACK);
                __rb_rotate_set_parents(parent, sibling, tree, RB_RED);
                if(td->augment) {
                    augment(td, parent);
                    augment(td, sibling);
                }
                sibling = tmp1;
            }
            tmp1 = sibling->cs[0];
            if(!tmp1 || rb_is_black(tmp1)) {
                tmp2 = sibling->cs[1];
                if(!tmp2 || rb_is_black(tmp2)) {
                    rb_set_parent_color(sibling, parent, RB_RED);
                    if(rb_is_red(parent))
                        rb_set_black(parent);
                    else {
                        node = parent;
                        parent = rb_parent(node);
                        if(parent)
                            continue;
                    }
                    break;
                }
                tmp1 = tmp2->cs[0];
                sibling->cs[1] = tmp1;
                tmp2->cs[0] = sibling;
                parent->cs[0] = tmp2;
                if(tmp1) 
                    rb_set_parent_color(tmp1, sibling, RB_BLACK);
                if(td->augment) {
                    augment(td, sibling);
                    augment(td, tmp2);
                }
                tmp1 = sibling;
                sibling = tmp2;
            }
            tmp2 = sibling->cs[1];
            parent->cs[0] = tmp2;
            sibling->cs[1] = parent;
            rb_set_parent_color(tmp1, sibling, RB_BLACK);
            if(tmp2)
                rb_set_parent(tmp2, parent);
            __rb_rotate_set_parents(parent, sibling, tree, RB_BLACK);
            if(td->augment) {
                augment(td, parent);
                augment(td, sibling);
            }
            break;
        }
    }
}

void ddsrt_rbt_delete(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode)
{
    assert(tree->count > 0);
    ddsrt_rbt_node_t *node = node_from_onode_nonnull(td, vnode);

    ddsrt_rbt_node_t *rebalance = __rb_erase(td, node, tree);
    if(rebalance)
        __rb_erase_color(td, rebalance, tree);

    tree->count --;
}
