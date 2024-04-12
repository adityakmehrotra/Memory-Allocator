# Memory-Allocator

## Background

Heap memory allocators have two distinct tasks.

First, the memory allocator uses the sbrk() system call to ask the operating system to allocate the heap segment of its virtual address space. However, in the code we've provided, we use mmap() to simulate a process's heap in the memory-mapped segment.
Second, the memory allocator manages this memory by maintaining the internal structure of the heap including tracking the size and status of each heap block.
When a process makes a request for heap memory, the allocator searches its list of heap blocks for a free block that is large enough to satisfy the request. In some implementations, the chosen block will be split into two smaller ones with the first part having its status set to allocated and the second part being free. Later when the process frees the heap memory, the allocator changes that block's status as needed. 

For this assignment, I implemented immediate coalescing, which requires that me check to see if the newly freed block can be coalesced "merged" with adjacent free neighbors to make larger blocks.   

A heap memory allocator is typically provided as part of a standard library rather than being part of the operating system. Thus, the memory allocator operates entirely within the virtual address space of a single process and knows nothing about which physical memory pages have been allocated to this process or the mapping from virtual addresses to physical addresses.  This allows us to focus on the more consistent virtual memory view of memory as a contiguous block.

The C programming language defines its allocator with the functions malloc() and free() which are found in "stdlib.h" as follows:

 - void *malloc(size_t s): allocates s bytes and returns a pointer to the allocated memory. The memory is not cleared.
 - void free(void *ptr): frees the memory pointed to by ptr that was previously allocated by malloc() (or calloc() or realloc()). The behavior is undefined if ptr is a stray pointer or if an attempt is made to free an allocation more than once. If ptr is NULL, the function does nothing and simply returns.
