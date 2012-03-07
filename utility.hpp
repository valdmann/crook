// SOME UTILITY FUNCTIONS
//
// Fit changes the precision of probability values: if x is a n-bit
// probability then Fit(x, n, m) is the closest m-bit probability.
//
// Fit0 is similar but it ensures the result does not become zero.

#ifndef UTILITY_HPP
#define UTILITY_HPP

#include "config.hpp"

U32 Fit(U32 x, U32 n, U32 m)
{
    assert(x < (1 << n));
    return (n > m) ? x >> (n - m) : x << (m - n);
}

U32 Fit0(U32 x, U32 n, U32 m)
{
    assert(0 < x && x < (1 << n));
    return Fit(x, n, m) + 1 - (x >> (n - 1));
}

#endif
