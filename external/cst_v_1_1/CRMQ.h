/***************************************************************************
 *   Copyright (C) 2006 by Niko Välimäki                                   *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

// CRMQ uses three different sample rates:
//  - sampleRate is the same as in Sadakane's paper (log^3 n).
//  - subSampleRate is the same as in Sadakane's paper but the bigger size (E.g. 10 * log n) is compensated with sequential calls to look-up tables.
//  - blockSampleRate is the sample rate used for Four-Russians technique (log n / 2).
// The sampleRate value affects the size of M[i, k] and M_j[i, k] tables. The subSampleRate affects the size of M_j[i, k] table.
// Value of blockSampleRate affects only the look-up tables.
 
#ifndef _CRMQ_H_
#define _CRMQ_H_

#include <iostream>
#include "SubblockRMQ.h"
// #include "TRMQ.h"
#include "Tools.h"
#include "BitRank.h"


//#define DEBUG_CRMQ

class CRMQ
{
private:
    ulong *M, *subM;     // Arrays M[i, k] and M_j[i, k]
    ulong *P;            // Parentheses sequence
    unsigned widthM;
    unsigned widthSubM;
    ulong n;
    unsigned sampleRate;
    unsigned subSampleRate;
    unsigned blockSampleRate;
    SubblockRMQ *srmq;
    //TRMQ *trmq;
    BitRank *br;
    
    ulong lookupSub(ulong, ulong, ulong) const;
    ulong lookupSubblock(ulong, ulong, ulong, ulong) const;
    void SetSubM(ulong j, ulong i, ulong k, ulong value)    // Set value of M_j[i, k]
    {
        Tools::SetField(subM, widthSubM, j * (sampleRate / subSampleRate) * widthSubM + i * widthSubM + k, value);
    }
    ulong GetSubM(ulong j, ulong i, ulong k) const   // Get value of M_j[i, k]
    {
        if (i * subSampleRate + j * sampleRate >= n)
            return 0;
        return Tools::GetField(subM, widthSubM, j * (sampleRate / subSampleRate) * widthSubM + i * widthSubM + k);
    }

    ulong GetSubblockIndex(ulong j, ulong i) const // Get the index of a minimum value inside a subblock i of block j
    {
        ulong subblockStart = i * subSampleRate / blockSampleRate + j * sampleRate / blockSampleRate;
        if (subblockStart * blockSampleRate >= n)
            return n - 1;
        ulong minIndex = srmq->lookup(Tools::GetField(P, blockSampleRate, subblockStart), 0, blockSampleRate - 1) + subblockStart * blockSampleRate;
        if (minIndex >= n)
            minIndex = n - 1;
        ulong minValue = GetValue(minIndex);
//         printf("j = %d, i = %d, subblockStart = %d, minIndex = %d (%d)\n", j, i, subblockStart, minIndex, minValue);
        for (ulong k = 1; k < (subSampleRate / blockSampleRate) && (subblockStart + k) * blockSampleRate < n; k ++)
        {
//             printf("srmq->lookup(%d, %d, %d)\n", Tools::GetField(P, blockSampleRate, subblockStart + k), 0, blockSampleRate - 1);
            ulong index = srmq->lookup(Tools::GetField(P, blockSampleRate, subblockStart + k), 0, blockSampleRate - 1) + (subblockStart + k) * blockSampleRate;
//             printf("subblockStart = %d (%d), index = %d (%d)\n", subblockStart + k, (subblockStart + k) * blockSampleRate, index, GetValue(index));
            if (index >= n)
                index = n - 1;
            if (minValue > GetValue(index))
            {
                minValue = GetValue(index);
                minIndex = index;
            }
        }
//         printf("Return = %d, value = %d\n", minIndex, minValue);
        return minIndex;
    }
        
    ulong GetValue(ulong i) const   // Get value of P'[i]
    {
        return 2 * br->rank(i) - i;
    }

public:
    CRMQ(BitRank *, ulong *, ulong, unsigned, unsigned, unsigned);
    ~CRMQ();
    ulong lookup(ulong, ulong) const;
};

#endif
