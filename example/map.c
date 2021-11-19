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
    struct map_pair *predeleted_pair = (struct map_pair *)skiplist_remove((struct skiplist *)m, (struct skiplist_node *)&pair);
    if(NULL != predeleted_pair){
        map_pair_free(predeleted_pair);
        return MAP_OK;
    }
    return MAP_ERR;
}

int map_put(struct map *m, struct map_pair *pair){
    return SKIPLIST_OK == skiplist_insert((struct skiplist *)m, (struct skiplist_node *)pair) ? MAP_OK : MAP_ERR;
}

struct map_pair *map_get(struct map *m, void *key){
    struct map_pair pair = { .key=key, .value=NULL };
    return (struct map_pair *)skiplist_search((struct skiplist *)m, (struct skiplist_node *)&pair);
}

void map_free(struct map *m){
    if(NULL == m) return;
    SKIPLIST_FOREACH((struct skiplist *)m, (struct skiplist_node *curr){
        struct map_pair *pair = (struct map_pair *)curr;
        //printf("key:%s value:%s\n", pair->key, pair->value);        
        map_del(m, pair->key);
        return SKIPLIST_OK; //continue
    });
    free(m);
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
        pair->key = random_str(data_len); //the key and value cannot be the same; otherwise, they will be released twice
        pair->value = random_str(data_len);
        assert(MAP_OK == map_put(m, pair));
        assert(pair == map_get(m, pair->key));
    }
    printf("time consuming:%ld data_len:%d count:%d\n", getCurrentTime() - start, data_len, count);
}

void cover_testing(struct map *m) __attribute__((unused));
void cover_testing(struct map *m) {
    struct map_pair *old_pair = calloc(1, sizeof(*old_pair));
    old_pair->key = strdup("test");
    old_pair->value = strdup("old");

    struct map_pair *new_pair = calloc(1, sizeof(*new_pair));
    new_pair->key = strdup("test");
    new_pair->value = strdup("new");

    assert(MAP_OK == map_put(m, old_pair));
    assert(MAP_ERR == map_put(m, new_pair)); //duplicate values are not allowed to be inserted
    assert(old_pair == map_get(m, (void *)"test"));
    map_pair_free(new_pair);
}

int main(){
    struct map *m = map_create();

    stress_testing(m, MAP_MAX_KEY_LEN, 100000);
    cover_testing(m);

    //free
    printf("map size before free:%d\n", ((struct skiplist *)m)->busy);
    map_free(m);

    printf("over\n");
    return 0;
}
