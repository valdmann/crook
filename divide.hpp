// REPLACING DIVISIONS
//
// In the pursuit of speed I have replaced all divisions (well there's
// only the one atm) with multiplications by the corresponding
// reciprocal which is looked up from a table:
//
// > x / y = x * (1/y).
//
// This is faster.
//
// The table is computed by the constructor of class ReciprocalTable.
//
// The divisions are done by the free-standing function Divide:
// if x is a n-bit integer and y is a m-bit integer then
//
// > Divide(x, n, y, m) ~= x / y.
//
// The equation is precise if n and m are small enough.

#ifndef DIVIDE_HPP
#define DIVIDE_HPP

#include "config.hpp"

class ReciprocalTable
{
    U16 t[DIVISOR_LIMIT];
public:
    ReciprocalTable()
    {
        for (U32 n = 0; n < DIVISOR_LIMIT; ++n)
            t[n] = RECIPROCAL_LIMIT / (n + 2);
    }
    U32 operator[](U32 n)
    {
        assert(n < DIVISOR_LIMIT);
        return t[n];
    }
} reciprocals;

U32 Excess(U32 n, U32 m)
{
    return (n > m) ? n - m : 0;
}

U32 Divide(U32 x, U32 n, U32 y, U32 m)
{
    assert (x < (1 << n));
    assert (y < (1 << m));
    U32 dn = Excess(n, 32 - RECIPROCAL_BITS);
    U32 dm = Excess(m, DIVISOR_BITS);
    U32 dk = RECIPROCAL_BITS + dm - dn;
    return ((x >> dn) * reciprocals[y >> dm]) >> dk;
}

#endif
