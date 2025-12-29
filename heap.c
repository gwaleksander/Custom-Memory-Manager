#include "heap.h"

struct heap *heap = NULL;

size_t get_size(struct mem_chunk *block) {
    if (block == NULL || heap == NULL) {
        return 0;
    }

    if (block->next != NULL) {
        if ((char *) block->next < (char *) heap || (char *) block->next >= (char *) heap + heap->size) {
            return 0;
        }
        return (char *) block->next - (char *) block - CHUNK_SIZE - 2 * sizeof(short);
    }

    return (char *) heap + heap->size - (char *) block - CHUNK_SIZE - 2 * sizeof(short);
}

int heap_setup(void) {
    void *heap_start = custom_sbrk(sizeof(struct heap));
    if (heap_start == (void *) -1)
        return -1;

    struct heap *temp = (struct heap *) heap_start;
    temp->head = NULL;
    temp->size = sizeof(struct heap);
    heap = temp;
    return 0;
}

void *heap_malloc(size_t size) {
    if (heap == NULL || size == 0) return NULL;

    int total_size = CHUNK_SIZE + size + 2 * sizeof(short);
    struct mem_chunk *current = heap->head;
    struct mem_chunk *last_block = NULL;

    while (current) {
        last_block = current;
        if (current->free && get_size(current) >= total_size - CHUNK_SIZE - 2 * sizeof(short)) {
            current->free = 0;
            current->size = size;

            *(short *) ((char *) current + CHUNK_SIZE) = FENCE_VAL;
            *(short *) ((char *) current + CHUNK_SIZE + size + sizeof(short)) = FENCE_VAL;

            return (char *) current + CHUNK_SIZE + sizeof(short);
        }
        current = current->next;
    }

    void *new_block_ptr = custom_sbrk(total_size);
    if (new_block_ptr == (void *) -1) return NULL;
    heap->size += total_size;

    struct mem_chunk *new_block = (struct mem_chunk *) new_block_ptr;
    new_block->free = 0;
    new_block->size = size;
    new_block->next = NULL;
    new_block->prev = NULL;

    if (last_block) {
        last_block->next = new_block;
        new_block->prev = last_block;
    } else {
        heap->head = new_block;
    }

    *(short *) ((char *) new_block + CHUNK_SIZE) = FENCE_VAL;
    *(short *) ((char *) new_block + CHUNK_SIZE + size + sizeof(short)) = FENCE_VAL;

    return (char *) new_block + CHUNK_SIZE + sizeof(short);
}

void *heap_calloc(size_t number, size_t size) {
    if (heap == NULL || size <= 0 || number <= 0) return NULL;

    size_t malloc_size = number * size;
    void *ptr = heap_malloc(malloc_size);
    if (ptr == NULL) return NULL;

    memset(ptr, 0, malloc_size);
    return ptr;
}

void *heap_realloc(void *memblock, size_t new_size) {
    if (memblock == NULL) return heap_malloc(new_size);
    if (heap == NULL || get_pointer_type(memblock) != pointer_valid) return NULL;
    if (heap_validate() != 0) return NULL;

    if (new_size == 0) {
        heap_free(memblock);
        return NULL;
    }

    struct mem_chunk *block = (struct mem_chunk *) ((char *) memblock - CHUNK_SIZE - sizeof(short));
    if (block->free != 0) return NULL;

    struct mem_chunk *last = heap->head;
    char *block_start = (char *) block + CHUNK_SIZE;
    while (last != NULL) {
        if (last == block && block->next == NULL && block->free == 0) {
            size_t rest = new_size - last->size;
            if (rest != 0) {
                void *sbrk_ptr = custom_sbrk(rest);
                if (sbrk_ptr != (void *) -1) {
                    heap->size += rest;
                    block->size = new_size;
                    *(short *) block_start = FENCE_VAL;
                    *(short *) (block_start + block->size + sizeof(short)) = FENCE_VAL;
                    return memblock;
                }
            }
        }
        last = last->next;
    }

    size_t prev_size = get_size(block);
    if (new_size == prev_size) return memblock;

    if (new_size > prev_size && block->next != NULL && block->next->free == 1) {
        size_t next_size = get_size(block->next);

        if (new_size < prev_size + next_size + CHUNK_SIZE + 2 * sizeof(short)) {
            size_t size_needed = new_size + 2 * sizeof(short) + 2 * CHUNK_SIZE + 1;
            struct mem_chunk *next_block = block->next;
            if (next_size + prev_size + 2 * sizeof(short) + 2 * CHUNK_SIZE < size_needed) {
                block->size = new_size;
                block->next = next_block->next;
                if (next_block->next) {
                    next_block->next->prev = block;
                }
                *(short *) block_start = FENCE_VAL;
                *(short *) (block_start + block->size + sizeof(short)) = FENCE_VAL;
                return memblock;
            } else { //zwolnij
                struct mem_chunk *newblock = (struct mem_chunk *) (block_start + new_size + 2 * sizeof(short));
                newblock->size = (prev_size + next_size) - new_size;
                newblock->free = 1;
                newblock->prev = block;
                newblock->next = block->next->next;

                if (newblock->next && newblock->next->free) {
                    newblock->size = get_size(newblock) + get_size(newblock->next) + 2 * sizeof(short) + CHUNK_SIZE;
                    struct mem_chunk *next = newblock->next->next;
                    if (next != NULL) next->prev = newblock;
                    newblock->next = next;
                }

                if (block->next && block->next->next) {
                    block->next->next->prev = newblock;
                }

                block->next = newblock;
                *(short *) ((char *) newblock + CHUNK_SIZE) = FENCE_VAL;
                *(short *) ((char *) newblock + CHUNK_SIZE + newblock->size + sizeof(short)) = FENCE_VAL;
                block->size = new_size;


                *(short *) block_start = FENCE_VAL;
                *(short *) (block_start + block->size + sizeof(short)) = FENCE_VAL;
                return memblock;
            }
        }
    } else if (new_size < prev_size) { //stworz nowy skoro zmniejszamy
        size_t old_total_size = CHUNK_SIZE + prev_size + 2 * sizeof(short);
        size_t new_total_size = CHUNK_SIZE + new_size + 2 * sizeof(short);
        block->size = new_size;
        *(short *) block_start = FENCE_VAL;
        *(short *) (block_start + block->size + sizeof(short)) = FENCE_VAL;
        size_t diff = old_total_size - new_total_size;

        if (diff >= CHUNK_SIZE + 2 * sizeof(short) + 1) {
            struct mem_chunk *temp = (struct mem_chunk *) ((char *) block + new_total_size);
            temp->size = diff - CHUNK_SIZE - 2 * sizeof(short);
            temp->free = 1;
            temp->prev = block;
            temp->next = block->next;

            if (block->next) {
                block->next->prev = temp;
            }
            block->next = temp;
            *(short *) ((char *) temp + CHUNK_SIZE) = FENCE_VAL;
            *(short *) ((char *) temp + CHUNK_SIZE + temp->size + sizeof(short)) = FENCE_VAL;
        }
        return memblock;
    }

    //jesli nic sie nie da zrobic, nowy malloc
    void *mem_to_return = heap_malloc(new_size);
    if (mem_to_return == NULL) return NULL;
    memcpy(mem_to_return, memblock, prev_size);
    heap_free(memblock);
    return mem_to_return;
}

int heap_validate(void) {
    if (heap == NULL || heap->size < sizeof(struct heap)) {
        return 2;
    }

    if (heap->head == NULL) return 0;

    if ((char *) heap->head < (char *) heap || (char *) heap->head >= (char *) heap + heap->size) {
        return 2;
    }

    struct mem_chunk *curr = heap->head;
    struct mem_chunk *prev = NULL;


    while (curr) {
        if (curr->prev != prev || curr->free != 0 && curr->free != 1) {
            return 3;
        }
        if ((char *) curr < (char *) heap || (char *) curr >= (char *) heap + heap->size) {
            return 3;
        }
        if ((char *) curr + CHUNK_SIZE + curr->size + 2 * sizeof(short) > (char *) heap + heap->size) {
            return 3;
        }
        if (curr->next != NULL) {
            if ((char *) curr->next < (char *) heap ||
                (char *) curr->next >= (char *) heap + heap->size) {
                return 3;
            }
        }
        if (!curr->free) { //sprawdz plotki
            char *fence1_ptr = (char *) curr + CHUNK_SIZE;
            char *fence2_ptr = fence1_ptr + sizeof(short) + curr->size;

            if ((char *) fence1_ptr < (char *) heap ||
                (char *) fence1_ptr + sizeof(short) > (char *) heap + heap->size ||
                (char *) fence2_ptr < (char *) heap ||
                (char *) fence2_ptr + sizeof(short) > (char *) heap + heap->size) {
                return 3;
            }

            short fence1 = *(short *) fence1_ptr;
            short fence2 = *(short *) fence2_ptr;
            if (fence1 != FENCE_VAL || fence2 != FENCE_VAL) {
                return 1;
            }
        }

        prev = curr;
        curr = curr->next;
    }

    return 0;
}

void heap_free(void *memblock) {
    if (!heap || !memblock || get_pointer_type(memblock) != pointer_valid) return;

    struct mem_chunk *block = (struct mem_chunk *) ((char *) memblock - CHUNK_SIZE - sizeof(short));
    block->free = 1;

    if (block->next != NULL && block->next->free) { //merge z nastepnym
        block->size += CHUNK_SIZE + block->next->size + 2 * sizeof(short);
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev != NULL && block->prev->free) { //merge z poprzednim
        block->prev->size += CHUNK_SIZE + block->size + 2 * sizeof(short);
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void heap_clean(void) {
    if (!heap) return;
    custom_sbrk((long) (-1 * heap->size));
    heap = NULL;
}

enum pointer_type_t get_pointer_type(const void *const pointer) {
    if (pointer == NULL) return pointer_null;
    if (heap == NULL || heap->head == NULL) return pointer_null;

    if ((const char *) pointer < (char *) heap + sizeof(struct heap)) return pointer_heap_corrupted;
    if ((const char *) pointer >= (char *) heap + heap->size) return pointer_heap_corrupted;

    struct mem_chunk *block = heap->head;
    while (block != NULL) {
        char *block_end = (char *) block + CHUNK_SIZE;
        char *data_start = (char *) block + CHUNK_SIZE + sizeof(short);
        char *data_end = (char *) block + CHUNK_SIZE + sizeof(short) + block->size;
        char *fence2_start = (char *) block + CHUNK_SIZE + sizeof(short) + block->size;
        char *fence2_end = (char *) block + CHUNK_SIZE + sizeof(short) + block->size + sizeof(short);

        if ((const char *) pointer < fence2_end && (const char *) pointer >= (char *) block) {
            if ((const char *) pointer < block_end) {
                if (block->free == 0) {
                    return pointer_control_block;
                } else {
                    return pointer_unallocated;
                }
            } else if ((const char *) pointer == data_start) {
                if (block->free == 0) {
                    return pointer_valid;
                } else {
                    return pointer_unallocated;
                }
            } else if ((const char *) pointer > data_start && (const char *) pointer < data_end) {
                if (block->free == 0) {
                    return pointer_inside_data_block;
                } else {
                    return pointer_unallocated;
                }
            } else if ((const char *) pointer >= fence2_start && (const char *) pointer < fence2_end) {
                if (block->free == 0) {
                    return pointer_inside_fences;
                } else {
                    return pointer_unallocated;
                }
            } else if ((const char *) pointer < (char *) block + CHUNK_SIZE + sizeof(short)) {
                if (block->free == 0) {
                    return pointer_inside_fences;
                } else {
                    return pointer_unallocated;
                }
            }
        }
        block = block->next;
    }
    return pointer_null;
}

size_t heap_get_largest_used_block_size(void) {
    if (heap == NULL || heap->head == NULL || heap->size <= 0) {
        return 0;
    }

    struct mem_chunk *curr = heap->head;
    if (!curr) return 0;
    if (heap_validate() != 0) return 0;

    size_t largest = 0;
    while (curr) {
        if (get_pointer_type((char *) curr + CHUNK_SIZE + sizeof(short)) == pointer_valid) {
            if (curr->size > largest) {
                largest = curr->size;
            }
        }
        curr = curr->next;
    }

    return largest;
}

int main () {
    return 0;
}