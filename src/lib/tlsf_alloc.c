#include <lib/tlsf_alloc.h>

void __tlsf_map(size_t size, u32 *fli_ptr, u32 *sli_ptr)
{
    if (size < TLSF_SL_COUNT) {
        *fli_ptr = 0;
        *sli_ptr = (u32)size;
        return;
    } 

    *fli_ptr = fls32(size);
    *sli_ptr = (u32)((size >> (*fli_ptr - TLSF_SL_SHIFT)) &
                      (TLSF_SL_COUNT - 1));
                
    /* handle the case of needing to round up to the next bin */
    if (size & ((1ULL << (*fli_ptr - TLSF_SL_SHIFT)) - 1)) {
        (*sli_ptr)++;

        /* check if we should round up to the next fl bin */
        if (*sli_ptr >= TLSF_SL_COUNT) {
            (*fli_ptr)++;
            *sli_ptr = 0;
        }
    }

    /* hold down fli */
    if (*fli_ptr > TLSF_FL_SHIFT)
        *fli_ptr = TLSF_FL_SHIFT;
}

void __tlsf_remove_free_block(struct tlsf *tlsf, struct tlsf_block *block, 
                              u32 fli, u32 sli)
{
    struct tlsf_block *prev_free = block->prev_free;
    struct tlsf_block *next_free = block->next_free;

    /* remove the block from the double linked list for the bin type shi */

    if (prev_free)
        prev_free->next_free = next_free;

    if (next_free)
        next_free->prev_free = prev_free;

    /* check if the block is the first in the double linked list */
    if (tlsf->blocks[fli][sli] == block) {

        /* set next_free to the start */
        tlsf->blocks[fli][sli] = next_free;
        
        /* unset the bitmaps for the fl/sl bins if they have no blocks left */
        if (!next_free) {
            tlsf->sl_bitmaps[fli] &= ~(1U << sli);

            if (!tlsf->sl_bitmaps[fli]) 
                tlsf->fl_bitmap &= ~(1U << fli);
        }
    }
}

void __tlsf_insert_free_block(struct tlsf *tlsf, struct tlsf_block *block,
                              u32 fli, u32 sli)
{
    /* set the block to the start of the double linked list for the bin */
    struct tlsf_block *cur = tlsf->blocks[fli][sli];

    block->next_free = cur;
    block->prev_free = NULL;

    if (cur)
        cur->prev_free = block;

    tlsf->blocks[fli][sli] = block;

    /* set the bin as available */
    tlsf->fl_bitmap |= (1U << fli);
    tlsf->sl_bitmaps[fli] |= (1U << sli);
}

struct tlsf_block *__tlsf_search_suitable_block(struct tlsf *tlsf, 
                                                u32 *fli_ptr, u32 *sli_ptr)
{
    u32 fli = *fli_ptr;
    u32 sli = *sli_ptr;

    /* check for available + large enough bins in the sl bitmap */
    u32 available_sl_bins = tlsf->sl_bitmaps[fli] & (~0U << sli);

    /* no available sl bins? */
    if (!available_sl_bins) {

        /* fuck it we can check if theres any other useable fl bins */
        u32 available_fl_bins = tlsf->fl_bitmap & (~0U << (fli + 1));
        if (!available_fl_bins)
            return NULL;
      
        /* get the first available fl bin and get its available sl bin */
        fli = ffs32(available_fl_bins);
        available_sl_bins = tlsf->sl_bitmaps[fli];
    } 

    /* get the first available sl bin and return its corresponding block */
    sli = ffs32(available_sl_bins);
    *fli_ptr = fli;
    *sli_ptr = sli;

    return tlsf->blocks[fli][sli];
}

struct tlsf_block *__tlsf_merge_prev(struct tlsf *tlsf, 
                                    struct tlsf_block *block)
{
    if (!tlsf_prev_is_free(block))
        return block;

    struct tlsf_block *prev = block->prev_phys;
    size_t prev_size = tlsf_get_block_size(prev);

    u32 fli;
    u32 sli;

    /* remove the free block so we can well, fucking merge it */
    tlsf_map_insert(prev_size, &fli, &sli);
    __tlsf_remove_free_block(tlsf, prev, fli, sli);

    /* add the size of prevs block + its header and the current block */
    size_t new_size = prev_size + sizeof(struct tlsf_block) + 
                      tlsf_get_block_size(block);

    tlsf_set_block_size(prev, new_size);

    /* update the next physical block to point back to the merged block */
    struct tlsf_block *next = tlsf_next_block(block);
    if ((u64)next < tlsf->heap_end)
        next->prev_phys = prev;

    return prev;
}

struct tlsf_block *__tlsf_merge_next(struct tlsf *tlsf, 
                                     struct tlsf_block *block)
{
    /* same shit as __tlsf_merge_prev but the other way around */
    struct tlsf_block *next = tlsf_next_block(block);
    if ((u64)next < tlsf->heap_end && tlsf_block_is_free(next)) {   

        size_t next_size = tlsf_get_block_size(next);

        u32 fli;
        u32 sli;

        tlsf_map_insert(next_size, &fli, &sli);
        __tlsf_remove_free_block(tlsf, next, fli, sli);

        size_t new_size = tlsf_get_block_size(block) + next_size + 
                          sizeof(struct tlsf_block);

        tlsf_set_block_size(block, new_size);

        struct tlsf_block *next_next = tlsf_next_block(next);
        if ((u64)next_next < tlsf->heap_end)
            next_next->prev_phys = block;
            
    }

    return block;
}

void __tlsf_mark_free(struct tlsf *tlsf, struct tlsf_block *block)
{
    /* mark the block as free and mark the next block prev as free */
    tlsf_set_block_free(block);

    struct tlsf_block *next = tlsf_next_block(block);
    if ((u64)next < tlsf->heap_end)
        tlsf_set_prev_phys_free(next);
}

void __tlsf_unmark_free(struct tlsf *tlsf, struct tlsf_block *block)
{
    /* same shit as __tlsf_mark_free but other way around */
    tlsf_unset_block_free(block);

    struct tlsf_block *next = tlsf_next_block(block);
    if ((u64)next < tlsf->heap_end)
        tlsf_unset_prev_phys_free(next);
}

bool __tlsf_try_carve(struct tlsf *tlsf, struct tlsf_block *block, size_t size)
{
    size_t merged_size = tlsf_get_block_size(block);
    if (merged_size < size + sizeof(struct tlsf_block) + TLSF_BLOCK_MIN)
        return false;

    struct tlsf_block *cutout = (void *)(tlsf_block_to_ptr(block) + size); 
    size_t cutout_size = merged_size - size - sizeof(struct tlsf_block);

    tlsf_set_block_size(cutout, cutout_size);
    tlsf_set_block_free(cutout);
    tlsf_unset_prev_phys_free(cutout);
    cutout->prev_phys = block;

    tlsf_set_block_size(block, size);

    struct tlsf_block *cutout_next = tlsf_next_block(cutout);
    if ((u64)cutout_next < tlsf->heap_end)
        cutout_next->prev_phys = cutout;

    u32 fli;
    u32 sli;
            
    tlsf_map_insert(cutout_size, &fli, &sli);
    __tlsf_insert_free_block(tlsf, cutout, fli, sli);

    return true;
}

int __tlsf_init(struct tlsf *tlsf, u64 heap_start, size_t heap_size)
{
    mcslock_isr_init(&tlsf->lock);

    if (!heap_start || heap_size < (TLSF_BLOCK_MIN + sizeof(struct tlsf_block)))
        return -EINVAL;

    u64 heap_start_aligned = tlsf_align_up(heap_start);
    if (heap_start_aligned - heap_start >= heap_size)
        return -EINVAL;

    u64 heap_size_adjusted = heap_size - (heap_start_aligned - heap_start);

    tlsf->heap_start = heap_start_aligned;
    tlsf->heap_end = heap_start_aligned + heap_size_adjusted;
    tlsf->heap_size = heap_size_adjusted;

    /* start with a big fucking block and setup its header */
    struct tlsf_block *init_block = (void *)heap_start_aligned;
    size_t block_size = heap_size_adjusted - sizeof(struct tlsf_block);

    tlsf_set_block_size(init_block, block_size);
    tlsf_set_block_free(init_block);

    init_block->prev_phys = NULL;
    init_block->prev_free = NULL;
    init_block->next_free = NULL;

    u32 fli;
    u32 sli;

    tlsf_map_insert(block_size, &fli, &sli);
    __tlsf_insert_free_block(tlsf, init_block, fli, sli);

    return 0;
}

void tlsf_lock(struct tlsf *tlsf, struct mcsnode *node)
{
    mcs_lock_isr_save(&tlsf->lock, node);
}

void tlsf_unlock(struct tlsf *tlsf, struct mcsnode *node)
{
    mcs_unlock_isr_restore(&tlsf->lock, node);
}

void *__tlsf_alloc(struct tlsf *tlsf, size_t size)
{
    if (!size || size > TLSF_MAX_ALLOC)
        return NULL;

    /* need to align this bitch up */
    size = tlsf_align_up(size);
    if (size < TLSF_BLOCK_MIN)
        size = TLSF_BLOCK_MIN;

    u32 fli;
    u32 sli;

    /* find the first available bin we can use */
    tlsf_map_search(size, &fli, &sli);

    struct tlsf_block *block = 
        __tlsf_search_suitable_block(tlsf, &fli, &sli);

    if (!block)
        return NULL;

    size_t block_size = tlsf_get_block_size(block);

    if (block_size < size)
        return NULL;

    /* unfree the block mate */
    __tlsf_remove_free_block(tlsf, block, fli, sli);

    /* check if we can cut the block, as in if extra space within it can 
       be used to create a new block with the left over space */
    if (block_size >= size + sizeof(struct tlsf_block) + TLSF_BLOCK_MIN) {

        struct tlsf_block *cutout = 
            (struct tlsf_block *)(tlsf_block_to_ptr(block) + size);
            
        size_t cutout_size = block_size - size - sizeof(struct tlsf_block);

        tlsf_set_block_size(cutout, cutout_size);
        tlsf_set_block_free(cutout);
        cutout->prev_phys = block;

        tlsf_set_block_size(block, size);

        /* set the phys block after the cutout to point back to it, 
           also we dont have to set cutout_next's prev_phys_free
           since it'll already be set from pointing to the block
           were allocating rn */
        struct tlsf_block *cutout_next = tlsf_next_block(cutout);
        if ((u64)cutout_next < tlsf->heap_end)
            cutout_next->prev_phys = cutout;

        /* we can insert the cutout to the block freelist */
        tlsf_map_insert(cutout_size, &fli, &sli);
        __tlsf_insert_free_block(tlsf, cutout, fli, sli);
    }

    /* mark the hoe as not free and return the start of the allocated memory */
    __tlsf_unmark_free(tlsf, block);
    return tlsf_block_to_ptr(block);
}

void __tlsf_free(struct tlsf *tlsf, void *addr)
{
    /* validate the shit theyre passing to us, shit validation but it'll do */
    u64 addru64 = (u64)addr;
    if (addru64 < tlsf->heap_start || addru64 >= tlsf->heap_end)
        return;

    /* validate shit again this time on the header */
    struct tlsf_block *block = tlsf_ptr_to_block(addr);
    u64 blocku64 = (u64)block;
    if (blocku64 < tlsf->heap_start || blocku64 >= tlsf->heap_end)
        return;

    /* mark the hoe as free */
    __tlsf_mark_free(tlsf, block);

    /* merge the previous and next blocks, they'll check if they're free, 
       also they'll remove em from the freelist so we can just insert the
       merged block without any complications, also we need to merge 
       next then prev since our merge routines merges into the previous 
       block (prev <- block <- next) */
    block = __tlsf_merge_next(tlsf, block);
    block = __tlsf_merge_prev(tlsf, block);

    /* put the block into the freelist, usual shit */
    size_t size = tlsf_get_block_size(block);

    u32 fli;
    u32 sli;
    tlsf_map_insert(size, &fli, &sli);
    __tlsf_insert_free_block(tlsf, block, fli, sli);
}

void *__tlsf_realloc(struct tlsf *tlsf, void *addr, size_t size)
{
    /* check if we can just alloc or free it */
    if (!addr)
        return __tlsf_alloc(tlsf, size);

    if (!size) {
        __tlsf_free(tlsf, addr);
        return NULL;
    }

    /* sanity check the address just in case */
    u64 addru64 = (u64)addr;
    if (addru64 < tlsf->heap_start || addru64 >= tlsf->heap_end)
        return NULL;

    /* validate the amount they're allocating too for our coalescing */
    if (size > TLSF_MAX_ALLOC)
        return NULL;

    struct tlsf_block *block = tlsf_ptr_to_block(addr);
    size_t block_size = tlsf_get_block_size(block);

    /* check if we can reuse the block */
    size = tlsf_align_up(size);
    if (size < TLSF_BLOCK_MIN)
        size = TLSF_BLOCK_MIN;

    if (block_size == size)
        return addr;

    /* if we shrink, try carve out some mem */
    if (block_size > size) {
        __tlsf_try_carve(tlsf, block, size);
        return addr;
    }

    /* we know now that a larger size is being allocated so ye,
       we can see if its possible to coalesce the next block */
    struct tlsf_block *next = tlsf_next_block(block);
    if ((u64)next < tlsf->heap_end && tlsf_block_is_free(next)) {

        size_t next_block_size = tlsf_get_block_size(next);
        size_t merged_size = next_block_size + block_size;

        if (merged_size >= size) {
            
            __tlsf_merge_next(tlsf, block);
            __tlsf_try_carve(tlsf, block, size);
            return tlsf_block_to_ptr(block);
        }
    }

    /* as a last resort our bitch ass will allocate and copy */
    void *new_addr = __tlsf_alloc(tlsf, size);
    if (!new_addr)
        return NULL;

    memcpy(new_addr, addr, block_size);
    __tlsf_free(tlsf, addr);
    return new_addr;
}

void *__tlsf_alloc_p2aligned(struct tlsf *tlsf, size_t size, u32 alignment)
{
     /* gotta make sure the alignment is a p2 and ptr size */
    if (!is_pow2(alignment))
        return NULL;  

    if (!size || size > TLSF_MAX_ALLOC)
        return NULL;

    /* basically what we can do is we can allocate a block and put our own
       header into it to store the original pointer then align it */
    size_t total_size = size + alignment + sizeof(struct aligned_block);
    if (total_size > TLSF_MAX_ALLOC)
        return NULL;

    u8 *ptr = __tlsf_alloc(tlsf, total_size);
    if (!ptr)
        return NULL;

    u8 *offset_ptr = ptr + sizeof(struct aligned_block);
    void *aligned_ptr = (void *)p2align_up((u64)offset_ptr, (u64)alignment);
    
    struct aligned_block *header = 
        (void *)((u8 *)aligned_ptr - sizeof(struct aligned_block));

    /* carve out the mem in the padding otherwise we will blow up due to
       internal fragmentation */
    u64 padding = (u64)header - (u64)ptr;
    if (padding >= (sizeof(struct tlsf_block) + TLSF_BLOCK_MIN)) {

        struct tlsf_block *orig_block = tlsf_ptr_to_block(ptr);
       
        struct tlsf_block *new_block = 
            (void *)((u8 *)header - sizeof(struct tlsf_block));

        size_t orig_total_size = tlsf_get_block_size(orig_block);
        size_t padding_size = (u64)new_block - (u64)ptr;
        size_t new_block_size = orig_total_size - padding_size - 
                                sizeof(struct tlsf_block);

        tlsf_set_block_size(new_block, new_block_size);
        tlsf_unset_block_free(new_block);
        tlsf_unset_prev_phys_free(new_block);
        new_block->prev_phys = orig_block;
        new_block->prev_free = NULL;
        new_block->next_free = NULL;
       
        struct tlsf_block *next = tlsf_next_block(new_block);
        if ((u64)next < tlsf->heap_end)
            next->prev_phys = new_block;

        tlsf_set_block_size(orig_block, padding_size);
        __tlsf_free(tlsf, ptr);
        ptr = tlsf_block_to_ptr(new_block);
    }
    
    header->original_ptr = ptr;
    header->size = size;

    return aligned_ptr;
}

void __tlsf_free_p2aligned(struct tlsf *tlsf, void *addr)
{
     struct aligned_block *header = 
        (void *)((u8 *)addr - sizeof(struct aligned_block));

    u64 headeru64 = (u64)header;

    if (headeru64 < tlsf->heap_start || 
        headeru64 >= tlsf->heap_end || 
        headeru64 >= (u64)addr) {

        return;
    }

    __tlsf_free(tlsf, header->original_ptr);
}

void *__tlsf_realloc_p2aligned(struct tlsf *tlsf, void *addr, 
                               size_t size, u32 alignment)
{
    if (!addr)
        return __tlsf_alloc_p2aligned(tlsf, size, alignment);

    if (!size) {
        __tlsf_free_p2aligned(tlsf, addr);
        return NULL;
    }

    struct aligned_block *header = 
        (void *)((u8 *)addr - sizeof(struct aligned_block));

    /* gotta also sanity check the header */
    u64 headeru64 = (u64)header;
    
    if (headeru64 < tlsf->heap_start || 
        headeru64 >= tlsf->heap_end || 
        headeru64 >= (u64)addr) {

        return NULL;
    }

    /* TODO: optimise by carving space on shrinks, merging on growth */

    void *new_addr = __tlsf_alloc_p2aligned(tlsf, size, alignment);
    if (!new_addr)
        return NULL;

    size_t old_size = header->size;
    size_t copy_size = size > old_size ? old_size : size;

    memcpy(new_addr, addr, copy_size);
    __tlsf_free_p2aligned(tlsf, addr);
    return new_addr;
}