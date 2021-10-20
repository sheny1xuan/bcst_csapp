#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include "../headers/allocator.h"

// if x == k * alignment    return x;
// if x == k * alignment + n (n < x) return (k + 1) * alignment
uint64_t round_up(int x, int alignment) {
    return alignment * ((x + alignment - 1) / alignment);
}

/* ------------------------ */ 
/*     Block Operation      */ 
/* ------------------------ */

uint32_t get_blocksize(uint64_t header_vaddr) {
    if(header_vaddr == NIL) {
        return 0;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);   // 4 Byte alignment for header and footer

    uint32_t header_value = *(uint32_t *)&heap[header_vaddr];

    return header_value & 0xFFFFFFF8;

}

// Set both header & footer
void set_blocksize(uint64_t header_vaddr, uint32_t block_size) {
    if(header_vaddr == 0) {
        return;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue() );
    assert((header_vaddr & 0x3) == 0x0);
    assert((block_size & 0x07) == 0x0);   // blocksize is 8 Byte aligned

    *(uint32_t *)&heap[header_vaddr] &= 0x00000007;
    *(uint32_t *)&heap[header_vaddr] |= block_size;

}

uint32_t get_allocated(uint64_t header_vaddr) {
    if(header_vaddr == NIL) {
        // null can considered as allocated
        return ALLOCATED;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);
    uint32_t header_value = *(uint32_t *)&heap[header_vaddr];

    return header_value & 0x1;
}

void set_allocated(uint64_t header_vaddr, uint32_t allocated) {
    if(header_vaddr == NIL) {
        return ;
    }

    assert(get_prologue() <= header_vaddr && header_vaddr <= get_epilogue());
    assert((header_vaddr & 0x3) == 0x0);

    *(uint32_t *)&heap[header_vaddr] &= 0xFFFFFFFE;
    *(uint32_t *)&heap[header_vaddr] |= allocated & 0x1;

}

uint64_t get_payload(uint64_t vaddr) {
    if(vaddr == NIL) {
        return NIL;
    }

    assert(get_firstblock() <= vaddr && vaddr <= get_epilogue());
    assert((vaddr & 0x3) == 0x0);
    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    return round_up(vaddr, 8);
}

uint64_t get_header(uint64_t vaddr) {
    if(vaddr == NIL) {
        return NIL;
    }

    assert(get_prologue() <= vaddr && vaddr <= get_epilogue());
    assert((vaddr & 0x3) == 0x0);
    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    // null block doesn't have header
    return round_up(vaddr, 8) - 4;
}

uint64_t get_footer(uint64_t vaddr) {
    if(vaddr == NIL) {
        return NIL;
    }

    assert(get_prologue() <= vaddr && vaddr <= get_epilogue());
    assert((vaddr & 0x3) == 0x0);
    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    uint64_t header_vaddr = get_header(vaddr);
    uint64_t footer_vaddr = get_blocksize(header_vaddr) + header_vaddr - 4;

    assert(get_prologue() <= footer_vaddr && footer_vaddr <= get_epilogue());
    assert((footer_vaddr & 0x3) == 0x0);
    return footer_vaddr;
}

/* ------------------------ */ 
/*     heap operation       */ 
/* ------------------------ */
uint64_t get_nextheader(uint64_t vaddr) {
    if(vaddr == NIL || vaddr == get_epilogue()) {
        return NIL;
    }

    assert(vaddr >= get_prologue() && vaddr < get_epilogue());

    // vaddr can be 
    // 1. starting address of the block
    // 2. starting address of the payload
    uint64_t header_vaddr = round_up(vaddr, 8) - 4;
    uint32_t block_size = get_blocksize(header_vaddr);
    uint64_t next_header_vaddr = header_vaddr + block_size;
    // next_header_vaddr may be epilogue
    assert(get_firstblock() <= next_header_vaddr &&
            next_header_vaddr <= get_epilogue());
    
    return next_header_vaddr;

}

uint64_t get_prevheader(uint64_t vaddr) {

    if(vaddr == NIL || vaddr == get_prologue()) {
        return NIL;
    }

    assert(vaddr >= get_firstblock() && vaddr <= get_epilogue());
    // vaddr can be 
    // 1. starting address of the block
    // 2. starting address of the payload
    uint64_t header_vaddr = get_header(vaddr);

    if(header_vaddr == heap_start_vaddr) {
        // this block is first block
        return 0;
    }

    uint64_t prev_footer_vaddr = header_vaddr - 4;
    uint32_t prev_block_size = get_blocksize(prev_footer_vaddr);

    uint64_t prev_header_vaddr = header_vaddr - prev_block_size;

    assert(get_prologue() <= prev_header_vaddr &&
        prev_header_vaddr < get_epilogue());
    assert(get_blocksize(prev_header_vaddr) == get_blocksize(prev_footer_vaddr));
    assert(get_allocated(prev_header_vaddr) == get_allocated(prev_footer_vaddr));
    
    return prev_header_vaddr;

}

uint64_t get_prologue() {
    // ensure heap begin and end area is correct
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    return heap_start_vaddr + 4;
}

uint64_t get_epilogue() {
    // ensure heap begin and end area is correct
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    return heap_end_vaddr - 4;
}

uint64_t get_firstblock() {
    // ensure heap begin and end area is correct
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    // cover(4Byte), prologue(8Byte), firstblock()
    return get_prologue() + 8;
}

uint64_t get_lastblock() {
    // Ensure heap begin and end area is correct
    assert(heap_end_vaddr > heap_start_vaddr);
    assert((heap_end_vaddr - heap_start_vaddr) % 4096 == 0);
    assert(heap_start_vaddr % 4096 == 0);

    // TODO by prevheader
    return get_prevheader(get_epilogue());
}

int is_firstblock(uint64_t vaddr) {
    if(vaddr == NIL) {
        return 0;
    }

    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert(get_firstblock() <= vaddr && vaddr < get_epilogue());
    assert((vaddr & 0x3) == 0);   // 4 Byte alignment

    uint64_t header_vaddr = get_header(vaddr);
    if(header_vaddr == get_firstblock()) {
        // is first block
        return 1;
    }

    // isn't first block
    return 0;
}

int is_lastblock(uint64_t vaddr) {

    if(vaddr == NIL) {
        return 0;
    }
    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert(heap_start_vaddr <= vaddr && vaddr <= heap_end_vaddr);
    assert((vaddr & 0x3) == 0x0);

    uint64_t header_addr = get_header(vaddr);
    uint32_t blocksize = get_blocksize(header_addr);

    if(header_addr + blocksize == get_epilogue()) {
        // it is last block

        return 1;
    }

    return 0;
}