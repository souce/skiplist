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
// array
///////////////////////////////////////////////////////////////////////////////
#define ARRAY_OK 0
#define ARRAY_ERR -1

struct array_item{
    int index;
    char *value;
};

struct array{
    struct skiplist *sl;
};

static int array_item_cmp(void *k1, void *k2){
    struct array_item *i = (struct array_item *)k1, *j = (struct array_item *)k2;
    return i->index - j->index;
}

static void array_item_free(struct array_item *item){
    if(NULL != item){
        if(NULL != item->value)
            free(item->value);
        free(item);
    }
}

struct array *array_create(){
    struct array *a = NULL;
    struct skiplist *sl = NULL;

    a = calloc(1, sizeof(*a));
    if(NULL == a) goto err;
    sl = skiplist_create(array_item_cmp);
    if(NULL == sl) goto err;
    a->sl = sl;
    return a;

err:
    if(NULL != sl)
        skiplist_free(sl);
    if(NULL != a)
        free(a);
    return NULL;
}

int array_del(struct array *a, int index){
    struct array_item item = { .index=index, .value=NULL };
    struct array_item *predeleted_item = skiplist_remove(a->sl, (void *)&item);
    if(NULL != predeleted_item){
        array_item_free(predeleted_item);
        return ARRAY_OK;
    }
    return ARRAY_ERR;
}

struct array_item *array_get(struct array *a, int index){
    struct array_item item = { .index=index, .value=NULL };
    return skiplist_search(a->sl, (void *)&item);
}

int array_set(struct array *a, struct array_item *item){
    array_del(a, item->index); //delete before insert
    return SKIPLIST_OK == skiplist_insert(a->sl, (void *)item) ? ARRAY_OK : ARRAY_ERR;
}

void array_free(struct array *a){
    if(NULL == a) return;
    SKIPLIST_FOREACH(a->sl, (void *curr){
        struct array_item *item = (struct array_item *)curr;
        //printf("index:%d value:%s\n", item->index, item->value);        
        array_del(a, item->index);
    });
    skiplist_free(a->sl);
    free(a);
}

///////////////////////////////////////////////////////////////////////////////
// test
///////////////////////////////////////////////////////////////////////////////
void stress_testing(struct array *a, int data_len, int count) __attribute__((unused));
void stress_testing(struct array *a, int data_len, int count) {
    int64_t start = getCurrentTime();
    int i = 0;
    for(; i < count; i++){
        struct array_item *item = calloc(1, sizeof(*item));
        item->index = i;
        item->value = random_str(data_len);
        assert(ARRAY_OK == array_set(a, (void *)item));
        assert(item == array_get(a, item->index));
    }
    printf("time consuming:%ld data_len:%d count:%d\n", getCurrentTime() - start, data_len, count);
}

void cover_testing(struct array *a) __attribute__((unused));
void cover_testing(struct array *a) {
    char *new_key = calloc(1, 6);
    strcpy(new_key, "test2");
    struct array_item *new_item = calloc(1, sizeof(*new_item));
    new_item->index = 0;
    new_item->value = new_key;
    assert(ARRAY_OK == array_set(a, (void *)new_item));
    assert(new_item == array_get(a, 0));
}

int main(){
    struct array *a = array_create();

    stress_testing(a, 32, 100000);
    cover_testing(a);

    //free
    printf("array size before free:%d\n", a->sl->busy);
    array_free(a);

    printf("over\n");
    return 0;
}
