/* BCST - Introduction to Computer Systems
 * Author:      yangminz@outlook.com
 * Github:      https://github.com/yangminz/bcst_csapp
 * Bilibili:    https://space.bilibili.com/4564101
 * Zhihu:       https://www.zhihu.com/people/zhao-yang-min
 * This project (code repository and videos) is exclusively owned by yangminz 
 * and shall not be used for commercial and profitting purpose 
 * without yangminz's permission.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "../headers/allocator.h"
#include "../headers/algorithm.h"

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

    heap_init();
    printf("Testing getting block size from header ... \n");

    for(int i = get_prologue() + 16; i <= get_epilogue(); i += 4) {
        *(uint32_t *)&heap[i] = 0x1234abc0;
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == FREE);

        *(uint32_t *)&heap[i] = 0x1234abc1;
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == ALLOCATED);

        *(uint32_t *)&heap[i] = 0x1234abc8;
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == FREE);

        *(uint32_t *)&heap[i] = 0x1234abc9;
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == ALLOCATED);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

void test_set_blocksize_allocated() {
    heap_init();
    printf("Testing set blocksize & allocated... \n");

    for(int i = get_firstblock(); i < get_epilogue(); i += 4) {
        set_blocksize(i, 0x1234abc0);
        set_allocated(i, FREE);
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == FREE);

        set_blocksize(i, 0x1234abc0);
        set_allocated(i, ALLOCATED);
        assert(get_blocksize(i) == 0x1234abc0);
        assert(get_allocated(i) == ALLOCATED);

        set_blocksize(i, 0x1234abc8);
        set_allocated(i, FREE);
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == FREE);

        set_blocksize(i, 0x1234abc8);
        set_allocated(i, ALLOCATED);
        assert(get_blocksize(i) == 0x1234abc8);
        assert(get_allocated(i) == ALLOCATED);
    }

    // test last block size 
    for(int i = 2; i < 100; i++) {
        uint32_t blocksize = i * 8;
        uint64_t addr = get_epilogue() - blocksize;    // last block count footer but don't have footer in this page

        set_blocksize(addr, blocksize);
        assert(get_blocksize(addr) == blocksize);
        assert(is_lastblock(addr) == 1 );
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

void test_get_header_payload_addr() {
    printf("Testing get payload addr \n");

    heap_init();

    uint64_t header_addr, payload_addr;
    for(int i = get_payload(get_firstblock()); i < get_epilogue(); i += 8) {
        payload_addr = i;
        header_addr = i - 4;

        assert(get_payload(payload_addr) == payload_addr);
        assert(get_payload(header_addr) == payload_addr);

        assert(get_header(payload_addr) == header_addr);
        assert(get_header(header_addr) == header_addr);
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

void test_get_next_prev() {
    printf("Testing get next & prev \n");

    srand(12345);

    uint64_t h = get_firstblock();
    uint64_t f = NIL;

    uint32_t collection_blocksize[1000];
    uint32_t collection_allocated[1000];
    uint32_t collection_headeraddr[1000];
    int counter = 0;

    uint32_t allocated = 1;
    uint64_t epilogue = get_epilogue();
    while(h < epilogue) {
        uint32_t blocksize = 8 * (1 + rand() % 16);

        if(epilogue - h <= 64) {
            blocksize = epilogue - h;
        }

        if(allocated == ALLOCATED && (rand() % 3) >= 1) {
            allocated = FREE;
        } else {
            allocated = ALLOCATED;
        }

        collection_blocksize[counter] = blocksize;
        collection_allocated[counter] = allocated;
        collection_headeraddr[counter] = h;

        set_blocksize(h, blocksize);
        set_allocated(h, allocated);

        f = h + blocksize - 4;
        set_blocksize(f, blocksize);
        set_allocated(f, allocated);

        h = h + blocksize;
        counter += 1;
    }

    // test get next header
    h = get_firstblock();
    int i = 0;
    while(is_lastblock(h) == 0) {
        assert(i < counter);
        assert(h == collection_headeraddr[i]);
        assert(get_blocksize(h) == collection_blocksize[i]);
        assert(get_allocated(h) == collection_allocated[i]);
        // assert(check_block(h) == 1);

        h = get_nextheader(h);
        i += 1;
    }

    h = get_lastblock();
    i = counter - 1;
    while(get_firstblock() <= h) {
        assert(0 <= i);
        assert(h == collection_headeraddr[i]);
        assert(get_blocksize(h) == collection_blocksize[i]);
        assert(get_allocated(h) == collection_allocated[i]);
        // assert(check_block(h) == 1);

        h = get_prevheader(h);
        i -= 1;
    }

    printf("\033[32;1m\tPass\033[0m\n");
}

linkedlist_t* ptrs;

static void test_malloc_free()
{
    printf("Testing malloc & free ...\n");

    heap_init();
    // check_heap_correctness();

    srand(123456);
    ptrs = linkedlist_construct();
    
    for (int i = 0; i < 100; ++ i)
    {
        if ((rand() & 0x1) == 0)
        {
            // malloc
            uint32_t size = rand() % 1024 + 1;  // a non zero value
            uint64_t p = mem_alloc(size);

            if (p != NIL)
            {
                linkedlist_add(ptrs, p);
            }
        }
        else if (ptrs->count != 0)
        {
            // free
            // randomly select one to free
            int random_index = rand() % ptrs->count;
            linkedlist_node_t *t = linkedlist_index(ptrs, random_index);

            mem_free(t->value);
            linkedlist_delete(ptrs, t);
        }
    }

    int num_still_allocated = ptrs->count;
    for (int i = 0; i < num_still_allocated; ++ i)
    {
        linkedlist_node_t *t = linkedlist_next(ptrs);
        mem_free(t->value);
        linkedlist_delete(ptrs, t);
    }
    assert(ptrs->count == 0);
    linkedlist_free(ptrs);

    // // finally there should be only one free block
    // assert(is_lastblock(get_firstblock()) == 1);
    // assert(get_allocated(get_firstblock()) == FREE);
    // check_heap_correctness();

    printf("\033[32;1m\tPass\033[0m\n");
}


int main() {


    printf("csapp malloc test begin \n");
    test_roundup();

    test_get_blocksize_allocated();

    test_set_blocksize_allocated();

    test_get_header_payload_addr();

    test_get_next_prev();

    test_malloc_free();

}