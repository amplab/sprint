/*
 * Collection of basic tools and defines
 */


#ifndef _TOOLS_H_
#define _TOOLS_H_

#include <iostream>
#include <fstream>
#include <ctime>
#include <climits>
#include <cstdlib>

// Generates an error if __WORDSIZE is not defined
#ifndef __WORDSIZE
#error Missing definition of __WORDSIZE; Please define __WORDSIZE in Tools.h!
#endif

// Check word length on GNU C/C++:
// __WORDSIZE should be defined in <bits/wordsize.h>, which is #included from <limits.h>
#if __WORDSIZE == 64
#   define W 64
#else
#   define W 32
#endif

#define WW (W*2)
#define Wminusone (W-1)

#ifndef uchar
#define uchar unsigned char
#endif
#ifndef ulong
#define ulong unsigned long
#endif


class Tools
{
private:
    static time_t startTime;
public:
    static void StartTimer();
    static double GetTime();
    static uchar * GetRandomString(unsigned, unsigned, unsigned &);
    static void PrintBitSequence(ulong *, ulong);
    static uchar * GetFileContents(char *, ulong =0);
    static ulong ustrlen(uchar *);
    static char *uctoc(uchar *, bool = false);
    static uchar *ctouc(char *, bool = false);
    static void RemoveControlCharacters(uchar *);
    static ulong * bp2bitstream(uchar *);
    static unsigned FloorLog2(ulong);
    static unsigned CeilLog2(ulong);
    static unsigned bits (ulong);
    static unsigned* MakeTable();
    static unsigned FastFloorLog2(unsigned);

    static inline void SetField(ulong *A, unsigned len, ulong index, ulong x) 
    {
        ulong i = index * len / W, 
                 j = index * len - i * W;
        ulong mask = (j+len < W ? ~0lu << (j+len) : 0) 
                        | (W-j < W ? ~0lu >> (W-j) : 0);
        A[i] = (A[i] & mask) | x << j;
        if (j + len > W) 
        {
            mask = ((~0lu) << (len + j - W));  
            A[i+1] = (A[i+1] & mask)| x >> (W - j);
        }     
    }
    
    static inline ulong GetField(ulong *A, unsigned len, ulong index) 
    {
        ulong i = index * len / W, 
                       j = index * len - W * i, 
                       result;
        if (j + len <= W)
            result = (A[i] << (W - j - len)) >> (W - len);
        else 
        {
            result = A[i] >> j;
            result = result | (A[i+1] << (WW - j - len)) >> (W - len);
        }
        return result;
    }
       
    
    static inline ulong GetVariableField(ulong *A, unsigned len, ulong index) 
    {
       ulong i=index/W, j=index-W*i, result;
       if (j+len <= W)
       result = (A[i] << (W-j-len)) >> (W-len);
       else {
             result = A[i] >> j;
             result = result | (A[i+1] << (WW-j-len)) >> (W-len);
          }
       return result;
    }

    static inline void SetVariableField(ulong *A, unsigned len, ulong index, ulong x) {
        ulong i=index/W, 
                    j=index-i*W;
        ulong mask = (j+len < W ? ~0lu << (j+len) : 0) 
                        | (W-j < W ? ~0lu >> (W-j) : 0);
        A[i] = (A[i] & mask) | x << j;
        if (j+len>W) {
            mask = ((~0lu) << (len+j-W));  
            A[i+1] = (A[i+1] & mask)| x >> (W-j);
        } 
    }


};

#endif
