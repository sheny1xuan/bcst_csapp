#ifndef MEM_ALLOCATOR_H
#define MEM_ALLOCATOR_H

#include <stdint.h>

#define MIN_EXPLICIT_FREE_LIST_BLOCKSIZE (16)
// heap's range:
// [heap_start_vaddr, heap_end_vaddr)
// [0, 4) 4 Bytes -> not used as cover
// [4, 12) 8 Bytes -> used as prologue block
// [12, 4096 * n - 4) -> regular block
// [4096 * n - 4, 4096 * (n + 1)) 4 Byte -> used as epilogue block
uint64_t heap_start_vaddr;
uint64_t heap_end_vaddr;

#define HEAP_MAX_SIZE (4096 * 8)
uint8_t heap[HEAP_MAX_SIZE];

#define FREE (0)
#define ALLOCATED (1)
#define NIL (0)

// to allocate one physical page for heap
uint32_t extend_heap(uint32_t size);
void os_syscall_brk();

// round up
uint64_t round_up(int x, int alignment);

// get block information by a header_vaddr or payload_vaddr
uint32_t get_blocksize(uint64_t header_vaddr);
void set_blocksize(uint64_t header_vaddr, uint32_t block_size);

uint32_t get_allocated(uint64_t header_vaddr);
void set_allocated(uint64_t header_vaddr, uint32_t allocated);

uint64_t get_payload(uint64_t vaddr);
uint64_t get_header(uint64_t vaddr);
uint64_t get_footer(uint64_t vaddr);

// operation for block list
uint64_t get_nextheader(uint64_t vaddr);
uint64_t get_prevheader(uint64_t vaddr);

int is_firstblock(uint64_t vaddr);
int is_lastblock(uint64_t vaddr);

uint64_t get_firstblock();
uint64_t get_lastblock();
uint64_t get_prologue();
uint64_t get_epilogue();

uint64_t get_field32_block_ptr(uint64_t header_vaddr, uint32_t min_blocksize, uint32_t offset); 
void set_field32_block_ptr(uint64_t header_vaddr, uint64_t block_ptr, uint32_t min_blocksize, uint32_t offset);

// interface
int heap_init();
uint64_t mem_alloc(uint32_t size);
void mem_free(uint64_t payload_vaddr);

// debug
void check_heap_correctness();

#endif