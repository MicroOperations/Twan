#ifndef _BMP512_H_
#define _BMP512_H_

#include <lib/dsa/bmp256.h>

struct bmp512 
{
    struct bmp256 high256;
    struct bmp256 low256;
};

inline void bmp512_and(struct bmp512 *bmp_dest, struct bmp512 *bmp_src)
{
    bmp256_and(&bmp_dest->low256, &bmp_src->low256);
    bmp256_and(&bmp_dest->high256, &bmp_src->high256);
}

inline void bmp512_or(struct bmp512 *bmp_dest, struct bmp512 *bmp_src)
{
    bmp256_or(&bmp_dest->low256, &bmp_src->low256);
    bmp256_or(&bmp_dest->high256, &bmp_src->high256);
}

inline void bmp512_set(struct bmp512 *bmp, int bit)
{
    if (bit >= 256)
        bmp256_set(&bmp->high256, bit - 256);
    else
        bmp256_set(&bmp->low256, bit);
}

inline void bmp512_unset(struct bmp512 *bmp, int bit)
{
    if (bit >= 256)
        bmp256_unset(&bmp->high256, bit - 256);
    else
        bmp256_unset(&bmp->low256, bit);
}


inline int bmp512_ffs(struct bmp512 *bmp)
{
    int bit = bmp256_ffs(&bmp->low256);
    if (bit != -1)
        return bit;

    bit = bmp256_ffs(&bmp->high256) ;
    if (bit == -1)
        return bit;

    return bit + 256;
}

inline int bmp512_fls(struct bmp512 *bmp)
{
    int bit = bmp256_fls(&bmp->high256);
    if (bit != -1)
        return bit + 256;

    bit = bmp256_fls(&bmp->low256) ;
    if (bit == -1)
        return bit;

    return bit;
}

inline void bmp512_set_all(struct bmp512 *bmp)
{
    bmp256_set_all(&bmp->high256);
    bmp256_set_all(&bmp->low256);
}

inline void bmp512_unset_all(struct bmp512 *bmp)
{
    bmp256_unset_all(&bmp->high256);
    bmp256_unset_all(&bmp->low256);
}

inline void bmp512_set2(struct bmp512 *bmp, int first_bit)
{
    bmp512_set(bmp, first_bit);
    bmp512_set(bmp, first_bit + 1);
}

inline void bmp512_unset2(struct bmp512 *bmp, int first_bit)
{
    bmp512_unset(bmp, first_bit);
    bmp512_unset(bmp, first_bit + 1);
}

inline int bmp512_ffs2(struct bmp512 *bmp)
{
    struct bmp512 bmp_tmp = *bmp; 
    
    int bit;
    while ((bit = bmp512_ffs(&bmp_tmp)) != -1) {

        bmp512_unset(&bmp_tmp, bit);
        
        int next = bmp512_ffs(&bmp_tmp);
        if (next == -1)
            break;

        if (next == (bit + 1))
            return bit;

        bmp512_unset(&bmp_tmp, next);
    }

    return -1;
}

inline int bmp512_ffs_unset(struct bmp512 *bmp)
{
    int bit = bmp512_ffs(bmp);
    if (bit != -1)
        bmp512_unset(bmp, bit);

    return bit;
}

inline int bmp512_ffs2_unset2(struct bmp512 *bmp)
{
    int bit = bmp512_ffs2(bmp);
    if (bit != -1)
        bmp512_unset2(bmp, bit);

    return bit;
}

#endif