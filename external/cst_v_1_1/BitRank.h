#include "ReplacePattern.h"
#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

class BitRank {
private:
    // Check word length
#if W == 32
    static const unsigned wordShift = 5;
    static const unsigned superFactor = 8; // 256 bit blocks
#else
    static const unsigned wordShift = 6;
    static const unsigned superFactor = 4; // 256 bit blocks
#endif
    
    ulong *data; //here is the bit-array
    bool owner;
    ulong n,integers;
    unsigned b,s; 
    ulong *Rs; //superblock array
    uchar *Rb; //block array
    ReplacePattern *rp;
    ulong BuildRankSub(ulong,  ulong); //internal use of BuildRank
    void BuildRank(); //crea indice para rank
public:
    BitRank(ulong *, ulong, bool, ReplacePattern * = 0);
    ~BitRank(); //destructor    
    ulong rank(ulong i); //Rank from 0 to n-1
    ulong select(ulong x); // gives the position of the x:th 1.
    ulong select0(ulong x); // gives the position of the x:th 0.

    bool IsBitSet(ulong i);
    ulong NumberOfBits();
};

#endif
