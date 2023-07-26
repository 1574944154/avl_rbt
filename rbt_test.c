#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <dds/dds.h>

#include "rbtree.h"


struct domain 
{
    struct rb_node node;
    int key;
};

#define MAX_SAMPLES 10000000

int max(int a, int b)
{
    return a>b ? a:b;
}

struct domain *my_search(struct rb_root *root, int key)
{
    struct rb_node *node = root->rb_node;

    while(node)
    {
        struct domain *data = container_of(node, struct domain, node);

        if(data->key > key) node = node->cs[0];
        else if(data->key < key) node = node->cs[1];
        else return data;
        
    }
    return NULL;
}

int my_insert(struct rb_root *root, struct domain *data)
{
    struct rb_node **new = &(root->rb_node), *parent = NULL;

    while(*new)
    {
        struct domain *this = container_of(*new, struct domain, node);

        parent = *new;
        if(data->key < this->key) new = &((*new)->cs[0]);
        else if(data->key > this->key) new = &((*new)->cs[1]);
        else return false;
    }

    rb_link_node(&data->node, parent, new);
    rb_insert_color(&data->node, root);
    
    return true;
}

void preorder(struct rb_node *node)
{
    if(!node) return;

    struct domain *cnode = container_of(node, struct domain, node);

    preorder(node->cs[0]);
    printf("%d ", cnode->key);
    preorder(node->cs[1]);
}

int height(struct rb_node *node)
{
    if(!node) return 0;
    return max(height(node->cs[0]), height(node->cs[1])) + 1;
}

char filename[] = "nums.txt";
void *samples[MAX_SAMPLES];

int main(int argc, char *argv[])
{
    struct rb_root tree_root = RB_ROOT;
    dds_time_t starttime, endtime, sumtime=0;

    printf("sizeof node is %ld\n", sizeof(struct domain));

    FILE *f = fopen(filename, "r");
    int num;
    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        samples[i] = malloc(sizeof(struct domain));
        fscanf(f, "%d ", &num);
        ((struct domain *)samples[i])->key = num;
    }
    endtime = dds_time();
    fclose(f);

    printf("alloc %d time is %ld ms\n", MAX_SAMPLES, (endtime-starttime)/DDS_NSECS_IN_MSEC);

    sumtime += endtime-starttime;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        my_insert(&tree_root, samples[i]);
    }

    endtime = dds_time();

    printf("insert all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

    sumtime += endtime-starttime;

    struct rb_node *node;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        samples[i] = my_search(&tree_root, i);
        assert(samples[i] != NULL);
    }

    endtime = dds_time();

    printf("lookup all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

    sumtime += endtime-starttime;

    starttime = dds_time();

    for(int i=0;i<MAX_SAMPLES;i++)
    {
        // ddsrt_rbt_delete(&treedef, &tree_root, &i);
        rb_erase(samples[MAX_SAMPLES-i-1], &tree_root);
    }

    endtime = dds_time();

    sumtime += endtime-starttime;

    printf("delete all, time is %ld ms\n", (endtime-starttime)/DDS_NSECS_IN_MSEC);

    printf("all time is %ld ms\n", sumtime/DDS_NSECS_IN_MSEC);

    // printf("rbtree's height is %d\n", height(tree_root.rb_node));
    // rb_erase(samples[0], &tree_root);

    return 0;
}