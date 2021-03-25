#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "goatmalloc.h"

void *arena_start;
int arena_size;

int statusno = 0;



struct __node_t* head;

int init(size_t size) {


    printf("Initializing arena: \n");

    // If the specified size is less than or equal to 0, throw an error

    printf("...requested size %d bytes \n", (int) size);
    int pageSize = getpagesize();
    if ((int) size <= 0) {
        return ERR_BAD_ARGUMENTS;
    }

    if(size > MAX_ARENA_SIZE){
        printf("...error: requested size larger than MAX_ARENA_SIZE (2147483647)\n");
        return ERR_SYSCALL_FAILED;

    }
    // Round size to the nearest multiple of pagesize

    size = size + pageSize / 2;
    size -= size % pageSize;

    if (size < pageSize) size = pageSize;

    printf("...pagesize is %d bytes \n", pageSize);

    printf("...adjusting size with page boundaries\n");

    printf("...adjusted size is %d bytes\n", (int) size);

    int fd = open("/dev/zero", O_RDWR);
    // Changed size to allocationsize
    printf("...mapping arena with mmap()\n");
    arena_start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    printf("...arena starts at %p\n", arena_start);
    printf("...arena ends at %p\n", arena_start + size);

    arena_size = size;



    // Initialzie the global pointer to start at the arena start location in memory
    head = arena_start;

    // Initialize the head chunk
    head->size = arena_size - sizeof(node_t);
    printf("...initializing header for initial free chunk\n");
    printf("...header size is %lu bytes \n", sizeof(node_t));
    head->is_free = 1;
    head->fwd = NULL;
    head->bwd = NULL;


    // if you return 0 everything breaks for some reason
    return size;
}

int destroy() {
    // If the arena was not created, throw an error
    printf("Destroying Arena: \n");
    if (arena_start == NULL) {
        printf("...error: cannot destroy uninitialized arena. Setting error status\n");
        statusno = ERR_UNINITIALIZED;
        return statusno;
    }

    printf("...unmapping arena with munmap() \n");
    munmap(arena_start, arena_size);
    arena_start = NULL;
    arena_size = 0;

    return 0;
}


void *walloc(size_t size) {

    printf("Allocating memory: \n");


    // Make sure arena is initialized
    if (arena_start == NULL) {
        printf("Error: Uninitialized. Setting status code\n");
        statusno = ERR_UNINITIALIZED;
        return NULL;
    }

    // Make sure size is at least 1 byte
    if (size <= 0) {
        statusno = ERR_BAD_ARGUMENTS;
        return NULL;
    }

    printf("...looking for free chunk of >= %d bytes \n", (int) size);


    struct  __node_t* checkNode = NULL;
    checkNode = head;

    int newChunkBuffer = 0;

    // "checkNode" will always be the header that the program goes into
    // "newChunkBuffer" is an address that always points to the start of the next new chunk header
    while (checkNode) {

        // If the current node is suitable to place the program into
        if ((checkNode->size >= size) && (checkNode->is_free == 1)) {
            printf("...found free chunk of %lu bytes with header at %p\n",checkNode->size,checkNode);
            printf("...free chunk->fwd currently points to %p\n",checkNode->fwd);
            printf("...free chunk->bwd currently points to %p\n",checkNode->bwd);

            // Initialize a new chunk if there is still space leftover big enough to fit another header
            printf("...checking if splitting is required\n");
            if(checkNode->size > size + sizeof(node_t)){

                // Update chunk buffer
                newChunkBuffer += size + sizeof(node_t);

                printf("...splitting free chunk\n");
                // Initialize the start location of new chunk pointer
                // Start at the arena start, add the total size behind the new chunk which is newChunkBuffer + size of incoming program
                struct __node_t* newChunkPtr = NULL;
                newChunkPtr = arena_start + newChunkBuffer;

                // Need to subtract size of node_t to account for the space that the new chunk pointer takes up
                newChunkPtr->size = checkNode->size - size - sizeof(node_t);
                newChunkPtr->fwd = checkNode->fwd;
                newChunkPtr->bwd = checkNode;
                newChunkPtr->is_free = 1;

                // Set the current node's forward to be this new free space
                checkNode->fwd = newChunkPtr;

                // Update checkNode to be the "chunk" that holds the program
                // Only update the size of checknode if there is room for another header in the free space after it
                // If there is not enough space, simply change is_free to 0 and keep the size the entire partition
                checkNode->size = size;
            }else{
                printf("...splitting not required\n");
            }


            printf("...updating chunk header at %p\n",checkNode);
            // fwd and bwd would remain the same
            checkNode->is_free = 0;

            // Return the location in memory where the program starts (accounts for header)
            // When adding an int to a pointer, it will add a multiple of the size of the struct in bytes to prevent misalignment
            // e.g. checkNode + 2 will add 64 bytes since size of node_t is 32 bytes
            printf("...being careful with my pointer arthimetic and void pointer casting\n");
            printf("...allocation starts at %p\n", checkNode + 1);
            return checkNode + 1;
        } else {
            // Update chunk buffer

            newChunkBuffer += checkNode->size + sizeof(node_t);
        }
            checkNode = checkNode->fwd;
    }

    // If no chunks are suitable, we are out of memory
    printf("...no such free chunk exists\n");
    printf("...setting error code\n");
    statusno = ERR_OUT_OF_MEMORY;
    return NULL;
}

void wfree(void *ptr) {


    printf("Freeing allocated memory: \n");
    printf("...supplied pointer %p \n", ptr);

    // Basic test
    // Have to subtract sizeof(node_t) since the pointer passed in is the location of the beginning of the chunk not the header
    printf("...being careful with my pointer arthimetic and void pointer casting\n");
    struct __node_t* chunk = ptr - sizeof(node_t);
    printf("...accessing chunk header at %p\n",chunk);
    printf("...chunk of size %lu\n",chunk->size);

    chunk->is_free = 1;

    // Free Coalescing
    // Worst case scenario: free -> chunk -> free
    // removing chunk would result in 3 neighboring free chunks
    // probably need to just check fwd and bwd of changed chunk

    /*
     * When two neighboring chunks are both free (chunk 1, chunk 2 in that order)
     * chunk1.fwd = chunk2.fwd
     * chunk1.bwd stays same
     * chunk1.size = chunk1.size + chunk2.size
     * chunk2 = null;
     */

    // Store fwd and bwd of chunk
    struct __node_t* bwd = chunk->bwd;
    struct __node_t* fwd = chunk->fwd;

    // Initialize an empty struct to zero stuff
    struct __node_t empty = {0};

    // Check bwd of desired chunk
    printf("...checking if coalescing is needed\n");
    if(bwd != NULL) {
        if (bwd->is_free == 1) {
            bwd->fwd = chunk->fwd;

            // Update the bwd of the chunk in front of the one that is getting deleted
            if(chunk->fwd != NULL) chunk->fwd->bwd = bwd;

            bwd->size += chunk->size + sizeof(node_t);
            chunk = bwd;
        }
    }
    printf("...coalescing not needed.\n");

    // Check fwd of desired chunk
    if(fwd != NULL) {
        if (fwd->is_free == 1) {
            chunk->fwd = fwd->fwd;

            // Update the bwd of the chunk in front of the one that is getting deleted
            // I don't use fwd and bwd here because if there is a backward coalesce right before this one,
            // bwd and fwd arent updated to the new chunk values
            if(chunk->fwd != NULL) {
                if (chunk->fwd->fwd != NULL) chunk->fwd->fwd->bwd = chunk;
            }

            chunk->size += fwd->size + sizeof(node_t);
            fwd = &empty;
        }
    }
}

