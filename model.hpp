// THE MODEL
//
// The model is arranged as a binary tree where each node corresponds
// a to a string of bits, the context. Every node can have pointers up
// to two child nodes which represent contexts that are extended by
// either 0 or 1 on the right.
//
// All contexts are byte-aligned so context strings always start on
// byte boundaries.  All nodes also have suffix pointers which link to
// nodes whose context strings are shortened by one byte on the left
// (but otherwise identical).
//
// There's always one active node in the model (pointed to by the
// field 'Model::act').  This node corresponds to the longest matching
// context string stored in the model and is used for making
// predictions (no other nodes are consulted).  The bitwise order of
// the active node (i.e. the length of it's context string in bits) is
// stored in the field 'Model::order'.
//
// The model is initialized as a order-0 (bytewise) model meaning
// there are 255 nodes in the tree at first plus a special root node.
//
// In each node statistics are kept on the bit values that have
// occured in this context.  These statistics are stored as a
// probability and as a total count.  Now, one could also store the
// statistics as a count of 1's and 0's:
//
// > p1 = n1/(n0+n1) <===> n1 = p1*(n0+n1).
//
// This would imply however that the resolution of p1 is small
// whenever the counts are small which doesn't mesh well with
// information inheritance.
//
// Probabilities are stored in each node in the high-order bits of the
// field 'Node::ctr' and the total count in the low-order bits.
//
// To reduce memory usage on 64-bit systems all pointers are stored as
// 32-bit offsets into the nodes pool.

#ifndef MODEL_HPP
#define MODEL_HPP

#include "config.hpp"

#include "utility.hpp"

struct Node
{
    U32 ext0;
    U32 ext1;
    U32 sfx;
    U32 ctr;

    Node() {}

    Node(U32 ext0, U32 ext1, U32 sfx)
        : ext0(ext0),
          ext1(ext1),
          sfx(sfx),
          ctr((PPM_P_START << PPM_C_BITS) + PPM_C_START) {}

    Node(U32 sfx, const Node * sfxp)
        : ext0(0),
          ext1(0),
          sfx(sfx),
          ctr((sfxp->ctr & PPM_P_MASK) + PPM_C_INH) {}

    U32 Predict()
    {
        return ctr >> PPM_C_BITS;
    }

    template <bool bit> void Update()
    {
        U32 cnt = ctr  & PPM_C_MASK;
        U32 p1  = ctr >> PPM_C_BITS;

        if (cnt < PPM_C_LIMIT - PPM_C_INC)
            cnt += PPM_C_INC;
        else
            cnt = PPM_C_LIMIT-1;

        // alternative:
        // cnt += PPM_C_INC;
        // cnt = -(cnt >> PPM_C_BITS) | cnt;
        // cnt &= PPM_C_MASK;

        assert(cnt < PPM_C_LIMIT);

        if (bit)
            p1 += PPM_C_SCALE * Divide(PPM_P_SCALE - p1, PPM_P_BITS, cnt, PPM_C_BITS);
        else
            p1 -= PPM_C_SCALE * Divide(              p1, PPM_P_BITS, cnt, PPM_C_BITS);

        assert(0 < p1 && p1 < PPM_P_SCALE);

        ctr = (p1 << PPM_C_BITS) + cnt;
    }

    template <bool bit> U32 & Ext()
    {
        return bit ? ext1 : ext0;
    }
};

extern int memoryLimit;
extern int orderLimit;

class PPM
{
    Node * nodes;
    Node * top;
    Node * end;

    Node * act;
    int order;

    const int nodesLimit;
    const int orderLimitBits;
public:
    PPM()
        : nodesLimit(memoryLimit * (1 << 20) / sizeof(Node)),
          orderLimitBits(8 * orderLimit + 7)
    {
        nodes = new Node[nodesLimit];
        end = nodes + nodesLimit;
        top = nodes;
        act = nodes + 1;
        order = 0;

        *top++ = Node(1, 1, 0);                 // 1   root node
        for (int dst = 2; dst != 256; dst += 2)
            *top++ = Node(dst, dst+1, 0);       // 127 internal nodes
        for (int i = 0; i != 128; ++i)
            *top++ = Node(0, 0, 0);             // 128 leaf nodes
    }

    U32 Predict()
    {
        return Fit0(act->Predict(), PPM_P_BITS, ARI_P_BITS);
    }

    template <bool bit> void Update()
    {
        act->Update<bit>();

        Node * lst = act;
        while (act->Ext<bit>() == 0)
        {
            lst = act;
            act = nodes + act->sfx;
            order -= 8;
            act->Update<bit>();
        }

        if (act != lst && order+9 <= orderLimitBits && top < end)
        {
            Node * ext = nodes + act->Ext<bit>();
            lst->Ext<bit>() = top - nodes;
            *top = Node(ext - nodes, ext);
            act = top++;
            order += 9;
        }
        else
        {
            Node * ext = nodes + act->Ext<bit>();
            act = ext;
            order++;
        }
    }

    U32 GetUsedMemory()
    {
        return ((top - nodes) * sizeof(Node)) >> 20;
    }
};

#endif
