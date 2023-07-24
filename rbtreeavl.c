
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

static inline void rb_link_node(ddsrt_rbt_node_t *node, ddsrt_rbt_node_t *parent, 
                            ddsrt_rbt_node_t **rb_link)
{
    node->__rb_parent_color = (unsigned long) parent;
    node->cs[0] = NULL;
    node->cs[1] = NULL;
    *rb_link = node;
}

static inline void __rb_change_child(ddsrt_rbt_node_t *old_node, ddsrt_rbt_node_t *new_node,
                ddsrt_rbt_node_t *parent, ddsrt_rbt_tree_t *tree)
{
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

static void rb_insert_color(ddsrt_rbt_node_t *node, ddsrt_rbt_tree_t *tree)
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
                rb_set_parent_color(tmp, gparent, RB_BLACK);
                rb_set_parent_color(parent, gparent, RB_BLACK);
                node = gparent;
                parent = rb_parent(node);
                rb_set_parent_color(node, parent, RB_RED);
                continue;
            }

            tmp = parent->cs[1];
            if(node == tmp) {

                tmp = node->cs[0];
                parent->cs[1] = tmp;
                node->cs[0] = parent;
                if(tmp)
                    rb_set_parent_color(tmp, parent, RB_BLACK);
                rb_set_parent_color(parent, node, RB_RED);

                parent = node;
                tmp = node->cs[1];
            }

            gparent->cs[0] = tmp;
            parent->cs[1] = gparent;
            if(tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, tree, RB_RED);
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
                parent = node;
                tmp = node->cs[0];
            }

            gparent->cs[1] = tmp;
            parent->cs[0] = gparent;
            if(tmp)
                rb_set_parent_color(tmp, gparent, RB_BLACK);
            __rb_rotate_set_parents(gparent, parent, tree, RB_RED);
            break;
        }
    }
}

static inline void rebalance_path(ddsrt_rbt_tree_t *tree, ddsrt_rbt_node_t *node)
{
    rb_insert_color(node, tree);
}

void ddsrt_rbt_insert_ipath(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode, ddsrt_rbt_ipath_t *path)
{
    ddsrt_rbt_node_t *node = node_from_onode_nonnull(td, vnode);
    
    assert(path->pnode);
    assert((*path->pnode) == NULL);

    rb_link_node(node, path->parent, path->pnode);
    rebalance_path(tree, node);
}

void ddsrt_rbt_delete_dpath(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *node, ddsrt_rbt_dpath_t *path)
{

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

void ddsrt_rbt_delete(const ddsrt_rbt_treedef_t *td, ddsrt_rbt_tree_t *tree, void *vnode)
{

    tree->count --;
}
