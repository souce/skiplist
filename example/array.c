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
    struct skiplist_node node;
    int index;
    char *value;
};

struct array{
    struct skiplist sl;
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
    struct array *a = calloc(1, sizeof(*a));
    if(NULL == a) goto err;
    if(SKIPLIST_OK != skiplist_init((struct skiplist *)a, array_item_cmp)) goto err;
    return a;

err:
    if(NULL != a)
        free(a);
    return NULL;
}

struct array_item *array_get(struct array *a, int index){
    struct array_item item = { .index=index, .value=NULL };
    struct skiplist_node * res_node = skiplist_search((struct skiplist *)a, (struct skiplist_node *)&item);
    if(NULL != res_node){
        return (struct array_item *)res_node;
    }
    return NULL;
}

int array_del(struct array *a, int index){
    struct array_item item = { .index=index, .value=NULL };
    struct skiplist_node * deleted_node = skiplist_remove((struct skiplist *)a, (struct skiplist_node *)&item);
    if(NULL != deleted_node){
        array_item_free((struct array_item *)deleted_node);
        return ARRAY_OK;
    }
    return ARRAY_ERR;
}

int array_set(struct array *a, struct array_item *item){
    array_del(a, item->index); //try deleting before inserting
    return SKIPLIST_OK == skiplist_insert((struct skiplist *)a, (struct skiplist_node *)item) ? ARRAY_OK : ARRAY_ERR;
}

void array_free(struct array *a){
    if(NULL == a) return;
    SKIPLIST_FOREACH((struct skiplist *)a, (struct skiplist_node *curr){
        struct array_item *item = (struct array_item *)curr;
        //printf("index:%d value:%s\n", item->index, item->value);        
        array_del(a, item->index);
        return SKIPLIST_OK; //continue
    });
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
        assert(NULL != item);
        item->index = i;
        assert(NULL != (item->value = random_str(data_len)));
        assert(ARRAY_OK == array_set(a, (void *)item));
        assert(item == array_get(a, item->index));
    }
    printf("time consuming:%ld data_len:%d count:%d\n", getCurrentTime() - start, data_len, count);
}

void cover_testing(struct array *a) __attribute__((unused));
void cover_testing(struct array *a) {
    struct array_item *new_item = calloc(1, sizeof(*new_item));
    assert(NULL != new_item);
    new_item->index = 0;
    assert(NULL != (new_item->value = strdup("test2")));
    assert(ARRAY_OK == array_set(a, (void *)new_item)); //replace
    assert(new_item == array_get(a, 0));
}

int main(){
    struct array *a = array_create();

    stress_testing(a, 32, 100000);
    cover_testing(a);

    //free
    printf("array size before free:%d\n", ((struct skiplist *)a)->busy);
    array_free(a);

    printf("over\n");
    return 0;
}
