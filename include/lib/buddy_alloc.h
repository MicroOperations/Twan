#ifndef _BUDDY_ALLOC_H_
#define _BUDDY_ALLOC_H_

#include <include/subsys/mem/pma.h>

#define BUDDY_32GB_SHIFT 35

#define BUDDY_BASE_ORDER PMA_BASE_ORDER
#define BUDDY_MAX_ORDER BUDDY_32GB_SHIFT
#define BUDDY_NUM_ORDERS (BUDDY_MAX_ORDER - BUDDY_BASE_ORDER + 1)
STATIC_ASSERT(BUDDY_NUM_ORDERS > 0);

#define BUDDY_ORDER_BITMAP_SIZE ((BUDDY_NUM_ORDERS + 7) / 8)

#define BUDDY_MIN_ARENA_SIZE (1ULL << BUDDY_BASE_ORDER)
#define BUDDY_MAX_ARENA_SIZE (1ULL << BUDDY_MAX_ORDER)

STATIC_ASSERT(BUDDY_MIN_ARENA_SIZE > 0);
STATIC_ASSERT(BUDDY_MAX_ARENA_SIZE > 0);

#define BUDDY_MAX_BLOCKS (BUDDY_MAX_ARENA_SIZE / BUDDY_MIN_ARENA_SIZE)
#define BUDDY_BLOCK_BITMAP_SIZE (((BUDDY_MAX_BLOCKS * 2) + 7) / 8)

struct buddy_arena
{
    u64 base_addr;

    u8 order_bitmap[BUDDY_ORDER_BITMAP_SIZE];
    u8 block_bitmap[BUDDY_BLOCK_BITMAP_SIZE];
};

inline u32 __buddy_get_order(u32 num_pages)
{
    u32 order = 0;
    size_t size = 1;
    while (size < num_pages) {
        size <<= 1;
        order++;
    }

    return order;
}

inline bool __buddy_bitmap_test(u8 *bitmap, u64 bit)
{
    return (bitmap[bit / 8] & (1ULL << (bit % 8))) != 0;
}

inline void __buddy_bitmap_set(u8 *bitmap, u64 bit)
{
    bitmap[bit / 8] |= (1ULL << (bit % 8));
}

inline void __buddy_bitmap_clear(u8 *bitmap, u64 bit)
{
    bitmap[bit / 8] &= ~(1ULL << (bit % 8));
}

inline void __buddy_bitmap_flip(u8 *bitmap, u64 bit)
{
    bitmap[bit / 8] ^= (1ULL << (bit % 8));
}

inline bool __buddy_is_order_free(struct buddy_arena *arena, u32 order)
{
    return __buddy_bitmap_test(arena->order_bitmap, order);
}

inline void __buddy_order_set_free(struct buddy_arena *arena, u32 order)
{
    __buddy_bitmap_set(arena->order_bitmap, order);
}

inline void __buddy_order_clear(struct buddy_arena *arena, u32 order)
{
    __buddy_bitmap_clear(arena->order_bitmap, order);
}

inline void __buddy_order_flip(struct buddy_arena *arena, u32 order)
{
    __buddy_bitmap_flip(arena->order_bitmap, order);
}

inline u64 buddy_order_num_blocks(u32 order)
{
    return BUDDY_MAX_BLOCKS >> order;
}

inline u64 __buddy_block_bitmap_bit(u32 order, u64 block_idx)
{
    u32 tree_order = (BUDDY_NUM_ORDERS - 1) - order;
    return ((1ULL << tree_order) - 1) + block_idx;
}

inline bool __buddy_is_block_free(struct buddy_arena *arena, u32 order, 
                                  u64 block_idx)
{
    u64 bit = __buddy_block_bitmap_bit(order, block_idx);
    return __buddy_bitmap_test(arena->block_bitmap, bit);
}

inline void __buddy_block_set_free(struct buddy_arena *arena, u32 order,
                                   u64 block_idx)
{
    u64 bit = __buddy_block_bitmap_bit(order, block_idx);
    __buddy_bitmap_set(arena->block_bitmap, bit);
}

inline void __buddy_block_clear(struct buddy_arena *arena, u32 order,
                                u64 block_idx)
{
    u64 bit = __buddy_block_bitmap_bit(order, block_idx);
    __buddy_bitmap_clear(arena->block_bitmap, bit);
}

inline void __buddy_block_flip(struct buddy_arena *arena, u32 order,
                              u64 block_idx)
{
    u64 bit = __buddy_block_bitmap_bit(order, block_idx);
    __buddy_bitmap_flip(arena->block_bitmap, bit);
}

inline u64 buddy_addr_to_block_idx(struct buddy_arena *arena, u64 addr, 
                                   u32 order)
{
    return (addr - arena->base_addr) >> (order + PMA_BASE_ORDER);
}

inline u64 buddy_block_idx_to_addr(struct buddy_arena *arena, u64 block_idx, 
                                   u32 order)
{
    return arena->base_addr + (block_idx << (order + PMA_BASE_ORDER));
}

inline u64 buddy_get_idx(u64 block_idx)
{
    return block_idx ^ 1;
}

inline u64 buddy_get_parent_idx(u64 block_idx)
{
    return block_idx >> 1;
}

inline long __buddy_find_block_at_order(struct buddy_arena *arena, u32 order)
{
    /* TODO: optimise this to scan full bytes rather than just bits */

    if (!__buddy_is_order_free(arena, order))
        return -1;

    u64 num_blocks = buddy_order_num_blocks(order);

    for (u32 i = 0; i < num_blocks; i++) {
        
        if (__buddy_is_block_free(arena, order, i))
            return i;
    }

    return -1;
}

inline void __buddy_update_order_bitmap(struct buddy_arena *arena, u32 order)
{
    if (__buddy_find_block_at_order(arena, order) < 0)
        __buddy_order_clear(arena, order);
    else
        __buddy_order_set_free(arena, order);   
}

inline bool __buddy_order_can_alloc(struct buddy_arena *arena, u32 order)
{
    if (order >= BUDDY_NUM_ORDERS)
        return false;

    for (u32 i = order; i < BUDDY_NUM_ORDERS; i++) {
        
        if (__buddy_is_order_free(arena, i))
            return true;
    }

    return false;
}

int buddy_arena_init(struct buddy_arena *arena, u64 base_addr, size_t size);

u64 __buddy_alloc(struct buddy_arena *arena, u32 order);
void __buddy_free(struct buddy_arena *arena, u64 addr, u32 order);

#endif