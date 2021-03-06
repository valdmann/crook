// THE ARITHMETIC ENCODER
//
// The arithmetic coder here is pretty typical; carries are handled
// with overflow being detected using a 64-bit low register and
// handled not immediately but later on in the renormalization loop.
//
// The first outputted byte is always zero and is ignored by the
// decoder.  This elides a branch in the renormalization loop.  I
// haven't actually tested the speed impact of this optimization.
//
// See also: the decoder in "decoder.hpp".

#ifndef RC_ENCODER_HPP
#define RC_ENCODER_HPP

#include "config.hpp"

class Encoder
{
    FILE * codeFile;
    U64 low;
    U32 range;
    U32 fluxLen;
    U8  fluxFst;
public:
    Encoder(FILE * codeFile)
        : codeFile(codeFile),
          low(0),
          range(0xFFFFFFFF),
          fluxLen(1),
          fluxFst(0) {}

    template <bool bit> void Encode(U32 p1)
    {
        assert(0 < p1 && p1 < ARI_P_SCALE);
        U32 mid = range / ARI_P_SCALE * p1;
        if (bit)
            range = mid;
        else
            low += mid, range -= mid;
    }

    void Normalize()
    {
        while (range <= 0xFFFFFF)
        {
            U32 lo32 = low, hi32 = low >> 32;
            if (lo32 < 0xFF000000 || hi32 != 0)
            {
                putc(fluxFst + hi32, codeFile);
                while (--fluxLen)
                    putc(0xFF + hi32, codeFile);
                fluxFst = lo32 >> 24;
            }
            ++fluxLen;
            low = (lo32 << 8);
            range <<= 8;
        }
    }

    void FlushBuffer()
    {
        U32 lo32 = low, hi32 = low >> 32;
        putc(fluxFst + hi32, codeFile);
        while (--fluxLen)
            putc(0xFF + hi32, codeFile);
        putc(lo32 >> 24, codeFile);
        putc(lo32 >> 16, codeFile);
        putc(lo32 >>  8, codeFile);
        putc(lo32 >>  0, codeFile);
    }
};
#endif
