#ifndef MAKOK_HEAP_H
#define MAKOK_HEAP_H

#include <string.h>
#include "custom_unistd.h"
#include <stddef.h>

#define FENCE_VAL 16
#define CHUNK_SIZE sizeof(struct mem_chunk)

enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

int heap_setup(void);
void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t count);
void heap_free(void* memblock);
int heap_validate(void);
size_t heap_get_largest_used_block_size(void);
void heap_clean(void);
enum pointer_type_t get_pointer_type(const void* const pointer);

struct heap {
    struct mem_chunk* head;
    size_t size;
};

struct __attribute__((packed)) mem_chunk {
//__attribute__((packed)) zeby kompilator nie ustawial zadnego paddingu
    size_t size;
    short free;
    struct mem_chunk* next;
    struct mem_chunk* prev;
};

#endif //MAKOK_HEAP_H
