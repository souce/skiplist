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

#ifndef __SKIPLIST__
#define __SKIPLIST__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#define SKIPLIST_OK 0
#define SKIPLIST_ERR -1
#define SKIPLIST_MAXLEVEL 16

struct skiplist_node{
    struct skiplist_node *forward[SKIPLIST_MAXLEVEL];
};

typedef int skiplist_cmp_item(void *k1, void *k2);
struct skiplist{
    int busy;
    skiplist_cmp_item *cmp_item;
    struct skiplist_node header[1];
};

int skiplist_init(struct skiplist *sl, skiplist_cmp_item *cmp_item);
struct skiplist_node *skiplist_search(struct skiplist *sl, struct skiplist_node *node);
int skiplist_insert(struct skiplist *sl, struct skiplist_node *new_node);
struct skiplist_node *skiplist_remove(struct skiplist *sl, struct skiplist_node *del_node);
#define SKIPLIST_FOREACH(sl, iter, loop_body) \
    do{ \
        while(NULL != (iter)){ \
            struct skiplist_node *iter_next = (iter)->forward[0]; \
            if((sl)->header != (iter)){ \
                (loop_body); \
            } \
            (iter) = iter_next; \
        }; \
    }while(0);

#ifdef __cplusplus
}
#endif
#endif
