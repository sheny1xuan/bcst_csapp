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
#include <string.h>
#include "headers/cpu.h"
#include "headers/memory.h"
#include "headers/common.h"
#include "headers/address.h"


// each swap file is swap page
// each line of this swap page is one nuit64

#define SWAP_PAGE_FILE_LINES 512

int swap_in(uint64_t daddr, uint64_t ppn) {
    FILE* fr;
    char filename[128];

    sprintf(filename, "../files/swap/page-%ld.txt", daddr);
    fr = fopen(filename, 'r');

    assert(fr != NULL);

    // base address of physics address.
    uint64_t ppn_ppo = ppn << PHYSICAL_PAGE_OFFSET_LENGTH;
    
    char buf[64] = {'0'};
    for(int i = 0; i < SWAP_PAGE_FILE_LINES; i++) {
        char* str = fgets(buf, 64, fr);

        *(uint64_t*)(&pm[ppn_ppo + i * 8]) = string2uint(str);
    }

    fclose(fr);
}

int swap_out(uint64_t daddr, uint64_t ppn) {
    FILE* fw;
    char filename[128];

    sprintf(filename, "../files/swap/page-%ld.txt", daddr);
    fw = fopen(filename, 'w');

    assert(fw != NULL);

    // base address of physics address.
    uint64_t ppn_ppo = ppn << PHYSICAL_PAGE_OFFSET_LENGTH;
    for(int i = 0; i < SWAP_PAGE_FILE_LINES; i++) {
        fprintf(fw, "0x%16lx\n", *(uint64_t *)(&pm[ppn_ppo + i * 8]));
    }

    fclose(fw);
}