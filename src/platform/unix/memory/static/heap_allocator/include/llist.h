#ifndef LLIST_H
#define LLIST_H

#include "heap.h"
#include <stdint.h>

sakura_void_t add_node(bin_t *bin, node_t *node);

sakura_void_t remove_node(bin_t *bin, node_t *node);

node_t *get_best_fit(bin_t *list, sakura_size_t size);
node_t *get_last_node(bin_t *list);

#endif
