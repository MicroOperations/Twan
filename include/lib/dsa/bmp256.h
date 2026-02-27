#ifndef _BMP256_H_
#define _BMP256_H_

#include <std.h>

struct bmp256 
{
    u64 bmp[4];
};

inline void bmp256_and(struct bmp256 *bmp_dest, struct bmp256 *bmp_src)
{
    bmp_dest->bmp[0] &= bmp_src->bmp[0];
    bmp_dest->bmp[1] &= bmp_src->bmp[1];
    bmp_dest->bmp[2] &= bmp_src->bmp[2];
    bmp_dest->bmp[3] &= bmp_src->bmp[3];
}

inline void bmp256_or(struct bmp256 *bmp_dest, struct bmp256 *bmp_src)
{
    bmp_dest->bmp[0] |= bmp_src->bmp[0];
    bmp_dest->bmp[1] |= bmp_src->bmp[1];
    bmp_dest->bmp[2] |= bmp_src->bmp[2];
    bmp_dest->bmp[3] |= bmp_src->bmp[3];
}

inline bool bmp256_test(struct bmp256 *bmp, u8 bit)
{
    u32 index = bit / 64;
    u8 off = bit % 64;

    return ((bmp->bmp[index] >> off) & 1) != 0;
}

inline void bmp256_set(struct bmp256 *bmp, u8 bit)
{
    u32 index = bit / 64;
    u8 off = bit % 64;

    bmp->bmp[index] |= (1ULL << off);
}

inline void bmp256_unset(struct bmp256 *bmp, u8 bit)
{
    u32 index = bit / 64;
    u8 off = bit % 64;

    bmp->bmp[index] &= ~(1ULL << off);
}

inline int bmp256_ffs(struct bmp256 *bmp)
{
    int base = ffs64(bmp->bmp[0]);
    if (base != -1)
        return base;

    base = ffs64(bmp->bmp[1]);
    if (base != -1)
        return base + 64;

    base = ffs64(bmp->bmp[2]);
    if (base != -1)
        return base + 128;

    base = ffs64(bmp->bmp[3]);
    if (base != -1)
        return base + 192;

    return base;
}

inline int bmp256_fls(struct bmp256 *bmp)
{
    int base = fls64(bmp->bmp[3]);
    if (base != -1)
        return base + 192;

    base = fls64(bmp->bmp[2]);
    if (base != -1)
        return base + 128;

    base = fls64(bmp->bmp[1]);
    if (base != -1)
        return base + 64;

    return fls64(bmp->bmp[0]);
}

inline void bmp256_set_all(struct bmp256 *bmp)
{
    memset(&bmp->bmp, 0xff, sizeof(bmp->bmp));
}

inline void bmp256_unset_all(struct bmp256 *bmp)
{
    memset(&bmp->bmp, 0, sizeof(bmp->bmp));
}

inline void bmp256_unset_all_above(struct bmp256 *bmp, u8 bit)
{
    u32 index = bit / 64;
    u32 off = bit % 64;
 
    bmp->bmp[index] &= (off == 63) ? ~0ULL : ((1ULL << (off + 1)) - 1);
    
    if (index < 3)
        bmp->bmp[3] = 0;

    if (index < 2)
        bmp->bmp[2] = 0;

    if (index < 1)
        bmp->bmp[1] = 0;
}

#endif