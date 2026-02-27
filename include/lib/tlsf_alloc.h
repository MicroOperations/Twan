#ifndef _TLSF_H_
#define _TLSF_H_

#include <kernel/kernel.h>
#include <subsys/sync/mcslock.h>

#define TLSF_FL_SHIFT (PAGE_SHIFT_2MB + 6)
#define TLSF_SL_SHIFT 5 
#define TLSF_FL_COUNT (TLSF_FL_SHIFT + 1)
#define TLSF_SL_COUNT (1 << TLSF_SL_SHIFT)

#define TLSF_BLOCK_MIN 16

#define TLSF_ALIGNMENT 8
STATIC_ASSERT(is_pow2(TLSF_ALIGNMENT));

#define TLSF_MAX_ALLOC (1ULL << TLSF_FL_SHIFT)

struct tlsf_block
{
    size_t size;
    struct tlsf_block *prev_phys;
    struct tlsf_block *prev_free;
    struct tlsf_block *next_free;
};

struct tlsf
{
    u32 fl_bitmap;
    u32 sl_bitmaps[TLSF_FL_COUNT];

    struct tlsf_block *blocks[TLSF_FL_COUNT][TLSF_SL_COUNT];

    u64 heap_start;
    u64 heap_end;
    size_t heap_size;
    size_t max;

    struct mcslock_isr lock;
};

struct aligned_block 
{
    void *original_ptr;
    size_t size;
};

STATIC_ASSERT(TLSF_FL_COUNT > 0 && TLSF_FL_COUNT <= 32, 
             "TLSF_FL_COUNT should be between 1 - 32");

STATIC_ASSERT(TLSF_SL_COUNT > 0 && TLSF_SL_COUNT <= 32, 
             "TLSF_SL_COUNT should be between 1 - 32");

#define TLSF_BLOCK_FREE_MASK (1ULL << 0)
#define TLSF_PREV_PHYS_FREE_MASK (1ULL << 1)
#define TLSF_SIZE_RESERVED_MASK \
    (TLSF_BLOCK_FREE_MASK | TLSF_PREV_PHYS_FREE_MASK)

#define tlsf_align_up(x) p2align_up((x), TLSF_ALIGNMENT)
#define tlsf_align_down(x) p2align_down((x), TLSF_ALIGNMENT)

#define tlsf_get_block_size(block) \
    ((block)->size & ~TLSF_SIZE_RESERVED_MASK)

#define tlsf_set_block_size(block, new_size)                      \
    ((block)->size = ((new_size & ~TLSF_SIZE_RESERVED_MASK) |     \
                      ((block)->size & TLSF_SIZE_RESERVED_MASK)))

#define tlsf_block_is_free(block) \
    (((block)->size & TLSF_BLOCK_FREE_MASK) != 0)

#define tlsf_prev_is_free(block) \
    (((block)->size & TLSF_PREV_PHYS_FREE_MASK) != 0)

#define tlsf_set_block_free(block) \
    ((block)->size |= TLSF_BLOCK_FREE_MASK)

#define tlsf_unset_block_free(block) \
    ((block)->size &= ~TLSF_BLOCK_FREE_MASK)

#define tlsf_set_prev_phys_free(block) \
    ((block)->size |= TLSF_PREV_PHYS_FREE_MASK)

#define tlsf_unset_prev_phys_free(block) \
    ((block)->size &= ~TLSF_PREV_PHYS_FREE_MASK)

#define tlsf_next_block(block) \
    ((struct tlsf_block *)((u8 *)&(block)[1] + tlsf_get_block_size((block))))

#define tlsf_block_to_ptr(block) (u8 *)&(block)[1]
#define tlsf_ptr_to_block(addr) \
    ((struct tlsf_block *)((u8 *)(addr) - sizeof(struct tlsf_block)))

#define tlsf_map_search(size, fli_ptr, sli_ptr) \
    __tlsf_map((size), (fli_ptr), (sli_ptr))

#define tlsf_map_insert(size, fli_ptr, sli_ptr) \
    __tlsf_map((size), (fli_ptr), (sli_ptr))

/* helpers */

void __tlsf_map(size_t size, u32 *fli_ptr, u32 *sli_ptr);

void __tlsf_remove_free_block(struct tlsf *tlsf, struct tlsf_block *block,
                              u32 fli, u32 sli);

void __tlsf_insert_free_block(struct tlsf *tlsf, struct tlsf_block *block,
                              u32 fli, u32 sli);

struct tlsf_block *__tlsf_search_suitable_block(struct tlsf *tlsf, 
                                                u32 *fli_ptr, u32 *sli_ptr);

struct tlsf_block *__tlsf_merge_prev(struct tlsf *tlsf, 
                                     struct tlsf_block *block);
struct tlsf_block *__tlsf_merge_next(struct tlsf *tlsf, 
                                     struct tlsf_block *block);

void __tlsf_mark_free(struct tlsf *tlsf, struct tlsf_block *block);
void __tlsf_unmark_free(struct tlsf *tlsf, struct tlsf_block *block);

bool __tlsf_try_carve(struct tlsf *tlsf, struct tlsf_block *block, size_t size);

/* core api's */

int __tlsf_init(struct tlsf *tlsf, u64 heap_start, size_t heap_size);

void tlsf_lock(struct tlsf *tlsf, struct mcsnode *node);
void tlsf_unlock(struct tlsf *tlsf, struct mcsnode *node);

void *__tlsf_alloc(struct tlsf *tlsf, size_t size);
void __tlsf_free(struct tlsf *tlsf, void *addr);
void *__tlsf_realloc(struct tlsf *tlsf, void *addr, size_t size);

void *__tlsf_alloc_p2aligned(struct tlsf *tlsf, size_t size, u32 alignment);
void __tlsf_free_p2aligned(struct tlsf *tlsf, void *addr);
void *__tlsf_realloc_p2aligned(struct tlsf *tlsf, void *addr, 
                               size_t size, u32 alignment);

#endif