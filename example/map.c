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
// map
///////////////////////////////////////////////////////////////////////////////
#define MAP_OK 0
#define MAP_ERR -1
#define MAP_MAX_KEY_LEN 32

struct map_pair{
    struct skiplist_node node;
    char *key;
    char *value;
};

struct map{
    struct skiplist sl;
};

static int map_pair_cmp(void *k1, void *k2){
    struct map_pair *i = (struct map_pair *)k1, *j = (struct map_pair *)k2;
    return strncmp(i->key, j->key, MAP_MAX_KEY_LEN);
}

void map_pair_free(struct map_pair *pair){
    if(NULL != pair){
        if(NULL != pair->key)
            free(pair->key);
        if(NULL != pair->value)
            free(pair->value);
        free(pair);
    }
}

struct map *map_create(){
    struct map *m = calloc(1, sizeof(*m));
    if(NULL == m) goto err;
    if(SKIPLIST_OK != skiplist_init((struct skiplist *)m, map_pair_cmp)) goto err;
    return m;

err:
    if(NULL != m)
        free(m);
    return NULL;
}

int map_del(struct map *m, void *key){
    struct map_pair pair = { .key=key, .value=NULL };
    struct skiplist_node *deleted_node = skiplist_remove((struct skiplist *)m, (struct skiplist_node *)&pair);
    if(NULL != deleted_node){
        map_pair_free((struct map_pair *)deleted_node);
        return MAP_OK;
    }
    return MAP_ERR;
}

int map_put(struct map *m, struct map_pair *pair){
    return SKIPLIST_OK == skiplist_insert((struct skiplist *)m, (struct skiplist_node *)pair) ? MAP_OK : MAP_ERR;
}

struct map_pair *map_get(struct map *m, void *key){
    struct map_pair pair = { .key=key, .value=NULL };
    struct skiplist_node *res_node = skiplist_search((struct skiplist *)m, (struct skiplist_node *)&pair);
    if(NULL != res_node){
        return (struct map_pair *)res_node;
    }
    return NULL;
}

void map_free(struct map *m){
    if(NULL == m) return;
    struct skiplist_node *iter = (struct skiplist_node *)&(((struct skiplist *)m)->header);
    SKIPLIST_FOREACH((struct skiplist *)m, iter, {
        struct map_pair *pair = (struct map_pair *)iter;
        //printf("key:%s value:%s\n", pair->key, pair->value);        
        map_del(m, pair->key);
    });
    free(m);
}

//iterator
struct map_iterator{
    struct map_pair *node_curr;
};

struct map_iterator map_iterator_begin(struct map *m, char *key){
    struct map_pair pair = (struct map_pair){ .key = key };
    struct map_pair *start_node = (struct map_pair *)skiplist_search((struct skiplist *)m, (struct skiplist_node *)&pair);
    return (struct map_iterator){ .node_curr = start_node };
}

struct map_pair *map_iterator_next(struct map_iterator *iter){
    struct skiplist_node *node_ref = (struct skiplist_node *)iter->node_curr;
    iter->node_curr = (struct map_pair *)(NULL != node_ref ? node_ref->forward[0] : NULL);
    return (struct map_pair *)node_ref;
}

///////////////////////////////////////////////////////////////////////////////
// test
///////////////////////////////////////////////////////////////////////////////
void stress_testing(struct map *m, int data_len, int count) __attribute__((unused));
void stress_testing(struct map *m, int data_len, int count) {
    int64_t start = getCurrentTime();
    int i = 0;
    for(; i < count; i++){
        struct map_pair *pair = calloc(1, sizeof(*pair));
        assert(NULL != (pair->key = random_str(data_len))); //the key and value cannot be the same; otherwise, they will be released twice
        assert(NULL != (pair->value = random_str(data_len)));
        assert(MAP_OK == map_put(m, pair));
        assert(pair == map_get(m, pair->key));
    }
    printf("time consuming:%ld data_len:%d count:%d\n", getCurrentTime() - start, data_len, count);
}

void cover_testing(struct map *m) __attribute__((unused));
void cover_testing(struct map *m) {
    struct map_pair *old_pair = calloc(1, sizeof(*old_pair));
    assert(NULL != (old_pair->key = strdup("test")));
    assert(NULL != (old_pair->value = strdup("old")));

    struct map_pair *new_pair = calloc(1, sizeof(*new_pair));
    assert(NULL != (new_pair->key = strdup("test")));
    assert(NULL != (new_pair->value = strdup("new")));

    assert(MAP_OK == map_put(m, old_pair));
    assert(MAP_ERR == map_put(m, new_pair)); //duplicate values are not allowed to be inserted
    assert(old_pair == map_get(m, (void *)"test"));
    map_pair_free(new_pair);
}

int main(){
    struct map *m = map_create();

    stress_testing(m, MAP_MAX_KEY_LEN, 100000);
    cover_testing(m);

    int i = 0;
    struct map_iterator iterator = map_iterator_begin(m, "test");
    struct map_pair *curr = NULL;
    while(NULL != (curr = map_iterator_next(&iterator))){
        printf("key:%s value:%s\n", curr->key, curr->value);
        map_del(m, curr->key);
        if(i++ >= 10){
            break;
        }
    }

    //free
    printf("map size before free:%d\n", ((struct skiplist *)m)->busy);
    map_free(m);

    printf("over\n");
    return 0;
}
