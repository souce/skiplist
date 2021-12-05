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

struct set_item{
    struct skiplist_node node;
    char *value;
};

struct set{
    struct skiplist sl;
};

static int set_item_cmp(void *k1, void *k2){
    struct set_item *i = (struct set_item *)k1, *j = (struct set_item *)k2;
    return strncmp(i->value, j->value, SET_MAX_KEY_LEN);
}

void set_item_free(struct set_item *item){
    if(NULL != item){
        if(NULL != item->value)
            free(item->value);
        free(item);
    }
}

struct set *set_create(){
    struct set *s = calloc(1, sizeof(*s));
    if(NULL == s) goto err;
    SKIPLIST_INIT((struct skiplist *)s, set_item_cmp);
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
        set_item_free((struct set_item *)deleted_node);
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
    struct set *s = set_create();

    stress_testing(s, SET_MAX_KEY_LEN, 100000);
    cover_testing(s);

    int i = 0;
    struct set_iterator iterator = set_iterator_begin(s);
    struct set_item *curr = NULL;
    while(NULL != (curr = set_iterator_next(&iterator))){
        printf("value:%s\n", curr->value);
        set_del(s, curr->value);
        if(i++ >= 10){
            break;
        }
    }

    //free
    printf("set size before free:%d\n", ((struct skiplist *)s)->busy);
    set_free(s);

    printf("over\n");
    return 0;
}
