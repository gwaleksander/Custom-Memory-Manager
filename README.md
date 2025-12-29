# Custom Heap Memory Manager (Malloc Implementation)

A robust, page-based memory allocator that manages a custom heap segment. This project implements a fully functional alternative to the standard `malloc` family with integrated memory corruption detection.

## 🛠 Key Engineering Features
- **Custom SBRK Integration:** Interacts directly with the system's memory break (`custom_sbrk`) to request 4KB memory pages.
- **Memory Fences (Paddings):** Implements "head" and "tail" fences (guard bytes) around every user block to detect **One-off** errors and buffer overflows.
- **Advanced Block Management:**
    - **First-Fit Allocation:** Scans the heap for the first available block that meets size requirements.
    - **Smart Realloc:** Attempts to expand blocks in-place by consuming adjacent free space before resorting to a full migration.
- **Heap Validation & Diagnostics:**
    - `heap_validate`: A high-integrity function capable of detecting heap corruption even if control structures are partially overwritten.
    - `get_pointer_type`: Classifies any pointer as valid, inside fences, control block, or unallocated.

## 📋 Security & Stability
- **Implicit Free List:** Efficiently tracks allocated and free segments within the heap.
- **Fragmentation Control:** Automatic coalescing (merging) of adjacent free blocks during deallocation and reallocation.
- **Resilience:** Designed to stay stable even under intentional memory abuse, providing clean exits via `heap_clean`.

## 📊 Demonstrated Skills
- **Memory Architecture:** Mastering the heap's internal layout: Control Structures -> Head Fence -> User Data -> Tail Fence.
- **Pointer Wizardry:** Managing metadata offsets and ensuring data alignment for CPU efficiency.
- **Software Reliability:** Implementing defensive programming techniques to prevent `Segmentation Faults` during heap inspection.
