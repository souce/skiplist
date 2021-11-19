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
    struct skiplist_node header[0]; //pointer to default_header
    struct skiplist_node default_header;
};

int skiplist_init(struct skiplist *sl, skiplist_cmp_item *cmp_item);
struct skiplist_node *skiplist_search(struct skiplist *sl, struct skiplist_node *node);
int skiplist_insert(struct skiplist *sl, struct skiplist_node *new_node);
struct skiplist_node *skiplist_remove(struct skiplist *sl, struct skiplist_node *del_node);

typedef int skiplist_traversal(struct skiplist_node *curr);
#define SKIPLIST_FOREACH(sl, function_body) \
    do{ \
        skiplist_traversal *traversal = ({ \
            int __nested_func_ptr__ function_body \
            __nested_func_ptr__; \
        }); \
        struct skiplist_node *node = (sl)->header->forward[0]; \
        while(NULL != node){ \
            struct skiplist_node *curr = node; \
            node = node->forward[0]; \
            if(SKIPLIST_OK != traversal(curr)) break; \
        }; \
    }while(0);

#ifdef __cplusplus
}
#endif
#endif
