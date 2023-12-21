#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>
#include "sakura_types.h"

#define HEAP_INIT_SIZE 0x10000
#define HEAP_MAX_SIZE 0xF0000
#define HEAP_MIN_SIZE 0x10000

#define MIN_ALLOC_SZ 4

#define MIN_WILDERNESS 0x2000
#define MAX_WILDERNESS 0x1000000

#define BIN_COUNT 9
#define BIN_MAX_IDX (BIN_COUNT - 1)


typedef struct node_t {
    sakura_uint32_t hole;
    sakura_uint32_t size;
    struct node_t* next;
    struct node_t* prev;
} node_t;

typedef struct {
    node_t *header;
} footer_t;

typedef struct {
    node_t* head;
} bin_t;

typedef struct {
    sakura_long32_t start;
    sakura_long32_t end;
    bin_t *bins[BIN_COUNT];
} heap_t;

sakura_void_t init_heap(heap_t *heap, sakura_long32_t start, sakura_int32_t size);

sakura_void_t *heap_alloc(heap_t *heap, sakura_size_t size);
sakura_void_t heap_free(heap_t *heap, sakura_void_t *p);

sakura_uint32_t get_bin_index(sakura_size_t sz);
sakura_void_t create_foot(node_t *head);
footer_t *get_foot(node_t *head);

node_t *get_wilderness(heap_t *heap);

#endif
