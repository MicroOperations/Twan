#include <lib/buddy_alloc.h>
#include <errno.h>

int buddy_arena_init(struct buddy_arena *arena, u64 base_addr, size_t size)
{
    if (base_addr == PMA_NULL)
        return -EINVAL;

    u64 aligned_addr = p2align_up(base_addr, BUDDY_MIN_ARENA_SIZE);

    /* wraparound occured (PMA_NULL is 0) */
    if (aligned_addr < base_addr || aligned_addr == PMA_NULL)
        return -EINVAL;

    size_t cutoff = aligned_addr - base_addr;
    if (cutoff > size)
        return -EINVAL;

    size -= cutoff;
    size = p2align_down(size, BUDDY_MIN_ARENA_SIZE);

    if (size < BUDDY_MIN_ARENA_SIZE)
        return -EINVAL;

    if (size > BUDDY_MAX_ARENA_SIZE)
        size = BUDDY_MAX_ARENA_SIZE;

    memset(arena->order_bitmap, 0, sizeof(arena->order_bitmap));
    memset(arena->block_bitmap, 0, sizeof(arena->block_bitmap));

    arena->base_addr = aligned_addr;

    u32 order = log2(size) - BUDDY_BASE_ORDER;
    __buddy_order_set_free(arena, order);
    __buddy_block_set_free(arena, order, 0);

    return 0;
}

u64 __buddy_alloc(struct buddy_arena *arena, u32 order)
{
    if (order >= BUDDY_NUM_ORDERS)
        return PMA_NULL;
   
    u32 alloc_order = order;
    long block_idx = -1;
    
    u32 max_order = BUDDY_NUM_ORDERS - 1;
    
    while (alloc_order <= max_order) {

        if (__buddy_is_order_free(arena, alloc_order)) {

            block_idx = __buddy_find_block_at_order(arena, alloc_order);
            if (block_idx >= 0)
                break;
        
            __buddy_update_order_bitmap(arena, alloc_order);
        }

        alloc_order++;
    }
    
    if (block_idx < 0)
        return PMA_NULL; 

    __buddy_block_clear(arena, alloc_order, block_idx);
    __buddy_update_order_bitmap(arena, alloc_order);
   
    while (alloc_order > order) {

        alloc_order--;
        
        u64 left = block_idx << 1;
        u64 right = left | 1;
        
        __buddy_block_clear(arena, alloc_order, left);
        
        __buddy_block_set_free(arena, alloc_order, right);
        __buddy_order_set_free(arena, alloc_order);
        
        block_idx = left;
    }
    
    u64 addr = buddy_block_idx_to_addr(arena, block_idx, order);
    return addr;
}

void __buddy_free(struct buddy_arena *arena, u64 addr, u32 order)
{
    if (addr == PMA_NULL || order >= BUDDY_NUM_ORDERS)
        return;
    
    u64 block_idx = buddy_addr_to_block_idx(arena, addr, order);
    
    u32 current_order = order;
    u32 max_order = BUDDY_NUM_ORDERS - 1;

    while (current_order < max_order) {
       
        u64 num_blocks = buddy_order_num_blocks(current_order);
        u64 buddy_idx = buddy_get_idx(block_idx);
        
        if (buddy_idx >= num_blocks || 
            !__buddy_is_block_free(arena, current_order, buddy_idx)) {
          
            break;
        }
        
        __buddy_block_clear(arena, current_order, block_idx);
        __buddy_block_clear(arena, current_order, buddy_idx);
        __buddy_update_order_bitmap(arena, current_order);
        
        current_order++;
        block_idx = buddy_get_parent_idx(block_idx);
    }
    
    __buddy_block_set_free(arena, current_order, block_idx);
    __buddy_order_set_free(arena, current_order);
}