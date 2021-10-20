#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include "../headers/allocator.h"
#include "../headers/algorithm.h"

// Operations for Free List Block structure

static int compare_node(uint64_t first, uint64_t second) {
    return !(first == second);
}

static int is_null_node(uint64_t node_id) {
    return node_id == NULL_ID;
}