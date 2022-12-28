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
    #define SKIPLIST_MAXLEVEL 8

    struct skiplist_node{
        int level;
        struct skiplist_node *prev[SKIPLIST_MAXLEVEL];
        struct skiplist_node *next[SKIPLIST_MAXLEVEL];
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

    #define SKIPLIST_TRACK(sl, node, tracks) \
        do{ \
            struct skiplist_node *tmp_node = (sl)->header; \
            int i = SKIPLIST_MAXLEVEL - 1; \
            for(; i >= 0; i--){ \
                while(tmp_node->next[i] != (sl)->header && \
                        0 > (sl)->cmp_item(tmp_node->next[i], (node))){ \
                    tmp_node = tmp_node->next[i]; \
                } \
                (tracks)[i] = tmp_node; \
            } \
        }while(0)

    #define SKIPLIST_FOREACH_PREV(sl, pos, iter) \
        for ((pos) = (sl)->header->prev[0], (iter) = (pos)->prev[0]; \
            (pos) != (sl)->header; \
            (pos) = (iter), (iter) = (pos)->prev[0])

    #define SKIPLIST_FOREACH_NEXT(sl, pos, iter) \
        for ((pos) = (sl)->header->next[0], (iter) = (pos)->next[0]; \
            (pos) != (sl)->header; \
            (pos) = (iter), (iter) = (pos)->next[0])

    static inline void skiplist_init(struct skiplist *sl, skiplist_cmp_item *cmp_item){
        sl->busy = 0;
        sl->cmp_item = cmp_item;
        *(sl->header) = (struct skiplist_node){
                            .prev = {[0 ... SKIPLIST_MAXLEVEL-1] = sl->header},
                            .next = {[0 ... SKIPLIST_MAXLEVEL-1] = sl->header}
                        };
    }

    static inline int skiplist_put(struct skiplist *sl, struct skiplist_node *new_node){
        struct skiplist_node *tracks[SKIPLIST_MAXLEVEL] = { 0, };
        SKIPLIST_TRACK(sl, new_node, tracks);
        struct skiplist_node *existing_node = tracks[0]->next[0];
        if(sl->header == existing_node || 0 != sl->cmp_item(existing_node, new_node)){
            new_node->level = SKIPLIST_RANDOM_LEVEL();
            int i = 0;
            for(; i < new_node->level; i ++){
                struct skiplist_node *p = tracks[i], *n = p->next[i];
                p->next[i] = n->prev[i] = new_node;
                new_node->prev[i] = p;
                new_node->next[i] = n;
            }
            sl->busy += 1;
            return SKIPLIST_OK;
        }
        return SKIPLIST_ERR;
    }

    static inline struct skiplist_node *skiplist_get(struct skiplist *sl, struct skiplist_node *node){
        struct skiplist_node *res = NULL;
        struct skiplist_node *tracks[SKIPLIST_MAXLEVEL] = { 0, };
        SKIPLIST_TRACK(sl, node, tracks);
        struct skiplist_node *existing_node = tracks[0]->next[0];
        if(sl->header != existing_node && 0 == sl->cmp_item(existing_node, node)){
            res = existing_node;
        }
        return res;
    }

    static inline int skiplist_del(struct skiplist *sl, struct skiplist_node *del_node){
        if(sl->header != del_node){
            int i = 0;
            for(; i < del_node->level; i++){
                struct skiplist_node *p = del_node->prev[i], *n = del_node->next[i];
                p->next[i] = n;
                n->prev[i] = p;
            }
            sl->busy -= 1;
            return SKIPLIST_OK;
        }
        return SKIPLIST_ERR;
    }

#ifdef __cplusplus
}
#endif
#endif
