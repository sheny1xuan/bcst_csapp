/*
 * @Descripttion: 
 * @version: 
 * @Author: Stonex
 * @Date: 2021-10-13 16:11:38
 * @LastEditTime: 2021-10-16 22:38:05
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

uint64_t heap_start_vaddr = 4;
uint64_t heap_end_vaddr = 4096-1;

#define HEAP_MAX_SIZE (4096 * 8)
uint8_t heap[HEAP_MAX_SIZE];

// TODO: syscall
void os_syscall_brk(uint64_t start_vaddr) {

}

static uint64_t round_up(int x, int alignment);

static uint32_t get_blocksize(uint64_t header_vaddr);
static void set_blocksize(uint64_t header_vaddr, uint32_t block_size);

static uint32_t get_allocated(uint64_t header_vaddr);
static void set_allocated(uint64_t header_vaddr, uint32_t allocated);

static int is_lastblock(uint64_t);

static uint64_t get_payload_addr(uint64_t vaddr);
static uint64_t get_header_addr(uint64_t vaddr);

static uint64_t get_nextheader(uint64_t vaddr);
static uint64_t get_prevheader(uint64_t vaddr);

// if x == k * alignment    return x;
// if x == k * alignment + n (n < x) return (k + 1) * alignment
uint64_t round_up(int x, int alignment) {
    return alignment * ((x + alignment - 1) / alignment);
}

static uint32_t get_blocksize(uint64_t header_vaddr) {
    if(header_vaddr == 0) {
        return 0;
    }

    assert(heap_start_vaddr <= header_vaddr && header_vaddr <= heap_end_vaddr );
    assert((header_vaddr & 0x3) == 0x0);
    uint32_t header_value = *(uint32_t *)&heap[header_vaddr];

    return header_value & 0xFFFFFFF8;

}

static void set_blocksize(uint64_t header_vaddr, uint32_t block_size) {
    if(header_vaddr == 0) {
        return;
    }

    assert(heap_start_vaddr <= header_vaddr && header_vaddr <= heap_end_vaddr );
    assert((header_vaddr & 0x3) == 0x0);
    assert((block_size & 0x07) == 0x0);   // blocksize is 8 Byte aligned

    *(uint32_t *)&heap[header_vaddr] &= 0x00000007;
    *(uint32_t *)&heap[header_vaddr] |= block_size;

}

static uint32_t get_allocated(uint64_t header_vaddr) {
    if(header_vaddr == 0) {
        // null can considered as allocated
        return 1;
    }

    assert(heap_start_vaddr <= header_vaddr && header_vaddr <= heap_end_vaddr);
    assert((header_vaddr & 0x3) == 0x0);
    uint32_t header_value = *(uint32_t *)&heap[header_vaddr];

    return header_value & 0x1;

}

static void set_allocated(uint64_t header_vaddr, uint32_t allocated) {
    if(header_vaddr == 0) {
        return ;
    }

    assert(heap_start_vaddr <= header_vaddr && header_vaddr <= heap_end_vaddr );
    assert((header_vaddr & 0x3) == 0x0);

    *(uint32_t *)&heap[header_vaddr] &= 0xFFFFFFFE;
    *(uint32_t *)&heap[header_vaddr] |= allocated & 0x1;

}

static int is_lastblock(uint64_t vaddr) {

    if(vaddr == 0) {
        return 0;
    }
    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    assert(heap_start_vaddr <= vaddr && vaddr <= heap_end_vaddr);
    assert((vaddr & 0x3) == 0x0);

    uint64_t header_addr = get_header_addr(vaddr);
    uint32_t blocksize = get_blocksize(header_addr);

    // last block don't footer, but its count the footer size
    if(header_addr + blocksize == heap_end_vaddr + 1 + 4) {
        // it is last block

        return 1;
    }

    return 0;
}

static uint64_t get_payload_addr(uint64_t vaddr) {

    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    return round_up(vaddr, 8);
}

static uint64_t get_header_addr(uint64_t vaddr) {
    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    uint64_t payload_addr = get_payload_addr(vaddr);

    // NULL block doesn't have header
    return payload_addr == 0 ? 0 : payload_addr - 4;
}

static uint64_t get_footer_addr(uint64_t vaddr) {
    // vaddr can be 
    // 1. starting address of the block (8 * n + 4)
    // 2. starting address of the payload (8 * m)
    uint64_t next_addr = get_nextheader(vaddr);
    // last block don't have footer
    return next_addr == 0 ? 0 : next_addr - 4;
}

static uint64_t get_nextheader(uint64_t vaddr) {
    if(is_lastblock(vaddr) || vaddr == 0) {
        return 0;
    }

    assert(vaddr >= heap_start_vaddr && vaddr <= heap_end_vaddr);
    // vaddr can be 
    // 1. starting address of the block
    // 2. starting address of the payload
    uint64_t header_vaddr = round_up(vaddr, 8) - 4;
    uint32_t block_size = get_blocksize(header_vaddr);
    uint64_t next_header_vaddr = header_vaddr + block_size;

    assert(heap_start_vaddr <= next_header_vaddr &&
            next_header_vaddr <= heap_end_vaddr);
    
    return next_header_vaddr;

}

static uint64_t get_prevheader(uint64_t vaddr) {

    // vaddr can be 
    // 1. starting address of the block
    // 2. starting address of the payload
    if(vaddr == 0) {
        return 0;
    }
    uint64_t header_vaddr = get_header_addr(vaddr);

    if(header_vaddr == heap_start_vaddr) {
        // this block is first block
        return 0;
    }

    uint64_t prev_footer_vaddr = header_vaddr - 4;
    uint32_t prev_block_size = get_blocksize(prev_footer_vaddr);

    uint64_t prev_header_vaddr = header_vaddr - prev_block_size;

    assert(heap_start_vaddr <= prev_header_vaddr &&
            prev_header_vaddr <= heap_end_vaddr -  12);
    
    return prev_header_vaddr;

}

int heap_init() {
    // heap_start_vaddr is the starting address of the first block
    // the payload of the first block is 8B aligned
    // so the headr address of the first block is 8 - 4 = 4
    heap_start_vaddr = 4;

    set_allocated(heap_start_vaddr, 0);
    set_blocksize(heap_start_vaddr, 4094 - 8);

    heap_end_vaddr = 4096 - 1;
    return 0;
}

// Explicit Free List

static uint32_t get_nextfree(uint64_t header_vaddr) {
    return *(uint32_t* )(header_vaddr + 8);
}

static uint32_t get_prevfree(uint64_t header_vaddr) {
    return *(uint32_t* )(header_vaddr + 4);
}

int check_block(uint64_t header_vaddr) {
    // rule1: block[0] ==> A/F
    // rule2: block[-1] ==> A/F
    // rule3: block[i] == A ==> block[i-1] == A/F && block[i+1] == A/F
    // rule4: block[i] == F ==> block[i-1] == A && block[i+1] == A
    // adjacent free blocks are always merged together.
    // hence external fragmentation are minimized

    assert(header_vaddr % 8 == 4);   // 8 Byte alignment and bias is 4 Byte

    if(get_allocated(header_vaddr) == 1) {
        // rule3 

        return 1;
    }

    uint32_t prev_allocated = 1;
    uint32_t next_allocated = 1;

    if(header_vaddr == heap_start_vaddr) {
        prev_allocated = 1;
    } else {
        prev_allocated = get_allocated(get_prevheader(header_vaddr));
    }

    if(is_lastblock(header_vaddr)) {
        next_allocated = 1;
    } else {
        next_allocated = get_allocated(get_nextheader(header_vaddr));
    }

    if(prev_allocated == 1 && next_allocated == 1) {
        // rule4

        return 1;
    }

    return 0;
}

static uint64_t try_alloc(uint64_t block_vaddr, uint64_t request_blocksize) {

    uint64_t b = block_vaddr;
    uint32_t b_blocksize = get_blocksize(b);    
    uint32_t b_allocated = get_allocated(b);

    if(b_allocated == 0 && b_blocksize >= request_blocksize ) {
        //  allocate this block
        if(b_blocksize > request_blocksize) {
            // need split
            // block - request_blocksize >= 8
            set_allocated(b, 1);
            set_blocksize(b, request_blocksize);

            // set left block
            // the worst situation left 8 Byte, the left block don't have payload;
            uint64_t next_header_vaddr = b + request_blocksize;
            set_blocksize(next_header_vaddr, b_blocksize - request_blocksize);
            set_allocated(next_header_vaddr, 0);
            // TODO set footer?

            return get_payload_addr(b);
        } else {
            // no need split
            set_allocated(b, 1);

            return get_payload_addr(b); 
        }
    }

    return 0;
}

// size - request payload size
// return - the virtual address of payload
static uint64_t mem_alloc(uint32_t size ) {
    assert(0 < size && size < 4096 - 8);

    uint64_t last_block = 0;
    uint64_t b = heap_start_vaddr;
    uint64_t payload_addr = 0;

    uint32_t request_blocksize = round_up(size, 8) + 4 + 4;

    while(b <= heap_end_vaddr) {
        payload_addr = try_alloc(b, request_blocksize);
        if(payload_addr != 0) {

        } else {
            if(is_lastblock(b)) {
                last_block = b;
            }

            b = get_nextfree(b);
        }
    }

    // when no enough free blcok for current heap
    // request a new free physical & virtual page from os
    if(heap_end_vaddr + 4096 <= HEAP_MAX_SIZE) {
        // we can allocated one page from the os
        uint64_t old_heap_end = heap_end_vaddr;
        // TODO: brk system call
        os_syscall_brk(heap_end_vaddr + 1);
        heap_end_vaddr += 4096; 

        uint32_t last_allocated = get_allocated(last_block);
        uint32_t last_blocksize = get_blocksize(last_block);

        if(last_allocated == 1) {
            // no merge

            // add footer for last block
            set_allocated(old_heap_end, 1);
            set_blocksize(old_heap_end, last_blocksize);

            // update first block of new page
            set_allocated(old_heap_end + 4, 0);
            set_blocksize(old_heap_end + 4, 4096);

            // update last block
            last_block = old_heap_end + 4;
        } else {
            // merge last_block
            set_blocksize(last_block, last_blocksize + 4096);
        }
        
        payload_addr = try_alloc(last_block, request_blocksize);

        if(payload_addr != 0) {
            return payload_addr;
        }
    } else {
#ifndef DEBUG_MALLOC
        printf("OS cannot allocate physical page for heap \n");
        exit(0);
#endif
    }

    return 0;
}

void mem_free(uint64_t payload_vaddr) {
    uint64_t vaddr = get_header_addr(payload_vaddr);

    assert(heap_start_vaddr <= vaddr && heap_end_vaddr >= vaddr);
    assert((payload_vaddr & 0x7) == 0x0);  // 8 Byte alignment.

    // request can be first or last block
    uint64_t req = get_header_addr(payload_vaddr);
    // last block req_footer = 0
    uint64_t req_footer = get_footer_addr(vaddr);

    uint32_t req_allocated = get_allocated(req);
    uint32_t req_blocksize = get_blocksize(req); 
    assert(req_allocated == 1);

    // block starting adress of next and prev blocks
    // TODO: corner case, not set footer
    // first block prev = 0
    uint64_t prev = get_prevheader(vaddr);
    // last block next = 0
    uint64_t next = get_nextheader(vaddr);

    // last block next_footer = 0
    uint64_t next_footer = get_footer_addr(next);

    uint32_t prev_allocated = get_allocated(prev);  // first block 1 
    uint32_t next_allocated = get_allocated(next);  // last block 1

    uint32_t prev_blocksize = get_blocksize(prev);  // first block 0
    uint32_t next_blocksize = get_blocksize(next);  // last block 0

    if(next_allocated == 1 && prev_allocated == 1) {
        // case 1: A (A->F) A
        // AFA
        set_allocated(req, 0);

        // if only one block is first and last block
        // footer return 0, nothing happened
        set_allocated(get_footer_addr(req), 0);
        
    } else if (next_allocated == 0 && prev_allocated == 1) {
        // case 2: A (A->F) F A
        // AFFA -> A[FF]A (merge next)
        set_allocated(req, 0);
        set_blocksize(req, req_blocksize + next_blocksize);

        set_allocated(next_footer, 0);
        set_blocksize(next_footer, req_blocksize + next_blocksize);
    } else if (next_allocated == 1 && prev_allocated == 0) {
        // case 3: A F (A->F) A
        // AFFA -> A[FF]A (merge prev)
        set_allocated(prev, 0); 
        set_blocksize(prev, prev_blocksize + req_blocksize);

        set_allocated(req_footer, 0);
        set_blocksize(req_footer, prev_blocksize + req_blocksize);
    } else {
        // case 4: A F (A->F) F A
        // AFFFA -> A[FFF] (merge cur prev next)
        set_allocated(prev, 0);
        set_blocksize(prev, prev_blocksize + req_blocksize + next_blocksize);

        set_allocated(next_footer, 0);
        set_blocksize(next_footer, prev_blocksize + req_blocksize + next_blocksize);

    }
#ifndef DEBUG_MALLOC
    printf("excetipn for free\n");
    exit(0);
#endif
}

// #define DEBUG_MALLOC
#ifdef DEBUG_MALLOC

void test_roundup() {
    printf("Testing round up... \n");

    for(int i = 0; i < 100; i++) {
        for(int j = 1; j <= 8; j++) {
            uint32_t x = i * 8 + j;
            assert(round_up(x, 8) == (i + 1) * 8);
        }
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

void test_get_blocksize_allocated() {
    printf("Testing getting block size from header ... \n");

    for(int i = 4; i < 4096 - 1; i += 4) {
        *(uint32_t *)&heap[i] = 0x1234abc0;
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == 0x0);

        *(uint32_t *)&heap[i] = 0x1234abc1;
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == 0x1);

        *(uint32_t *)&heap[i] = 0x1234abc8;
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == 0x0);

        *(uint32_t *)&heap[i] = 0x1234abc9;
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == 0x1);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

void test_set_blocksize_allocated() {
    printf("Testing set blocksize & allocated... \n");

    for(int i = 4; i <= 4096 - 1; i += 8) {
        set_blocksize(i, 0x1234abc0);
        set_allocated(i, 0);
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == 0x0);

        set_blocksize(i, 0x1234abc0);
        set_allocated(i, 1);
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == 1);

        set_blocksize(i, 0x1234abc8);
        set_allocated(i, 0);
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == 0);

        set_blocksize(i, 0x1234abc8);
        set_allocated(i, 1);
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == 1);
    }

    // test last block size 
    for(int i = 2; i < 100; i++) {
        uint32_t blocksize = i * 8;
        uint64_t addr = 4096 + 4 - blocksize;    // last block count footer but don't have footer in this page

        set_blocksize(addr, blocksize);
        assert(get_blocksize(addr) == blocksize);
        assert(is_lastblock(addr) == 1 );
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

void test_get_header_payload_addr() {
    printf("Testing get payload addr \n");

    uint64_t header_addr, payload_addr;

    for(int i = 8; i <= 4096 - 1; i += 8) {
        payload_addr = i;
        header_addr = i - 4;

        assert(get_payload_addr(payload_addr) == payload_addr);
        assert(get_payload_addr(header_addr) == payload_addr);

        assert(get_header_addr(payload_addr) == header_addr);
        assert(get_header_addr(header_addr) == header_addr);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

void test_get_next_prev() {
    printf("Testing get next & prev \n");

    srand(12345);

    uint64_t h = heap_start_vaddr;
    uint64_t f = 0;
    uint32_t prev_allocated = 1;

    uint32_t collection_blocksize[1000];
    uint32_t collection_allocated[1000];
    uint32_t collection_headeraddr[1000];
    int counter = 0;

    while(h <= heap_end_vaddr) {
        uint32_t blocksize = 8 * (1 + rand() % 8);

        if(heap_end_vaddr - h <= 64) {
            blocksize = 4096 + 4 - h;
        }

        uint32_t allocated = 1;
        if(prev_allocated == 1 && (rand() & 0x1) == 1) {
            allocated = 0;
        }

        collection_blocksize[counter] = blocksize;
        collection_allocated[counter] = allocated;
        collection_headeraddr[counter] = h;

        set_blocksize(h, blocksize);
        set_allocated(h, allocated);

        // set footer
        if(is_lastblock(h) == 0) {
            f = h + blocksize - 4;
            set_blocksize(f, blocksize);
            set_allocated(f, allocated);
        }

        h = h + blocksize;
        prev_allocated = allocated;
        counter += 1;
    }

    // test get next header
    h = heap_start_vaddr;
    int i = 0;
    while(is_lastblock(h) == 0) {
        assert(i < counter);
        assert(h == collection_headeraddr[i]);
        assert(get_blocksize(h) == collection_blocksize[i]);
        assert(get_allocated(h) == collection_allocated[i]);
        assert(check_block(h) == 1);

        h = get_nextheader(h);
        i += 1;
    }

    assert(is_lastblock(h));

    i = counter - 1;
    while(heap_end_vaddr <= h) {
        assert(0 <= i);
        assert(h == collection_headeraddr[i]);
        assert(get_blocksize(h) == collection_blocksize[i]);
        assert(get_allocated(h) == collection_allocated[i]);
        assert(check_block(h) == 1);

        h = get_prevheader(h);
        i -= 1;
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

#endif

int main() {

#ifdef DEBUG_MALLOC
    printf("csapp malloc test begin \n");
    test_roundup();

    test_get_blocksize_allocated();

    test_set_blocksize_allocated();

    test_get_header_payload_addr();

    test_get_next_prev();

#endif
}