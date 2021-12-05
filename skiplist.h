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

#define SKIPLIST_P 0.25
#define SKIPLIST_RANDOM_LEVEL() \
    ({ \
        int level = 1; \
        while ((random() & 0xFFFF) < (SKIPLIST_P * 0xFFFF)) \
            level += 1; \
        level < SKIPLIST_MAXLEVEL ? level : SKIPLIST_MAXLEVEL; \
    })

#define SKIPLIST_INIT(sl, cmp) \
    do{ \
        (sl)->busy = 0; \
        (sl)->cmp_item = (cmp); \
        *((sl)->header) = (struct skiplist_node){ .forward = {[0 ... SKIPLIST_MAXLEVEL-1] = NULL} }; \
    }while(0)

#define SKIPLIST_TRACK(sl, node, tracks) \
    do{ \
        struct skiplist_node *tmp_node = (sl)->header; \
        int i = SKIPLIST_MAXLEVEL - 1; \
        for(; i >= 0; i--){ \
            while(tmp_node->forward[i] != NULL && 0 > (sl)->cmp_item(tmp_node->forward[i], (node))){ \
                tmp_node = tmp_node->forward[i]; \
            } \
            (tracks)[i] = tmp_node; \
        } \
    }while(0)

#define SKIPLIST_GET(sl, node) \
    ({ \
        struct skiplist_node *res = NULL; \
        struct skiplist_node *tracks[SKIPLIST_MAXLEVEL]; \
        SKIPLIST_TRACK((sl), (node), (tracks)); \
        struct skiplist_node *existing_node = tracks[0]->forward[0]; \
        if(NULL != existing_node && 0 == (sl)->cmp_item(existing_node, (node))){ \
            res = existing_node; \
        } \
        res; \
    })

#define SKIPLIST_PUT(sl, new_node) \
    ({ \
        int res = SKIPLIST_ERR; \
        struct skiplist_node *tracks[SKIPLIST_MAXLEVEL]; \
        SKIPLIST_TRACK((sl), (new_node), tracks); \
        struct skiplist_node *existing_node = tracks[0]->forward[0]; \
        if(NULL == existing_node || 0 != (sl)->cmp_item(existing_node, (new_node))){ \
            int level = SKIPLIST_RANDOM_LEVEL(); \
            int i = 0; \
            for(; i < level; i ++){ \
                (new_node)->forward[i] = tracks[i]->forward[i]; \
                tracks[i]->forward[i] = (new_node); \
            } \
            (sl)->busy += 1; \
            res = SKIPLIST_OK; \
        } \
        res; \
    })

#define SKIPLIST_REMOVE(sl, del_node) \
    ({ \
        struct skiplist_node *res = NULL; \
        struct skiplist_node *tracks[SKIPLIST_MAXLEVEL]; \
        SKIPLIST_TRACK((sl), (del_node), tracks); \
        struct skiplist_node *existing_node = tracks[0]->forward[0]; \
        if(NULL != existing_node && 0 == (sl)->cmp_item(existing_node, (del_node))){ \
            int i = 0; \
            for(; i < SKIPLIST_MAXLEVEL; i++){ \
                if(tracks[i]->forward[i] == existing_node){ \
                    tracks[i]->forward[i] = existing_node->forward[i]; \
                } \
            } \
            (sl)->busy -= 1; \
            res = existing_node; \
        } \
        res; \
    })

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
