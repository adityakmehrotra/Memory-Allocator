#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "heapAllocate.h"

/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block.
 */
typedef struct blockHeader {           
    int size_status;
} blockHeader;         

blockHeader *heap_start = NULL;     

int alloc_size;

/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 */
void* balloc(int size) {     

    int leftover; // The leftover variable stores all of the leftover space

    blockHeader *currentHeader = heap_start;
    blockHeader *x;
    blockHeader *matchVal = NULL;
    
    if (size < 1) {
        return NULL;
    } 

    if (size >= alloc_size) {
        return NULL;
    } 

    // If the size is not a multiple of 8, round it
    size += sizeof(blockHeader); 
	if ((size % 8) != 0) {
	   	size = (size + 7) / 8 * 8; // Align size to the next multiple of 8.
	}
    while (!((currentHeader->size_status == 1))) {
        // Check if there is no allocation
        if (!(0x0001 & currentHeader->size_status)) {
            // Check to see if the siize is the same
            if ((currentHeader->size_status & (~(0x0001 | 0x0002))) == size) {
                currentHeader->size_status |= 0x0001;
                if (!(((currentHeader + ((currentHeader->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader))))->size_status == 1)) {
                    (currentHeader + ((currentHeader->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader)))->size_status |= 0x0002;
                }
                // If the currentHeader is the first block, then set the p-bit
                if (currentHeader == heap_start) {
                    currentHeader->size_status |= 0x0002;
                }
                return (currentHeader + 1);
            }

            // The 2nd Case; If the block is not the exact same size
            if ((currentHeader->size_status & (~(0x0001 | 0x0002))) > size && (matchVal == NULL || (currentHeader->size_status & (~(0x0001 | 0x0002))) < (matchVal->size_status & (~(0x0001 | 0x0002))))) {
                matchVal = currentHeader;
            }
        }

        // To access the next block
        currentHeader = (currentHeader + ((currentHeader->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader)));
    }
  
    // The 3rd Case: Return null if there are no matching blocks
    if (matchVal == NULL) {
        return NULL;
    }       
  
    leftover = (matchVal->size_status & (~(0x0001 | 0x0002))) - size;
    x = (size / sizeof(blockHeader)) + matchVal;

    x->size_status = ((leftover | 0x0002) & ~(0x0001));
    ((x + ((x->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader))) - 1)->size_status =(x->size_status & (~(0x0001 | 0x0002)));

     matchVal->size_status = size | (matchVal->size_status & (0x0001 | 0x0002));
     matchVal->size_status |= 0x0001;

     if (!(((matchVal + ((matchVal->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader))))->size_status == 1)) {
        (matchVal + ((matchVal->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader)))->size_status |= 0x0002;
     }

     if (matchVal == heap_start) {
        matchVal->size_status |= 0x0002;
     }
    
    return matchVal + 1;
} 

/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int bfree(void *ptr) {    
    blockHeader *x;
    blockHeader *headerVal;
    blockHeader *n;
    
    // Check if the ptr parameter passed is null
    if (ptr == NULL) {
        return -1;
    }

    // Check if the ptr parameter passed is a multiple of 8
    if((int)ptr % 8 != 0) {
        return -1;
    } 

    // Check if the ptr parameter passed is larger than allocated size
    if (((int)ptr) >= alloc_size) {
        return -1;
    } 
    
    headerVal = ptr - sizeof(blockHeader);

    x = (headerVal + ((headerVal->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader)));
    if (!((headerVal->size_status & 0x0001))) {
        return -1;
    } 

    // The following code is to free the block
    headerVal->size_status &= ~0x0001;
    x->size_status &= ~0x0002;

    ((headerVal + ((headerVal->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader))) - 1)->size_status = (headerVal->size_status & (~(0x0001 | 0x0002)));

    if (!(x->size_status & 0x0001) && !(x->size_status == 1)) {
        headerVal->size_status = headerVal->size_status + x->size_status;
        ((headerVal + ((headerVal->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader))) - 1)->size_status = (headerVal->size_status & (~(0x0001 | 0x0002)));
    }

    if (!(headerVal->size_status & 0x0002)) {
        n = headerVal - (((headerVal - 1)->size_status) / sizeof(blockHeader));
        n->size_status += headerVal->size_status;
        ((n + ((n->size_status & (~(0x0001 | 0x0002))) / sizeof(blockHeader))) - 1)->size_status = (n->size_status & (~(0x0001 | 0x0002)));  
    }

    return 0;
} 


/* 
 * Initializes the memory allocator.
 * Called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int init_heap(int sizeOfRegion) {    

    static int allocated_once = 0; //prevent multiple myInit calls

    int   pagesize; // page size
    int   padsize;  // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int   fd;

    blockHeader* end_mark;

    if (0 != allocated_once) {
        fprintf(stderr, 
                "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize from O.S. 
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }

    allocated_once = 1;

    // for double word alignment and end mark
    alloc_size -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heap_start = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    end_mark = (blockHeader*)((void*)heap_start + alloc_size);
    end_mark->size_status = 1;

    // Set size in header
    heap_start->size_status = alloc_size;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heap_start->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heap_start + alloc_size - 4);
    footer->size_status = alloc_size;

    return 0;
} 

/* 
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void disp_heap() {     

    int    counter;
    char   status[6];
    char   p_status[6];
    char * t_begin = NULL;
    char * t_end   = NULL;
    int    t_size;

    blockHeader *current = heap_start;
    counter = 1;

    int used_size =  0;
    int free_size =  0;
    int is_used   = -1;

    fprintf(stdout, 
            "*********************************** HEAP: Block List ****************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
            "---------------------------------------------------------------------------------\n");

    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;

        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;

        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
                p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);

        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
            "---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
            "*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
            "*********************************************************************************\n");
    fflush(stdout);

    return;  
} 
