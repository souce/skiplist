/*
 * 
 * Copyright (c) 2021, Joel
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <assert.h>

#include "utils.h"
#include "skiplist.h"

///////////////////////////////////////////////////////////////////////////////
// set
///////////////////////////////////////////////////////////////////////////////
#define SET_OK 0
#define SET_ERR -1
#define SET_MAX_KEY_LEN 32

void set_value_free(char *value){
    if(NULL != value){
        free(value);
    }
}

struct set_item{
    struct skiplist_node node;
    char *value;
};

typedef void value_free(char *value);
struct set{
    struct skiplist sl;
    value_free *value_free;
};

static int set_item_cmp(void *k1, void *k2){
    struct set_item *i = (struct set_item *)k1, *j = (struct set_item *)k2;
    return strncmp(i->value, j->value, SET_MAX_KEY_LEN);
}

void set_item_free(struct set *s, struct set_item *item){
    if(NULL != item){
        if(NULL != s->value_free){
            s->value_free(item->value);
        }
        free(item);
    }
}

struct set *set_create(value_free *value_free){
    struct set *s = calloc(1, sizeof(*s));
    if(NULL == s) goto err;
    SKIPLIST_INIT((struct skiplist *)s, set_item_cmp);
    s->value_free = value_free;
    return s;

err:
    if(NULL != s)
        free(s);
    return NULL;
}

int set_del(struct set *s, void *value){
    struct set_item item = { .value=value };
    struct skiplist_node *deleted_node = SKIPLIST_REMOVE((struct skiplist *)s, (struct skiplist_node *)&item);
    if(NULL != deleted_node){
        set_item_free(s, (struct set_item *)deleted_node);
        return SET_OK;
    }
    return SET_ERR;
}

int set_put(struct set *s, struct set_item *item){
    return SKIPLIST_OK == SKIPLIST_PUT((struct skiplist *)s, (struct skiplist_node *)item) ? SET_OK : SET_ERR;
}

int set_contains(struct set *s, void *value){
    struct set_item item = { .value=value };
    struct skiplist_node *res_node = SKIPLIST_GET((struct skiplist *)s, (struct skiplist_node *)&item);
    if(NULL != res_node){
        return SET_OK;
    }
    return SET_ERR;
}

void set_free(struct set *s){
    if(NULL == s) return;
    struct skiplist_node *iter = (struct skiplist_node *)&(((struct skiplist *)s)->header);
    SKIPLIST_FOREACH((struct skiplist *)s, iter, {
        struct set_item *item = (struct set_item *)iter;
        //printf("value:%s\n", item->value);        
        set_del(s, item->value);
    });
    free(s);
}

int set_intersection(struct set *dst_s, struct set *src_s1, struct set *src_s2){
    struct skiplist_node *s1_iter = (struct skiplist_node *)&(((struct skiplist *)src_s1)->header);
    SKIPLIST_FOREACH((struct skiplist *)src_s1, s1_iter, {
        struct set_item *item = (struct set_item *)s1_iter;   
        if(SET_OK == set_contains(src_s2, item->value) && SET_OK != set_contains(dst_s, item->value)){
            struct set_item *new_item = calloc(1, sizeof(struct set_item));
            new_item->value = item->value;
            if(SET_OK != set_put(dst_s, new_item)){
                return SET_ERR;
            }
        }
    });
    return SET_OK;
}

int set_difference(struct set *dst_s, struct set *src_s1, struct set *src_s2){
    struct skiplist_node *s1_iter = (struct skiplist_node *)&(((struct skiplist *)src_s1)->header);
    SKIPLIST_FOREACH((struct skiplist *)src_s1, s1_iter, {
        struct set_item *item = (struct set_item *)s1_iter;
        if(SET_OK != set_contains(src_s2, item->value) && SET_OK != set_contains(dst_s, item->value)){
            struct set_item *new_item = calloc(1, sizeof(struct set_item));
            new_item->value = item->value;
            if(SET_OK != set_put(dst_s, new_item)){
                return SET_ERR;
            }
        }
    });
    return SET_OK;
}

int set_union(struct set *dst_s, struct set *src_s1, struct set *src_s2){
    struct skiplist_node *s1_iter = (struct skiplist_node *)&(((struct skiplist *)src_s1)->header);
    SKIPLIST_FOREACH((struct skiplist *)src_s1, s1_iter, {
        struct set_item *item = (struct set_item *)s1_iter;
        if(SET_OK != set_contains(dst_s, item->value)){
            struct set_item *new_item = calloc(1, sizeof(struct set_item));
            new_item->value = item->value;
            if(SET_OK != set_put(dst_s, new_item)){
                return SET_ERR;
            }
        }
    });

    struct skiplist_node *s2_iter = (struct skiplist_node *)&(((struct skiplist *)src_s2)->header);
    SKIPLIST_FOREACH((struct skiplist *)src_s2, s2_iter, {
        struct set_item *item = (struct set_item *)s2_iter;
        if(SET_OK != set_contains(dst_s, item->value)){
            struct set_item *new_item = calloc(1, sizeof(struct set_item));
            new_item->value = item->value;
            if(SET_OK != set_put(dst_s, new_item)){
                return SET_ERR;
            }
        }
    });
    return SET_OK;
}

//iterator
struct set_iterator{
    struct set_item *node_curr;
};

struct set_iterator set_iterator_begin(struct set *s){
    struct set_item *start_node = (struct set_item *)((struct skiplist *)s)->header->forward[0];
    return (struct set_iterator){ .node_curr = start_node };
}

struct set_item *set_iterator_next(struct set_iterator *iter){
    struct skiplist_node *node_ref = (struct skiplist_node *)iter->node_curr;
    iter->node_curr = (struct set_item *)(NULL != node_ref ? node_ref->forward[0] : NULL);
    return (struct set_item *)node_ref;
}

///////////////////////////////////////////////////////////////////////////////
// test
///////////////////////////////////////////////////////////////////////////////
void stress_testing(struct set *s, int data_len, int count) __attribute__((unused));
void stress_testing(struct set *s, int data_len, int count) {
    int64_t start = getCurrentTime();
    int i = 0;
    for(; i < count; i++){
        struct set_item *item = calloc(1, sizeof(*item));
        assert(NULL != (item->value = random_str(data_len)));
        assert(SET_OK == set_put(s, item));
    }
    printf("time consuming:%ld data_len:%d count:%d\n", getCurrentTime() - start, data_len, count);
}

void cover_testing(struct set *s) __attribute__((unused));
void cover_testing(struct set *s) {
    struct set_item *test_item = calloc(1, sizeof(*test_item));
    assert(NULL != (test_item->value = strdup("old")));

    assert(SET_OK == set_put(s, test_item));
    assert(SET_ERR == set_put(s, test_item)); //duplicate values are not allowed to be inserted
    assert(SET_OK == set_contains(s, (void *)"old"));
}

int main(){
    struct set *s = set_create(set_value_free);

    stress_testing(s, SET_MAX_KEY_LEN, 100000);
    cover_testing(s);
    
    //free
    int i = 0;
    struct set_iterator iterator = set_iterator_begin(s);
    struct set_item *curr = NULL;
    while(NULL != (curr = set_iterator_next(&iterator))){
        printf("value:%s\n", curr->value);
        set_del(s, curr->value);
        if(30 < i++){
            break;
        }
    }

    //set_intersection & set_difference & set_union
    struct set *s1 = set_create(NULL);
    set_put(s1, &(struct set_item){ .value = "A" });
    set_put(s1, &(struct set_item){ .value = "B" });
    set_put(s1, &(struct set_item){ .value = "C" });

    struct set *s2 = set_create(NULL);
    set_put(s2, &(struct set_item){ .value = "B" });
    set_put(s2, &(struct set_item){ .value = "C" });
    set_put(s2, &(struct set_item){ .value = "D" });

    struct set *s_i = set_create(NULL);
    set_intersection(s_i, s1, s2);
    printf("set_intersection:\n");
    struct set_iterator s_i_iterator = set_iterator_begin(s_i);
    struct set_item *s_i_curr = NULL;
    while(NULL != (s_i_curr = set_iterator_next(&s_i_iterator))){
        printf("value:%s\n", s_i_curr->value);
    }

    struct set *s_d = set_create(NULL);
    set_difference(s_d, s1, s2);
    printf("set_difference:\n");
    struct set_iterator s_d_iterator = set_iterator_begin(s_d);
    struct set_item *s_d_curr = NULL;
    while(NULL != (s_d_curr = set_iterator_next(&s_d_iterator))){
        printf("value:%s\n", s_d_curr->value);
    }

    struct set *s_u = set_create(NULL);
    set_union(s_u, s1, s2);
    printf("set_union:\n");
    struct set_iterator s_u_iterator = set_iterator_begin(s_u);
    struct set_item *s_u_curr = NULL;
    while(NULL != (s_u_curr = set_iterator_next(&s_u_iterator))){
        printf("value:%s\n", s_u_curr->value);
    }

    //free
    printf("set size before free:%d\n", ((struct skiplist *)s)->busy);
    set_free(s);
    free(s1); //do not clear items
    free(s2); //do not clear items
    set_free(s_i);
    set_free(s_d);
    set_free(s_u);

    printf("over\n");
    return 0;
}
