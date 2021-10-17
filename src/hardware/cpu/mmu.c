/* BCST - Introduction to Computer Systems
 * Author:      yangminz@outlook.com
 * Github:      https://github.com/yangminz/bcst_csapp
 * Bilibili:    https://space.bilibili.com/4564101
 * Zhihu:       https://www.zhihu.com/people/zhao-yang-min
 * This project (code repository and videos) is exclusively owned by yangminz 
 * and shall not be used for commercial and profitting purpose 
 * without yangminz's permission.
 */

// Memory Management Unit 
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/common.h"
#include "headers/address.h"

#define NUM_TLB_CACHE_LINE_PER_SET (8)

// TLB cache
//  -------------------------

typedef struct 
{
    int    valid;
    uint64_t tag;
    uint64_t ppn;
} tlb_cacheline_t;

typedef struct
{
    tlb_cacheline_t lines[NUM_TLB_CACHE_LINE_PER_SET];
} tlb_cacheset_t;

typedef struct
{
    tlb_cacheset_t sets[(1 << TLB_CACHE_INDEX_LENGTH)];
} tlb_cache_t;

static tlb_cache_t mmu_tlb;

static uint64_t page_walk(uint64_t vaddr_value);
static void page_fault_handler();

static uint64_t read_tlb(uint64_t vaddr_value, uint64_t* paddr_ptr);
static int write_tlb(uint64_t vaddr_value, uint64_t paddr_value);

static uint64_t read_tlb(uint64_t vaddr_value, uint64_t* paddr_ptr) {
    address_t addr;
    addr.address_value = vaddr_value;

    tlb_cacheset_t* set = &mmu_tlb.sets[addr.tlbi];
    for(int i = 0; i < NUM_TLB_CACHE_LINE_PER_SET; i++ ) {
        tlb_cacheline_t* line = &set->lines[i];

        if(line->tag == addr.tlbi && 
            line->valid == 1) {
                // TLB read hit
                *paddr_ptr = line->ppn;
                return 1;
        }
    }

    *paddr_ptr = NULL;
    return 0;
}

static int write_tlb(uint64_t vaddr_value, uint64_t paddr_value) {
    address_t vaddr = {
        .vaddr_value = vaddr_value,
    };

    address_t paddr = {
        .paddr_value = paddr_value,
    };
    int free_tlb_line_idx = -1;

    tlb_cacheset_t* set = &mmu_tlb.sets[vaddr.tlbi];

    // have free tlb line
    for(int i = 0; i < NUM_TLB_CACHE_LINE_PER_SET; i++ ) {
        tlb_cacheline_t* line = &set->lines[i];

        if(line->valid == 0 ) {
            free_tlb_line_idx = i;
            break;
        }
    }

    if(0 <= free_tlb_line_idx && free_tlb_line_idx < NUM_TLB_CACHE_LINE_PER_SET) {
        tlb_cacheline_t* line = &set->lines[free_tlb_line_idx];

        line->valid = 1;
        line->ppn = paddr.ppn;
        line->tag = vaddr.tlbt;

        return 1;
    }

    // no free tlb line
    // random choose a victim
    int random_victim_index = random() % NUM_TLB_CACHE_LINE_PER_SET;


    tlb_cacheline_t* line = &set->lines[random_victim_index];

    line->valid = 1;
    line->ppn = paddr.ppn;
    line->tag = vaddr.tlbt;

    return 1;
}

// consider this function va2pa as functional
uint64_t va2pa(uint64_t vaddr)
{
    uint64_t paddr = 0;
#ifdef USE_TLB_HARDWARE
    int tlb_hit = read_tlb(vaddr, paddr);

    if(tlb_hit == 1) {
        // TLB read hit
        return paddr;
    }
    // TLB read miss
#endif

    // assume that page_walk consume much time
    paddr = page_walk(vaddr);

#ifdef USE_TLB_HARDWARE
    // refresh TLB
    // TODO: check if this paddr is safe
    if(paddr != 0 ) {
        // TLB write
        if(write_tlb(vaddr, paddr) == 1) {
            return paddr;
        }

    }
#endif

    return paddr;
}

// input - virtual address
// output - physical address
static uint64_t page_walk(uint64_t vaddr_value)
{
    address_t vaddr = {
        .vaddr_value = vaddr_value
    };
    int page_table_size = PAGE_TABLE_ENTRY_NUM * sizeof(pte123_t);  // should be 4KB

    // CR3 register's value is malloced on the heap of the simulator
    pte123_t *pgd = (pte123_t *)cpu_controls.cr3;
    assert(pgd != NULL);
    
    if (pgd[vaddr.vpn1].present == 1)
    {
        // PHYSICAL PAGE NUMBER of the next level page table
        // aka. high bits starting address of the page table
        pte123_t *pud = pgd[vaddr.vpn1].paddr;

        if (pud[vaddr.vpn2].present == 1)
        {
            // find pmd ppn

            pte123_t *pmd = (pte123_t *)(pud[vaddr.vpn2].paddr);

            if (pmd[vaddr.vpn3].present == 1)
            {
                // find pt ppn
                
                pte4_t *pt = (pte4_t *)(pmd[vaddr.vpn3].paddr);

                if (pt[vaddr.vpn4].present == 1)
                {
                    // find page table entry
                    address_t paddr = {
                        .ppn = pt[vaddr.vpn4].ppn,
                        .ppo = vaddr.vpo    // page offset inside the 4KB page
                    };

                    return paddr.paddr_value;
                }
                else
                {
                    // page table entry not exists

                    // TODO: raise exception 14 (page fault) here.
#ifdef DBUEG_PAGE_WALK
                    printf("page walk level 4: pt[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn1);
#endif
                    pte4_t *pt = malloc(page_table_size);
                    memset(pt, 0, page_table_size);

                    // set page table entry
                    pmd[vaddr.vpn3].present = 1;
                    pud[vaddr.vpn3].paddr   = (uint64_t)pt;

                    // TODO: page fault here
                    // map the physical page and the virtual page
                    exit(0);
                }
            }
            else
            {
                // pt - level 4 not exists
#ifdef DBUEG_PAGE_WALK
                printf("page walk level 3: pmd[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn1);
#endif
                pte4_t *pt = malloc(page_table_size);
                memset(pt, 0, page_table_size);

                // set page table entry
                pmd[vaddr.vpn3].present = 1;
                pud[vaddr.vpn3].paddr   = (uint64_t)pt;

                // TODO: page fault here
                // map the physical page and the virtual page
                exit(0);
            }
        }
        else
        {
            // pmd - level 3 not exists
#ifdef DBUEG_PAGE_WALK
            printf("page walk level 2: pud[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn1);
#endif
            pte123_t *pmd = malloc(page_table_size);
            memset(pmd, 0, page_table_size);

            // set page table entry
            pud[vaddr.vpn2].present = 1;
            pud[vaddr.vpn2].paddr   = (uint64_t)pmd;

            // TODO: page fault here
            // map the physical page and the virtual page
            exit(0);
        }
    }
    else
    {
        // pud - level 2 not exists
#ifdef DBUEG_PAGE_WALK
        printf("page walk level 1: pgd[%lx].present == 0\n\tmalloc new page table for it\n", vaddr.vpn1);
#endif
        pte123_t *pud = malloc(page_table_size);
        memset(pud, 0, page_table_size);

        // set page table entry
        pgd[vaddr.vpn1].present = 1;
        pgd[vaddr.vpn1].paddr   = (uint64_t)pud;

        // TODO: page fault here
        // map the physical page and the virtual page
        exit(0);
    }
}

static void page_fault_handler(pte4_t* pte, address_t vaddr) {

    // not in physical memory
    assert(pte->present == 0);
    // select a physics page to make map
    int ppn = -1;
    pte4_t* victim;
    pte4_t prev; 
    // 1. try to request one free physics page to fill this vp
    // kernel's responsibility
    for (int i = 0; i < MAX_NUM_PHYSICAL_PAGE; i++) {

        if (page_map[i].pte4->present == 0) {
            
            printf("PageFault: use free ppn %d \n", i);

            ppn = i;

            // reverse map
            page_map[ppn].allocated = 1;
            page_map[ppn].dirty = 0;
            page_map[ppn].time = 0;
            page_map[ppn].pte4 = pte;

            // va2pa
            pte->present = 1;
            pte->ppn = ppn;
            pte->dirty = 0;

            return ;
        }    
    }

    // 2. no free ppn, select one clean page
    // in this case, this no DRAM -> DISK TRANSACTION
    int lru_ppn = -1;
    int lru_time = -1;

    for(int i = 0; i < MAX_NUM_PHYSICAL_PAGE; i++ ) {
        
        if(page_map[i].pte4->dirty == 0 && lru_time < page_map[i].time) {
            
            lru_ppn = i;
            lru_time = page_map[i].time;
            
        }
    }

    if(lru_ppn != -1 && lru_ppn < MAX_NUM_PHYSICAL_PAGE) {

        ppn = lru_ppn;

        victim = page_map[ppn].pte4;

        // clear victim flag;
        victim->pte_value = 0;
        victim->present = 0;    // swap out in disk
        victim->daddr = page_map[ppn].daddr;

        // load page from disk to physical memory
        uint64_t daddr = pte->daddr;
        swap_in(daddr, ppn);

        // swap in
        pte->pte_value = 0;
        pte->present = 1;
        pte->ppn = ppn;
        pte->dirty = 0;

        page_map[ppn].allocated = 1;
        page_map[ppn].time = 0;
        page_map[ppn].dirty = 0;
        page_map[ppn].pte4 = pte;
        page_map[ppn].daddr = daddr;

        return; 
    }

    // 3. no clean ppn, find a victim by LRU policy
    // write back dirty victim (swap out)
    lru_ppn = -1;
    lru_time = -1;

    for(int i = 0; i < MAX_NUM_PHYSICAL_PAGE; i++ ) {
        
        if(lru_time < page_map[i].time) {
            
            lru_ppn = i;
            lru_time = page_map[i].time;
            
        }
    }
    
    assert(lru_ppn >= 0 && lru_ppn < MAX_NUM_PHYSICAL_PAGE);

    ppn = lru_ppn;

    victim = page_map[ppn].pte4;

    swap_out(page_map[ppn].daddr, ppn);

    // clear victim flag;
    victim->pte_value = 0;
    victim->present = 0;    // swap out in disk
    victim->daddr = page_map[ppn].daddr;

    // load page from disk to physical memory
    uint64_t daddr = pte->daddr;
    swap_in(daddr, ppn);

    // swap in
    pte->pte_value = 0;
    pte->present = 1;
    pte->ppn = ppn;
    pte->dirty = 0;

    page_map[ppn].allocated = 1;
    page_map[ppn].time = 0;
    page_map[ppn].dirty = 0;
    page_map[ppn].pte4 = pte;
    page_map[ppn].daddr = daddr;


} 