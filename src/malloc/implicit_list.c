#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../headers/allocator.h"
#include "../headers/algorithm.h"

int implicit_list_initialize_free_block() {
    return 1;
}

uint64_t implicit_list_search_free_block(uint32_t payload_size, uint32_t* alloc_blocksize) {
    // search 8-byte block list
    // TODO

    uint32_t free_blocksize = round_up(payload_size, 8) + 4 + 4;
    *alloc_blocksize = free_blocksize;

    // search the whole heap
    uint64_t b = get_firstblock();
    while (b <= get_lastblock()) {
        uint32_t blocksize = get_blocksize(b);
        uint32_t allocated = get_allocated(b);

        if (allocated == FREE && free_blocksize <= blocksize) {
            return b;
        } else {
            b = get_nextheader(b);
        }
    }

    return NIL;
}

int implicit_list_insert_free_block(uint64_t free_header) {
    return 1;
}

int implicit_list_delete_free_block(uint64_t free_header) {
    return 1;
} 

void implicit_list_check_free_block() {
    return;
}
