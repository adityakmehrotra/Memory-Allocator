# Custom Heap Memory Allocator

This project implements a custom heap memory allocator in C, using memory management techniques such as the `mmap()` system call instead of `sbrk()`. The allocator efficiently handles memory requests by implementing immediate coalescing to merge adjacent free blocks, mimicking behavior similar to standard memory allocators like `malloc()` and `free()`.

## Project Overview

The allocator is designed to simulate a process's heap in a memory-mapped segment. It provides functionality to allocate and free memory blocks within this simulated heap, and it includes features for displaying heap status and managing internal heap structure.

## Features

- **Custom Allocation**: Implements a custom allocation function `balloc()` that allocates heap memory based on size requests.
- **Memory Freeing**: Includes `bfree()`, a function to free previously allocated blocks, with immediate coalescing of adjacent free blocks.
- **Heap Initialization**: Uses `init_heap()` to initialize the memory allocator, which can be called only once per program execution.
- **Heap Display**: `disp_heap()` function to print detailed information about the heap's current status, including block sizes and allocation status.

## Building and Running

### Prerequisites

- GCC compiler
- Linux environment (due to the use of `mmap()` and specific file descriptors)

### Compile the Project

 ```bash
 gcc -o heapAllocator heapAllocate.c
 ```

## Run the Program
 ```bash
 ./heapAllocator
 ```


