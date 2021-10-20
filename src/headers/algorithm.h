/* BCST - Introduction to Computer Systems
 * Author:      yangminz@outlook.com
 * Github:      https://github.com/yangminz/bcst_csapp
 * Bilibili:    https://space.bilibili.com/4564101
 * Zhihu:       https://www.zhihu.com/people/zhao-yang-min
 * This project (code repository and videos) is exclusively owned by yangminz 
 * and shall not be used for commercial and profitting purpose 
 * without yangminz's permission.
 */

// include guards to prevent double declaration of any identifiers 
// such as types, enums and static variables
#ifndef DATASTRUCTURE_GUARD
#define DATASTRUCTURE_GUARD

#include <stdint.h>

#define NULL_ID (0)
//  The following data structures and algorithms
//  are designed to be Generic. To use, the user
//  should design their own data structure and
//  implement the interface with function pointers.
//  With this design, the actual data structure
//  can be very free. It can be malloced on heap.
//  And it can be a byte-block like in ./src/malloc
/*======================================*/
/*      Circular Doubly Linked List     */
/*======================================*/

// Define the interface for generic linked list node structure
typedef struct
{
    // "malloc" the memory of a node
    uint64_t (*construct_node)();
    // "free" the memory of a node
    int (*destruct_node)(uint64_t);

    // <uint64_t> "node": the id of node
    // return <uint64_t>: 1 if node is null
    int (*is_null_node)(uint64_t);

    // <uint64_t> "first": the id of first node
    // <uint64_t> "second": the id of second node
    // return <uint64_t>: 0 if they are the same
    int (*compare_nodes)(uint64_t, uint64_t);

    // <uint64_t> "node": the id of current node
    // return <uint64_t>: the id of the previous node
    uint64_t (*get_node_prev)(uint64_t);
    // <uint64_t> "node": the id of current node
    // <uint64_t> "prev": the id of previous node
    // return <int>: 1 if the setting is successful
    int (*set_node_prev)(uint64_t, uint64_t);

    // <uint64_t> "node": the id of current node
    // return <uint64_t>: the id of the next node
    uint64_t (*get_node_next)(uint64_t);
    // <uint64_t> "node": the id of current node
    // <uint64_t> "next": the id of next node
    // return <int>: 1 if the setting is successful
    int (*set_node_next)(uint64_t, uint64_t);

    // <uint64_t> "node": the id of current node
    // return <uint64_t>: the value of node
    uint64_t (*get_node_value)(uint64_t);
    // <uint64_t> "node": the id of current node
    // <uint64_t> "value": the value of the node
    // return <int>: 1 if the setting is successful
    int (*set_node_value)(uint64_t, uint64_t);
} linkedlist_node_interface;


#define ILLIST_CONSTRUCT (1 << 0)
#define ILLIST_DESTRUCT (1 << 1)
#define ILLIST_CHECKNULL (1 << 2)
#define ILLIST_COMPARE (1 << 3)
#define ILLIST_PREV (1 << 4)
#define ILLIST_NEXT (1 << 5)
#define ILLIST_VALUE (1 << 6)
void linkedlist_validate_interface(linkedlist_node_interface *i_node,
    uint64_t flags);

// internal class of the linked list
typedef struct LINKEDLIST_INTERNAL_STRUCT {
    uint64_t head;
    uint64_t count;

    // this: this pointer
    // <uint64_t> "node": the if of new head node
    // return <int>: 1 if the updating is successful
    int (*update_head)(struct LINKEDLIST_INTERNAL_STRUCT* this, uint64_t);
} linkedlist_internal_t;

// The linked list implementation open to other data structures
// especially useful for malloc explicit list implementation
// can be treat as base class `linkedlist_internal_t` method.
int linkedlist_internal_add(linkedlist_internal_t *list, 
    linkedlist_node_interface *i_node, 
    uint64_t value);
int linkedlist_internal_insert(linkedlist_internal_t *list, 
    linkedlist_node_interface *i_node, 
    uint64_t node);
int linkedlist_internal_delete(linkedlist_internal_t *list, 
    linkedlist_node_interface *i_node, 
    uint64_t node);
uint64_t linkedlist_internal_index(linkedlist_internal_t *list,
    linkedlist_node_interface *i_node, 
    uint64_t index);
uint64_t linkedlist_internal_next(linkedlist_internal_t *list,
    linkedlist_node_interface *i_node);

typedef struct LINKED_LIST_NODE_STRUCT
{
    uint64_t value;
    struct LINKED_LIST_NODE_STRUCT *prev;
    struct LINKED_LIST_NODE_STRUCT *next;
} linkedlist_node_t;

// 
// the implementation of default linked list 
// 
typedef union
{
    linkedlist_internal_t base;
    // public
    struct {
        uint64_t head;
        uint64_t count;
    };
} linkedlist_t;

linkedlist_t *linkedlist_construct();
void linkedlist_free(linkedlist_t *list);
void linkedlist_add(linkedlist_t *list, uint64_t value);
void linkedlist_delete(linkedlist_t *list, linkedlist_node_t *node);
linkedlist_node_t *linkedlist_index(linkedlist_t *list, uint64_t value);
linkedlist_node_t *linkedlist_next(linkedlist_t *list);

/*======================================*/
/*      Dynamic Array                   */
/*======================================*/
typedef struct
{
    uint32_t size;
    uint32_t count;
    uint64_t *table;
} array_t;

array_t *array_construct(int size);
void array_free(array_t *arr);
array_t *array_insert(array_t *arr, uint64_t value);
int array_delete(array_t *arr, int index);
int array_get(array_t *arr, int index, uint64_t *valptr);

/*======================================*/
/*      Extendible Hash Table           */
/*======================================*/
typedef struct
{
    int localdepth;     // the local depth
    int counter;        // the counter of slots (have data)
    char **karray;
    uint64_t *varray;
} hashtable_bucket_t;

typedef struct
{
    int num;            // number of buckets = 1 << globaldepth
    int globaldepth;    // the global depth

    int size;           // the size of (key, value) tuples of each bucket
    hashtable_bucket_t **directory;    // the internal table is actually an array
} hashtable_t;

hashtable_t *hashtable_construct(int size);
void hashtable_free(hashtable_t *tab);
int hashtable_get(hashtable_t *tab, char *key, uint64_t *valptr);
hashtable_t *hashtable_insert(hashtable_t *tab, char *key, uint64_t val);

/*======================================*/
/*      Trie - Prefix Tree              */
/*======================================*/
typedef struct TRIE_NODE_STRUCT
{
    hashtable_t *next;
    uint64_t value;
    int isvalue;
} trie_node_t;

trie_node_t * trie_construct();
void trie_free(trie_node_t *root);
trie_node_t *trie_insert(trie_node_t *root, char *key, uint64_t value);
int trie_get(trie_node_t *root, char *key, uint64_t *valptr);

/*======================================*/
/*      Red Black Tree                  */
/*======================================*/
typedef enum
{
    COLOR_RED,
    COLOR_BLACK,
} rb_color_t;

typedef struct RB_NODE_STRUCT
{
    // pointers
    struct RB_NODE_STRUCT *parent;

    union
    {
        struct RB_NODE_STRUCT *childs[2];
        struct
        {
            struct RB_NODE_STRUCT *left;        // childs[0]
            struct RB_NODE_STRUCT *right;       // childs[1]
        };
    };

    // edge color to parent
    rb_color_t color;

    // tree node value
    uint64_t value;
} rb_node_t;

rb_node_t *rb_insert(rb_node_t *root, uint64_t val);
rb_node_t *rb_delete(rb_node_t *root, uint64_t val);
rb_node_t *rb_find(rb_node_t *root, uint64_t val);

/*======================================*/
/*      Binary Search Tree              */
/*======================================*/

rb_node_t *bst_insert(rb_node_t *root, uint64_t val);
rb_node_t *bst_delete(rb_node_t *root, uint64_t val);
rb_node_t *bst_find(rb_node_t *root, uint64_t val);

#endif
