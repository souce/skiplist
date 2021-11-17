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

struct skiplist_node{
    void *item;
    struct skiplist_node *forward[0];
};

typedef int skiplist_cmp_item(void *k1, void *k2);
struct skiplist{
    int busy;
    struct skiplist_node *header;
    skiplist_cmp_item *cmp_item;
};

struct skiplist* skiplist_create(skiplist_cmp_item *key_cmp);
void skiplist_free(struct skiplist *sl);
void *skiplist_search(struct skiplist *sl, void *item);
int skiplist_insert(struct skiplist *sl, void *item);
void *skiplist_remove(struct skiplist *sl, void *item);

typedef void skiplist_traversal(void *curr);
#define SKIPLIST_FOREACH(sl, function_body) \
    do{ \
        skiplist_traversal *traversal = ({ \
            void __nested_func_ptr__ function_body \
            __nested_func_ptr__; \
        }); \
        struct skiplist_node *node = (sl)->header->forward[0]; \
        while(NULL != node){ \
            struct skiplist_node *curr = node; \
            node = node->forward[0]; \
            traversal((void *)(curr->item)); \
        }; \
    }while(0);

#ifdef __cplusplus
}
#endif
#endif
