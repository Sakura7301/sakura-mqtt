#include "include/heap.h"
#include "include/llist.h"

#include "sakura_log.h"

#define LOG_TAG           "HEAP"
#define HEAP_LOGE         SAKURA_LOGE
#define HEAP_LOGI         SAKURA_LOGI
#define HEAP_LOGD         SAKURA_LOGD
#define HEAP_LOGW         SAKURA_LOGW

#define USER_POINTER_OFFSET     (2 * sizeof(sakura_int32_t))   /* the offset of member `next` in structure `node_t` */

static sakura_uint32_t overhead = sizeof(footer_t) + sizeof(node_t);

sakura_void_t init_heap(heap_t *heap, sakura_long32_t start, sakura_int32_t size) {
    node_t *init_region = NULL;

    do
    {
        /* check params */
        if(heap == NULL){
            HEAP_LOGE("invalid heap pointer!\n");
            break;
        }

        init_region = (node_t *) start;

        init_region->hole = 1;
        init_region->size = size - sizeof(node_t) - sizeof(footer_t);

        /* create foot */
        create_foot(init_region);

        add_node(heap->bins[get_bin_index(init_region->size)], init_region);

        heap->start = start;
        heap->end   = start + size;        
    } while (SAKURA_FALSE);
}

sakura_void_t *heap_alloc(heap_t *heap, sakura_size_t size) {
    sakura_uint32_t index = 0;
    bin_t *temp = NULL;
    node_t *found = NULL;
    node_t *split = NULL;
    sakura_uint32_t new_idx = 0;
    sakura_int32_t find_flag = 0;
    sakura_void_t* result = NULL;

    do
    {
        /* check params */
        if(heap == NULL){
            HEAP_LOGE("invalid heap pointer!\n");
            break;
        }

        /* By default, align with 4 bytes */
        size = ((size + 3) >> 2) << 2;
        index = get_bin_index(size);
        temp = (bin_t *) heap->bins[index];
        found = get_best_fit(temp, size);

        while (found == NULL) {
            /* a patch from the issue of the original repository: https://github.com/CCareaga/heap_allocator/issues/8 */
            if (index >= BIN_MAX_IDX){
                find_flag = -1;
                HEAP_LOGW("insufficient memory, unable to allocate! total memory size = %d, need size = %d\n", CONFIG_IMPL_DYNAMIC_MEMORY_SIZE, size);
                break;
            }

            /* found best fit memory space */
            temp = heap->bins[++index];
            found = get_best_fit(temp, size);
        }

        /* memory full */
        if(find_flag == -1){
            break;
        }

        if ((found->size - size) > (overhead + MIN_ALLOC_SZ)) {
            split = (node_t *) (((char *) found + sizeof(node_t) + sizeof(footer_t)) + size);
            split->size = found->size - size - sizeof(node_t) - sizeof(footer_t);
            split->hole = 1;

            create_foot(split);
            new_idx = get_bin_index(split->size);
            add_node(heap->bins[new_idx], split);
            found->size = size;
            create_foot(found);
        }

        found->hole = 0;
        remove_node(heap->bins[index], found);

        found->prev = NULL;
        found->next = NULL;   

        find_flag = 1;     
    } while (SAKURA_FALSE);
    
    /* check memory result */
    if(find_flag == 1){
        /* the real user pointer */
        result = &found->next;
    }
    return result;
}

sakura_void_t heap_free(heap_t *heap, sakura_void_t *p) {
    bin_t *list = NULL;
    footer_t *new_foot = NULL;
    footer_t *old_foot = NULL;
    sakura_int32_t has_next = 0;
    sakura_int32_t has_prev = 0;
    footer_t *f = NULL;
    node_t *prev = NULL;
    node_t *head = NULL;
    node_t *next = NULL;
    do
    {
        /* check params */
        if(heap == NULL || p == NULL){
            HEAP_LOGE("invalid params!\n");
            break;
        }

        /*
        * in case of the first block and the last block
        * the first block has no previous block
        * while the last block has no next block
        */
        head = (node_t *) ((char *) p - USER_POINTER_OFFSET);
        if (head != (node_t *) (uintptr_t) heap->start) {
            has_prev = 1;
            f = (footer_t *) ((char *) head - sizeof(footer_t));
            prev = f->header;
        }

        next = (node_t *) ((char *) get_foot(head) + sizeof(footer_t));
        if (next != (node_t *) (uintptr_t) heap->end) {
            has_next = 1;
        }

        if (has_prev && prev->hole) {
            list = heap->bins[get_bin_index(prev->size)];
            remove_node(list, prev);

            prev->size += overhead + head->size;
            new_foot = get_foot(head);
            new_foot->header = prev;

            head = prev;
        }

        if (has_next && next->hole) {
            list = heap->bins[get_bin_index(next->size)];
            remove_node(list, next);

            head->size += overhead + next->size;

            old_foot = get_foot(next);
            old_foot->header = 0;
            next->size = 0;
            next->hole = 0;

            new_foot = get_foot(head);
            new_foot->header = head;
        }

        head->hole = 1;
        add_node(heap->bins[get_bin_index(head->size)], head);        
    } while (SAKURA_FALSE);
}


sakura_uint32_t get_bin_index(sakura_size_t sz) {
    sakura_uint32_t index = 0;
    sz = sz < 4 ? 4 : sz;

    while (sz >>= 1){
        index++; 
    } 
    index -= 2;

    if (index > BIN_MAX_IDX){
        index = BIN_MAX_IDX;
    }

    return index;
}

sakura_void_t create_foot(node_t *head) {
    footer_t *foot = NULL;

    do
    {
        /* check params */
        if(head == NULL){
            HEAP_LOGE("invalid params!\n");
            break;
        }
        foot = get_foot(head);
        foot->header = head;
    } while (SAKURA_FALSE);    
}

footer_t *get_foot(node_t *node) {
    footer_t* result = NULL;

    do
    {
        if(node == NULL){
            HEAP_LOGE("invalid params!\n");
            break;
        }
        result = (footer_t *) ((char *) node + sizeof(node_t) + node->size);
    } while (SAKURA_FALSE);
    
    return result;
}

node_t *get_wilderness(heap_t *heap) {
    footer_t *wild_foot = NULL;
    do
    {
        /* check params */
        if(heap == NULL){
            HEAP_LOGE("invalid params!\n");
            break;
        }
        
        wild_foot = (footer_t *) ((char *) heap->end - sizeof(footer_t));
    } while (SAKURA_FALSE);
    
    return wild_foot->header;
}
