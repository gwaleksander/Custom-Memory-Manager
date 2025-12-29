# Custom Heap Memory Allocator

A low-level implementation of a dynamic memory management system in C, built from the ground up to replace standard library allocation functions.

## 🛠 Technical Implementation
- **Block Architecture:** Uses metadata headers for each memory block to track size and availability.
- **Optimization Strategies:**
    - **Splitting:** Divides large free blocks to reduce internal fragmentation.
    - **Coalescing:** Merges adjacent free blocks during `free()` to maintain large contiguous memory areas.
- **Alignment:** Guarantees 8-byte alignment for all allocated pointers, ensuring CPU compatibility.
- **First-Fit Algorithm:** Efficiently scans the implicit free list to locate suitable memory segments.



## 📋 API Overview
- `malloc_custom(size_t size)` - Allocates a block of memory.
- `free_custom(void *ptr)` - Releases and coalesces memory.
- `calloc_custom(size_t nmemb, size_t size)` - Zero-initialized allocation.

## 📊 Demonstrated Mastery
- **Heap Segment Control:** Understanding of the heap's internal layout.
- **Pointer Manipulation:** Complex casting and arithmetic between metadata and payloads.
- **System Stability:** Robust handling of edge cases like zero-size allocation or exhausted memory.
