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
#define SKIPLIST_MAXLEVEL 32
static int random_level() {
    int level = 1;
    while ((random() & 0xFFFF) < (SKIPLIST_P * 0xFFFF))
        level += 1;
    return level < SKIPLIST_MAXLEVEL ? level : SKIPLIST_MAXLEVEL;
}

static struct skiplist_node* create_node(int level, void *item){
    struct skiplist_node *node = (struct skiplist_node*)calloc(1, sizeof(*node) + (sizeof(node) * level));
    if(NULL == node){
        return NULL;
    }
    node->item = item;
    return node;
}

static void free_node(struct skiplist_node *node){
    if(NULL != node){
        free(node); //free a node does not reclaim its item
    }
}

static void search(struct skiplist *sl, void *item, struct skiplist_node **tracks){
    struct skiplist_node *tmp_node = NULL;
    tmp_node = sl->header;
    int i = SKIPLIST_MAXLEVEL - 1; //the index starts at 0, so minus 1
    for(; i >= 0; i--){
        while(tmp_node->forward[i] != NULL && 0 > sl->cmp_item(tmp_node->forward[i]->item, item)){
            tmp_node = tmp_node->forward[i];
        }
        tracks[i] = tmp_node; //record the path of each layer
    }
}

struct skiplist* skiplist_create(skiplist_cmp_item *cmp_item){
    struct skiplist *sl = NULL;

    if(NULL == cmp_item){
        goto err;
    }
    
    sl = (struct skiplist*)calloc(1, sizeof(*sl));
    if(NULL == sl){
        goto err;
    }
    sl->busy = 0;
    sl->cmp_item = cmp_item;
    sl->header = create_node(SKIPLIST_MAXLEVEL, NULL); //default header
    if(NULL == sl->header){
        goto err;
    }
    return sl;
    
err:
    skiplist_free(sl);
    return NULL;
}

void skiplist_free(struct skiplist *sl){
    if(NULL != sl){
        struct skiplist_node *node = sl->header;
        while(NULL != node){
            struct skiplist_node *curr = node;
            node = node->forward[0];
            free_node(curr); //free a node does not reclaim its item
        };
        sl->busy = 0;
        free(sl);
    }
}

void *skiplist_search(struct skiplist *sl, void *item){
    struct skiplist_node *tracks[SKIPLIST_MAXLEVEL];

    if(NULL == item){
        goto err; //only the default header item can be NULL, and it cannot be found
    }

    search(sl, item, tracks);
    struct skiplist_node *node = tracks[0]->forward[0];
    if(NULL != node && NULL != node->item && 0 == sl->cmp_item(item, node->item)){
        return node->item;
    }
    //node not found

err:
    return NULL;
}

int skiplist_insert(struct skiplist *sl, void *item){
    struct skiplist_node *existing_node = NULL;
    struct skiplist_node *new_node = NULL;
    struct skiplist_node *tracks[SKIPLIST_MAXLEVEL];

    if(NULL == item){
        goto err; //only the default header item can be NULL
    }

    search(sl, item, tracks);
    existing_node = tracks[0]->forward[0];
    if(NULL != existing_node && NULL != existing_node->item && 0 == sl->cmp_item(existing_node->item, item)){
        goto err; //no repetition allowed
    }else{
        //create a node to save data
        int level = random_level();
        new_node = create_node(level, item);
        if(NULL == new_node){
            goto err;
        }
        int i = 0;
        for(; i < level; i ++){
            new_node->forward[i] = tracks[i]->forward[i];
            tracks[i]->forward[i] = new_node;
        }
        sl->busy += 1;
    }
    return SKIPLIST_OK;

err:
    free_node(new_node);
    return SKIPLIST_ERR;
}

void *skiplist_remove(struct skiplist *sl, void *item){
    struct skiplist_node *tracks[SKIPLIST_MAXLEVEL];

    if(NULL == item){
        goto err; //the default header cannot be removed
    }
    
    search(sl, item, tracks);
    struct skiplist_node *predeleted_node = tracks[0]->forward[0];
    if(NULL != predeleted_node && NULL != predeleted_node->item && 0 == sl->cmp_item(predeleted_node->item, item)){
        int i = 0;
        for(; i < SKIPLIST_MAXLEVEL; i++){ //remove the deleted node from the linked list
            if(tracks[i]->forward[i] == predeleted_node){
                tracks[i]->forward[i] = predeleted_node->forward[i];
            }
        }
        void *predeleted_item = predeleted_node->item;
        free_node(predeleted_node);
        sl->busy -= 1;
        return predeleted_item; //returns the item of the deleted node to the user for cleansing
    }
    //node not found

err:
    return NULL;
}
