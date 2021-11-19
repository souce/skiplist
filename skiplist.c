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

#include <stdlib.h>
#include <stdio.h>

#include "skiplist.h"

#define SKIPLIST_P 0.25
static int random_level() {
    int level = 1;
    while ((random() & 0xFFFF) < (SKIPLIST_P * 0xFFFF))
        level += 1;
    return level < SKIPLIST_MAXLEVEL ? level : SKIPLIST_MAXLEVEL;
}

int skiplist_init(struct skiplist *sl, skiplist_cmp_item *cmp_item){
    if(NULL == cmp_item){
        goto err;
    }
    sl->busy = 0;
    sl->cmp_item = cmp_item;
    return SKIPLIST_OK;
    
err:
    return SKIPLIST_ERR;
}

static inline void search(struct skiplist *sl, struct skiplist_node *node, struct skiplist_node **tracks){
    struct skiplist_node *tmp_node = sl->header;
    int i = SKIPLIST_MAXLEVEL - 1; //the index starts at 0, so minus 1
    for(; i >= 0; i--){
        while(tmp_node->forward[i] != NULL && 0 > sl->cmp_item(tmp_node->forward[i], node)){
            tmp_node = tmp_node->forward[i];
        }
        tracks[i] = tmp_node; //record the path of each layer
    }
}

struct skiplist_node *skiplist_search(struct skiplist *sl, struct skiplist_node *node){
    struct skiplist_node *tracks[SKIPLIST_MAXLEVEL];
    search(sl, node, tracks);
    struct skiplist_node *existing_node = tracks[0]->forward[0];
    if(NULL != existing_node && 0 == sl->cmp_item(existing_node, node)){
        return existing_node;
    }
    //node not found
    return NULL;
}

int skiplist_insert(struct skiplist *sl, struct skiplist_node *new_node){
    struct skiplist_node *tracks[SKIPLIST_MAXLEVEL];
    search(sl, new_node, tracks);
    struct skiplist_node *existing_node = tracks[0]->forward[0];
    if(NULL != existing_node && 0 == sl->cmp_item(existing_node, new_node)){
        goto err; //no repetition allowed
    }else{
        int level = random_level();
        int i = 0;
        for(; i < level; i ++){
            new_node->forward[i] = tracks[i]->forward[i];
            tracks[i]->forward[i] = new_node;
        }
        sl->busy += 1;
    }
    return SKIPLIST_OK;

err:
    return SKIPLIST_ERR;
}

struct skiplist_node *skiplist_remove(struct skiplist *sl, struct skiplist_node *del_node){
    struct skiplist_node *tracks[SKIPLIST_MAXLEVEL];
    search(sl, del_node, tracks);
    struct skiplist_node *existing_node = tracks[0]->forward[0];
    if(NULL != existing_node && 0 == sl->cmp_item(existing_node, del_node)){
        int i = 0;
        for(; i < SKIPLIST_MAXLEVEL; i++){ //remove the deleted node from the linked list
            if(tracks[i]->forward[i] == existing_node){
                tracks[i]->forward[i] = existing_node->forward[i];
            }
        }
        sl->busy -= 1;
        return existing_node; //returns the item of the deleted node to the user for cleansing
    }
    //node not found
    return NULL;
}
