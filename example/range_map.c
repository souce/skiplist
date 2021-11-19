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
    if(NULL != i->min_key && NULL != i->max_key && NULL != j->min_key && NULL != j->max_key){
        if(0 < strncmp(i->min_key, j->max_key, RANGE_MAP_MAX_KEY_LEN)){
            return 1;
        } else if(0 > strncmp(i->max_key, j->min_key, RANGE_MAP_MAX_KEY_LEN)){
            return -1;
        } else {
            return 0; //i->min_key <= j && i->max_key >= j
        }
    }
    return -1; //err
}

struct range_map *range_map_create(){
    struct range_map *m = calloc(1, sizeof(*m));
    if(NULL == m) goto err;
    if(SKIPLIST_OK != skiplist_init((struct skiplist *)m, range_map_pair_cmp)) goto err;
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
    if(SKIPLIST_OK == skiplist_insert((struct skiplist *)m, (struct skiplist_node *)pair)){
        return RANGE_MAP_OK;
    }

err:
    if(NULL != pair){
        free(pair); //do not free key & value
    }
    return RANGE_MAP_ERR;
}

struct range_map_pair *range_map_get(struct range_map *m, void *key){
    struct range_map_pair pair = { .min_key=key, .max_key=key };
    return (struct range_map_pair *)skiplist_search((struct skiplist *)m, (struct skiplist_node *)&pair);
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
    struct range_map_pair *predeleted_pair = (struct range_map_pair *)skiplist_remove((struct skiplist *)m, (struct skiplist_node *)&pair);
    if(NULL != predeleted_pair){
        range_map_pair_free(predeleted_pair);
        return RANGE_MAP_OK;
    }
    return RANGE_MAP_ERR;
}

void range_map_free(struct range_map *m){
    if(NULL == m) return;
    SKIPLIST_FOREACH((struct skiplist *)m, (struct skiplist_node *curr){
        struct range_map_pair *pair = (struct range_map_pair *)curr;
        //printf("del: min_key:%s max_key:%s value:%s\n", pair->min_key, pair->max_key, pair->value);        
        range_map_del(m, pair->min_key);
        return SKIPLIST_OK; //continue
    });
    free(m);
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
    SKIPLIST_FOREACH((struct skiplist *)m, (struct skiplist_node *curr){
        struct range_map_pair *pair = (struct range_map_pair *)curr;
        printf("min_key:%s max_key:%s value:%s\n", pair->min_key, pair->max_key, pair->value);
        return SKIPLIST_OK; //continue     
    });

    //check
    struct range_map_pair *pair = NULL;
    assert(NULL == range_map_get(m, "r"));

    assert(NULL != (pair = range_map_get(m, "g")));
    printf("'g' in range:%s\n", pair->value);

    assert(NULL != (pair = range_map_get(m, "b")));
    printf("'b' in range:%s\n", pair->value);

    assert(NULL == range_map_get(m, "e"));

    assert(NULL != (pair = range_map_get(m, "i")));
    printf("'i' in range:%s\n", pair->value);

    assert(NULL != (pair = range_map_get(m, "m")));
    printf("'m' in range:%s\n", pair->value);

    assert(NULL == range_map_get(m, "q"));
}

int main(){
    struct range_map *m = range_map_create();

    stress_testing(m);
    range_map_free(m);

    printf("over\n");
    return 0;
}
