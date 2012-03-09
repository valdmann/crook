// THE ARITHMETIC DECODER
//
// See also: the encoder in "encoder.hpp".

#ifndef RC_DECODER_HPP
#define RC_DECODER_HPP

#include "config.hpp"

class Decoder
{
    FILE * codeFile;
    U32 range;
    U32 cml; // code minus low
public:
    Decoder(FILE * codeFile)
        : codeFile(codeFile),
          range(0xFFFFFFFF),
          cml(0) {}

    void FillBuffer()
    {
        for (int i = 0; i < 5; ++i)
            cml = (cml << 8) + getc(codeFile);
    }

    bool Decode(U32 p1)
    {
        assert(0 < p1 && p1 < ARI_P_SCALE);
        U32 mid = range / ARI_P_SCALE * p1;
        if (cml < mid)
        {
            range = mid;
            return 1;
        }
        else
        {
            cml -= mid, range -= mid;
            return 0;
        }
    }

    void Normalize()
    {
        while (range <= 0xFFFFFF)
        {
            cml = (cml << 8) + getc(codeFile);
            range <<= 8;
        }
    }
};
#endif
