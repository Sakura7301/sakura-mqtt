#include "include/llist.h"

sakura_void_t add_node(bin_t *bin, node_t* node) {
    node_t *current = NULL;
    node_t *previous = NULL;
    do
    {
        /* check params */
        if(bin == NULL || node == NULL){
            break;
        }

        /* reset pointer */
        node->next = NULL;
        node->prev = NULL;    

        if(bin->head == NULL){
            bin->head = node;
            break;
        }

        /* we need to save next and prev while we iterate */
        current = bin->head;
        /* iterate until we get the the end of the list or we find a 
            node whose size is*/
        while (current != NULL && current->size <= node->size) {
            previous = current;
            current = current->next;
        }

        /* we reached the end of the list */
        if (current == NULL) {
            previous->next = node;
            node->prev = previous;
        } else {
            if (previous != NULL) {
                /* middle of list, connect all links! */
                node->next = current;
                previous->next = node;

                node->prev = previous;
                current->prev = node;
            } else {
                /* head is the only element */
                node->next = bin->head;
                bin->head->prev = node;
                bin->head = node;
            }
        }
    } while (SAKURA_FALSE);
}

sakura_void_t remove_node(bin_t * bin, node_t *node) {
    node_t *temp = NULL;

    do
    {
        /* check params */
        if(bin == NULL || node == NULL){
            break;
        }

        if (bin->head == node) { 
            bin->head = bin->head->next;
            break;;
        }      
        /* set temp */
        temp = bin->head->next;
        /* find node in list */
        while (temp != NULL) {
            if (temp == node) {
                /* found the node */
                if (temp->next == NULL) {
                    /* last item */
                    temp->prev->next = NULL;
                } else {
                    /* middle item */
                    temp->prev->next = temp->next;
                    temp->next->prev = temp->prev;
                }
                /* we dont worry about deleting the head here because we already checked that */
                break;;
            }
            temp = temp->next;
        }
    } while (SAKURA_FALSE);
}

node_t *get_best_fit(bin_t *bin, sakura_size_t size) {
    node_t *temp = NULL;
    sakura_uint32_t find_flag = 0;
    do
    {
        /* empty list! */
        if(bin == NULL|| bin->head == NULL){
            break;
        }

        /* set temp */
        temp = bin->head;

        /* check size */
        while (temp != NULL) {
            if (temp->size >= size) {
                /* found a fit! */
                find_flag = 1;
                break;
            }
            temp = temp->next;
        }
    } while (SAKURA_FALSE);

    /* check find result */
    if(find_flag == 0){
        temp = NULL;
    }

    return temp;
}

node_t *get_last_node(bin_t *bin) {
    node_t *temp = NULL;
    do
    {
        /* check params */
        if(bin == NULL){
            break;
        }

        /* set temp */
        temp = bin->head;

        /* find last node */
        while (temp != NULL && temp->next != NULL) {
            temp = temp->next;
        }

    } while (SAKURA_FALSE);

    return temp;
}

