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
// range_map
///////////////////////////////////////////////////////////////////////////////
#define RANGE_MAP_OK 0
#define RANGE_MAP_ERR -1
#define RANGE_MAP_MAX_KEY_LEN 32

struct range_map_pair{
    struct skiplist_node node;
    char *min_key;
    char *max_key;
    char *value;
};

struct range_map{
    struct skiplist sl;
};

static int range_map_pair_cmp(void *k1, void *k2){
    struct range_map_pair *i = (struct range_map_pair *)k1, *j = (struct range_map_pair *)k2;
    if(0 < strncmp(i->min_key, j->max_key, RANGE_MAP_MAX_KEY_LEN)){
        return 1;
    } else if(0 > strncmp(i->max_key, j->min_key, RANGE_MAP_MAX_KEY_LEN)){
        return -1;
    }
    return 0; //i->min_key <= j->max_key && i->max_key >= j->min_key
}

struct range_map *range_map_create(){
    struct range_map *m = calloc(1, sizeof(*m));
    if(NULL == m) goto err;
    SKIPLIST_INIT((struct skiplist *)m, range_map_pair_cmp);
    return m;

err:
    if(NULL != m)
        free(m);
    return NULL;
}

int range_map_put(struct range_map *m, char *min_key, char *max_key, char *value){
    struct range_map_pair *pair = NULL;
    if(0 < strncmp(min_key, max_key, RANGE_MAP_MAX_KEY_LEN)) goto err;
    
    pair = calloc(1, sizeof(*pair));
    if(NULL == pair) goto err;
    pair->min_key = min_key;
    pair->max_key = max_key;
    pair->value = value;
    if(SKIPLIST_OK == SKIPLIST_PUT((struct skiplist *)m, (struct skiplist_node *)pair)){
        return RANGE_MAP_OK;
    }

err:
    if(NULL != pair){
        free(pair); //do not free key & value
    }
    return RANGE_MAP_ERR;
}

void range_map_pair_free(struct range_map_pair *pair){
    if(NULL != pair){
        if(NULL != pair->min_key)
            free(pair->min_key);
        if(NULL != pair->max_key)
            free(pair->max_key);
        if(NULL != pair->value)
            free(pair->value);
        free(pair);
    }
}

int range_map_del(struct range_map *m, void *key){
    struct range_map_pair pair = { .min_key = key, .max_key = key };
    struct skiplist_node *deleted_node = SKIPLIST_REMOVE((struct skiplist *)m, (struct skiplist_node *)&pair);
    if(NULL != deleted_node){
        range_map_pair_free((struct range_map_pair *)deleted_node);
        return RANGE_MAP_OK;
    }
    return RANGE_MAP_ERR;
}

void range_map_free(struct range_map *m){
    if(NULL == m) return;
    struct skiplist_node *iter = (struct skiplist_node *)&(((struct skiplist *)m)->header);
    SKIPLIST_FOREACH((struct skiplist *)m, iter, {
        struct range_map_pair *pair = (struct range_map_pair *)iter;
        //printf("del: min_key:%s max_key:%s value:%s\n", pair->min_key, pair->max_key, pair->value);        
        range_map_del(m, pair->min_key);
    });
    free(m);
}

//iterator
struct range_map_iterator{
    struct range_map_pair *node_curr;
    struct range_map_pair range_compare_pair;
};

struct range_map_iterator range_map_iterator_begin(struct range_map *m, char *min_key, char *max_key){
    struct range_map_pair pair = (struct range_map_pair){ .min_key = min_key, .max_key = max_key };
    struct range_map_pair *start_node = (struct range_map_pair *)SKIPLIST_GET((struct skiplist *)m, (struct skiplist_node *)&pair);
    return (struct range_map_iterator){ .node_curr = start_node, .range_compare_pair = pair };
}

struct range_map_pair *range_map_iterator_next(struct range_map_iterator *iter){
    struct skiplist_node *node_ref = (struct skiplist_node *)iter->node_curr;
    iter->node_curr = NULL;
    if(NULL != node_ref){
        struct skiplist_node *next_node = node_ref->forward[0];
        if(NULL != next_node && 0 <= range_map_pair_cmp(&(iter->range_compare_pair), next_node)){
            iter->node_curr = (struct range_map_pair *)next_node;
        }
    }
    return (struct range_map_pair *)node_ref;
}

///////////////////////////////////////////////////////////////////////////////
// test
///////////////////////////////////////////////////////////////////////////////
void stress_testing(struct range_map *m) __attribute__((unused));
void stress_testing(struct range_map *m) {
    assert(RANGE_MAP_OK == range_map_put(m, strdup("lll"), strdup("ooo"), strdup("l~o")));
    assert(RANGE_MAP_OK == range_map_put(m, strdup("aaa"), strdup("ddd"), strdup("a~d")));
    assert(RANGE_MAP_OK == range_map_put(m, strdup("fff"), strdup("jjj"), strdup("f~j")));
    assert(RANGE_MAP_OK == range_map_put(m, strdup("www"), strdup("zzz"), strdup("w~z")));
    assert(RANGE_MAP_OK == range_map_put(m, strdup("uuu"), strdup("vvv"), strdup("u~v")));

    //check sorted skiplist
    struct skiplist_node *iter = (struct skiplist_node *)&(((struct skiplist *)m)->header);
    SKIPLIST_FOREACH((struct skiplist *)m, iter, {
        struct range_map_pair *pair = (struct range_map_pair *)iter;
        printf("min_key:%s max_key:%s value:%s\n", pair->min_key, pair->max_key, pair->value);
    });

    //check
    assert(NULL == range_map_iterator_begin(m, "r", "r").node_curr);
    assert(NULL == range_map_iterator_begin(m, "e", "e").node_curr);

    struct range_map_iterator iterator = range_map_iterator_begin(m, "g", "g");
    struct range_map_pair *curr = NULL;
    while(NULL != (curr = range_map_iterator_next(&iterator))){
        printf("'g' in range:%s\n", curr->value);
    }

    iterator = range_map_iterator_begin(m, "b", "b");
    curr = NULL;
    while(NULL != (curr = range_map_iterator_next(&iterator))){
        printf("'b' in range:%s\n", curr->value);
    }

    iterator = range_map_iterator_begin(m, "i", "z");
    curr = NULL;
    while(NULL != (curr = range_map_iterator_next(&iterator))){
        printf("'i-z' in range:%s\n", curr->value);
    }
}

int main(){
    struct range_map *m = range_map_create();

    stress_testing(m);
    range_map_free(m);

    printf("over\n");
    return 0;
}
