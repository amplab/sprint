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

#include "CRMQ.h"

// CRMQ uses three different sample rates:
//  - sampleRate is the same as in Sadakane's paper (log^3 n).
//  - subSampleRate is the same as in Sadakane's paper but the bigger size (E.g. 10 * log n) is compensated with sequential calls to look-up tables.
//  - blockSampleRate is the sample rate used for Four-Russians technique (log n / 2).
// The sampleRate value affects the size of M[i, k] and M_j[i, k] tables. The subSampleRate affects the size of M_j[i, k] table.
// Value of blockSampleRate affects only the look-up tables.

CRMQ::CRMQ(BitRank *br, ulong *P, ulong n, unsigned sampleRate, unsigned subSampleRate, unsigned blockSampleRate)
{
    this->n = n;
    this->P = P;
    srmq = new SubblockRMQ(blockSampleRate);

    this->br = br;
    this->sampleRate = sampleRate;
    this->subSampleRate = subSampleRate;
    this->blockSampleRate = blockSampleRate;

    #ifdef DEBUG_CRMQ
//         trmq = new TRMQ(br, n);
        printf("n = %lu, sampleRate = %d, subSampleRate = %d, blockSampleRate = %d\n", n, sampleRate, subSampleRate, blockSampleRate);
    #endif
    
    // Array M[i, k]
    // Size of the array is  (n / sampleRate) * \ceil(\log(n / sampleRate)) * W  bits
    widthM = Tools::CeilLog2(n / sampleRate + 1);
    M = new ulong[(n / sampleRate + 1) * widthM];  
    #ifdef DEBUG_CRMQ
        printf("Allocated %lu bytes for %lu x %d array M.\n", (n / sampleRate + 1) * widthM, n / sampleRate + 1, widthM);
    #endif
    
    // Array M_j[i, k]
    // Size of the array is  
    // 
    // (n / sampleRate) * (sampleRate / subSampleRate) * \ceil(\log(sampleRate / subSampleRate)) * \ceil(\log(sampleRate / subSampleRate))  bits
    //
    widthSubM = Tools::CeilLog2(sampleRate / subSampleRate + 1);
    subM = new ulong[((n / sampleRate + 1) * (sampleRate / subSampleRate) * widthSubM * widthSubM) / W + 1];
    #ifdef DEBUG_CRMQ
        printf("Allocated %lu bits in %lu bytes for %lu x %d x %d array M_j. (%d bits per cell)\n", (n / sampleRate + 1) * (sampleRate / subSampleRate) * widthSubM * widthSubM, ((n / sampleRate + 1) * (sampleRate / subSampleRate) * widthSubM * widthSubM) / W + 1, n / sampleRate + 1, sampleRate / subSampleRate, widthSubM, widthSubM);
    #endif
    
    // Init dynamic programming for subblocks
    for (ulong j = 0; j < n/sampleRate + 1; j++)
        for (ulong i = 0; i < sampleRate/subSampleRate; i++)
            if (j * sampleRate + i * subSampleRate < n)
            {
                #ifdef DEBUG_CRMQ
                    printf("Setting M_%lu[%lu, %d] = %lu\n", j, i, 0, i);
                #endif
                SetSubM(j, i, 0, i);
            }

    // Calculate subblocks
    for (unsigned k = 1; k < widthSubM; k++)
        for (ulong j = 0; j < n/sampleRate + 1; j++)
            for (ulong i = 0; i < sampleRate/subSampleRate; i++)
//                if (i + (1 << k) - 1 < sampleRate/subSampleRate && j * sampleRate + (i + (1 << (k - 1))) * subSampleRate + (1 << k) - 1 < n)
                {
                    // Get indexes inside subblocks
                    ulong index1 = GetSubblockIndex(j, GetSubM(j, i, k - 1));
                    ulong index2 = GetSubblockIndex(j, GetSubM(j, i + (1 << (k - 1)), k - 1));
                    
                    if (index1 >= n)
                        index1 = n - 1;
                    if (index2 >= n)
                        index2 = n - 1;

                    if (GetValue(index1) <= GetValue(index2))
                        SetSubM(j, i, k, GetSubM(j, i, k - 1));
                    else
                        SetSubM(j, i, k, GetSubM(j, i + (1 << (k - 1)), k - 1));
                    #ifdef DEBUG_CRMQ
                        printf("Setting M_%lu[%lu, %d] = min([%lu, %d], [%lu, %d]) = %lu\n", j, i, k, i, k - 1, i + (1 << (k - 1)), k - 1, GetSubM(j, i, k));
                    #endif
                }

    // Init dynamic programming for blocks  (and widthM > 0)
    for (ulong i = 0; i < n/sampleRate + 1 && widthM > 0; i++)
    {
        #ifdef DEBUG_CRMQ
            printf("Setting M[%lu, %d] = %lu\n", i, 0, lookupSub(i, 0, sampleRate - 1));
        #endif
        M[i * widthM] = lookupSub(i, 0, sampleRate - 1);
    }
    
    // Calculate blocks
    for (unsigned k = 1; k < widthM; k++)
        for (ulong i = 0; i < n/sampleRate + 1; i++)
            if (i + (1 << k) - 1 < n/sampleRate + 1)
            {
                if (GetValue(M[i * widthM + k - 1]) <= GetValue(M[(i + (1 << (k - 1))) * widthM + k - 1]))
                    M[i * widthM + k] = M[i * widthM + k - 1];
                else
                    M[i * widthM + k] = M[(i + (1 << (k - 1))) * widthM + k - 1];
                
                #ifdef DEBUG_CRMQ
                    printf("Setting M[%lu, %d] = min([%lu, %d], [%lu, %d]) = %lu\n", i, k, i, k - 1, i + (1 << (k - 1)), k - 1, M[i * widthM + k]);
                #endif
            }

    #ifdef DEBUG_CRMQ_OLD
        for (ulong i = 0; i < n; i ++)
        {
            if (i % sampleRate == 0)
                for (unsigned k = 0; k < widthM; k++)
                    if (i + sampleRate * (1 << k) - 1 < n || k == 0)
                    {
                        printf("Calc M(%lu, %d)  = L[%lu, %lu] = %d\n", i / sampleRate, k, i, i + sampleRate * (1 << k) - 1, trmq->lookup(i, i + sampleRate * (1 << k) - 1));
                        if (M[(i / sampleRate) * widthM + k] != trmq->lookup(i, i + sampleRate * (1 << k) - 1))
                        {
                            printf("Mismatch at M(%lu, %d)  = L[%lu, %lu] = %d\n", i / sampleRate, k, i, i + sampleRate * (1 << k) - 1, trmq->lookup(i, i + sampleRate * (1 << k) - 1));
                            exit(0);
                        }
                    }

            if (i % subSampleRate == 0)
                for (unsigned k = 0; k < widthSubM; k++)
                    if ((i % sampleRate) + subSampleRate * (1 << k) - 1 < sampleRate)
                    {
                        printf("Calc M_%lu(%lu, %d)  = L[%lu, %lu] = %lu\n", i / sampleRate, (i % sampleRate) / subSampleRate, k, i, i + subSampleRate * (1 << k) - 1, trmq->lookup(i, i + subSampleRate * (1 << k) - 1) - (i - i % sampleRate));
                        if (GetSubM(i / sampleRate, (i % sampleRate) / subSampleRate, k) != ((trmq->lookup(i, i + subSampleRate * (1 << k) - 1) - (i - i % sampleRate)) % sampleRate) / subSampleRate)
                        {
                            printf("Mismatch at M_%lu(%lu, %d) = %lu, L[%lu, %lu] = %lu\n", i / sampleRate, (i % sampleRate) / subSampleRate, k, GetSubM(i / sampleRate, (i % sampleRate) / subSampleRate, k), i, i + subSampleRate * (1 << k) - 1, ((trmq->lookup(i, i + subSampleRate * (1 << k) - 1) - (i - i % sampleRate)) % sampleRate) / subSampleRate);
                            exit(0);
                        }
                    }
        }
    
    
        printf("Result:\n");
        for (ulong j = 0; j < n/sampleRate + 1; j++)
            for (ulong i = 0; i < sampleRate/subSampleRate; i++)
                for (unsigned k = 0; k < widthSubM; k++)
                    if (i + (1 << k) - 1 < sampleRate/subSampleRate && n > j * sampleRate + i * subSampleRate + (1 << k) - 1)
                        printf("M_%lu(%lu, %d) = %lu\n", j, i, k, GetSubM(j, i, k));
        delete trmq;
    #endif
}


CRMQ::~CRMQ()
{
    delete srmq;
    delete [] M;
    delete [] subM;
}

ulong CRMQ::lookup(ulong v, ulong w) const
{
    if (v >= n)
        v = n - 1;
    if (w >= n)
        w = n - 1;
    if (v == w)
        return v;
    if (v > w)
    {
        unsigned temp = v;
        v = w;
        w = temp;
    }
    
    ulong x = v / sampleRate,
          y = w / sampleRate;
    ulong minM = ~0, indexM = 0,          // Minimum value/index of blocks between x and y
          minStart, indexStart,   // Minimum values/indexes of start and end blocks
          minEnd, indexEnd;
    
    #ifdef DEBUG_CRMQ
        printf("x = %lu, y = %lu\n", x, y);
    #endif
    // Find minimum in blocks between x and y
    if (y - x > 1)
    {
        unsigned k = Tools::FloorLog2(y - x - 1);
        ulong valueX = GetValue(M[(x + 1) * widthM + k]);
        ulong valueY = GetValue(M[(y - (1 << k)) * widthM + k]);
        #ifdef DEBUG_CRMQ
            printf("k = %d, M[%lu, %d] = %lu, M[%lu, %d] = %lu, valueX = %lu, valueY = %lu\n", k, x + 1, k, M[(x + 1) * widthM + k], y - (1 << k), k, M[(y - (1 << k)) * widthM + k], valueX, valueY);
        #endif
        if (valueX < valueY)
        {
            minM = valueX;
            indexM = M[(x + 1) * widthM + k];
        }
        else
        {
            minM = valueY;
            indexM = M[(y - (1 << k)) * widthM + k];
        }
    
        #ifdef DEBUG_CRMQ
            printf("minM = %lu, indexM = %lu\n", minM, indexM);
        #endif
    }

    // Find minimum values for start and end blocks x and y    
    if (x != y)
    {
        indexStart = lookupSub(x, v % sampleRate, sampleRate - 1);
        minStart = GetValue(indexStart);
        #ifdef DEBUG_CRMQ
            printf("lookupSub(%lu, %lu, %d) = %lu (value = %lu)\n", x, v % sampleRate, sampleRate - 1, indexStart, minStart);
        #endif
        
        indexEnd = lookupSub(y, 0, w % sampleRate);
        minEnd = GetValue(indexEnd);
        #ifdef DEBUG_CRMQ
            printf("lookupSub(%lu, %d, %lu) = %lu (value = %lu)\n", y, 0, w % sampleRate, indexEnd, minEnd);
        #endif
        
        ulong returnIndex, returnValue;
        if (minStart <= minEnd)
        {
            returnIndex = indexStart;
            returnValue = minStart;
        }
        else
        {
            returnIndex = indexEnd;
            returnValue = minEnd;
        }
        
        if (y - x <= 1)
            return returnIndex;
        
        if (returnValue < minM)
            return returnIndex;
        
        return indexM;
    }
    
    #ifdef DEBUG_CRMQ
        printf("lookSub(%lu, %lu, %lu) (inside block)\n", x, v % sampleRate, w % sampleRate);
    #endif
    return lookupSub(x, v % sampleRate, w % sampleRate);
}

ulong CRMQ::lookupSub(ulong block, ulong v, ulong w) const
{    
    if (v >= sampleRate)
        v = sampleRate - 1;
    if (w >= sampleRate)
        w = sampleRate - 1;
    
    if (v > w)
    {
        unsigned temp = v;
        v = w;
        w = temp;
    }   
    
    if (block * sampleRate + v >= n)
        return n - 1;        
    if (block * sampleRate + w >= n)
        w = n % sampleRate;
    if (v == w)
        return block * sampleRate + v;
    
    
    ulong x = v / subSampleRate,
          y = w / subSampleRate;
    ulong minSubM = ~0, indexSubM = 0,    // Minimum value/index of subblocks between x and y
          minStart, indexStart,   // Minimum values/indexes of start and end subblocks
          minEnd, indexEnd;

    
              
    #ifdef DEBUG_CRMQ
        printf("block = %lu, x = %lu, y = %lu\n", block, x, y);
    #endif
    // Find minimum in subblocks between x and y
    if (y - x > 1)
    {
        unsigned k = Tools::FloorLog2(y - x - 1);
        // Get indexes inside subblocks
        ulong indexX = GetSubblockIndex(block, GetSubM(block, x + 1, k));
        ulong indexY = GetSubblockIndex(block, GetSubM(block, y - (1 << k), k));
        if (indexX >= n)
            indexX = n - 1;
        if (indexY >= n)
            indexY = n - 1;
        ulong valueX = GetValue(indexX);
        ulong valueY = GetValue(indexY);
        #ifdef DEBUG_CRMQ
            printf("k = %d, M_%lu[%lu, %d] = %lu, M_%lu[%lu, %d] = %lu, indexX = %lu, indexY = %lu, valueX = %lu, valueY = %lu\n", k, block, x + 1, k, GetSubM(block, x + 1, k), block, y - (1 << k), k, GetSubM(block, y - (1 << k), k), indexX, indexY, valueX, valueY);
        #endif
        if (valueX < valueY)
        {
            minSubM = valueX;
            indexSubM = indexX;
        }
        else
        {
            minSubM = valueY;
            indexSubM = indexY;
        }
    
        #ifdef DEBUG_CRMQ
            printf("minSubM = %lu, indexSubM = %lu\n", minSubM, indexSubM);
        #endif
    }

    // Find minimum values for start and end subblocks x and y
    if (x != y)
    {
        #ifdef DEBUG_CRMQ
            printf("Calculating RMQ(%lu, %lu) (%lu, %lu, %lu) (%lu, %lu)\n",   v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate, block, v, w, x, y);
        #endif
        
        indexStart = lookupSubblock(block, x, v % subSampleRate, subSampleRate - 1);
        if (indexStart > n - 1)
            indexStart = n - 1;
        minStart = GetValue(indexStart);
        #ifdef DEBUG_CRMQ
            printf("GetField from P index %lu\n", block * sampleRate / subSampleRate + x);
            printf("SubblockRMQ(%lu, %lu, %d) = %lu (value = %lu)\n", Tools::GetField(P, subSampleRate, block * sampleRate / subSampleRate + x), v % subSampleRate, subSampleRate - 1, indexStart, minStart);
        
            /*if (indexStart != trmq->lookup(v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate))
            {
                printf("Error in subblock RMQ(%lu, %lu) = %lu\n",   v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate, trmq->lookup(v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate));
                exit(0);
            }*/
        #endif
            
        indexEnd = lookupSubblock(block, y, 0, w % subSampleRate);
        if (indexStart > n - 1)
            indexStart = n - 1;
        minEnd = GetValue(indexEnd);
        #ifdef DEBUG_CRMQ
            printf("In subblock RMQ(%lu, %lu) = %lu (value = %lu)\n", y * subSampleRate + block * sampleRate, w + block * sampleRate, indexEnd, minEnd);
        #endif
        
        ulong returnIndex, returnValue;
        if (minStart <= minEnd)
        {
            returnIndex = indexStart;
            returnValue = minStart;
        }
        else
        {
            returnIndex = indexEnd;
            returnValue = minEnd;
        }
        
        if (y - x <= 1)
            return returnIndex;
        
        if (returnValue < minSubM)
            return returnIndex;
        
        if (returnValue == minSubM && returnIndex < indexSubM)
            return returnIndex;
            
        return indexSubM;
    }
    
    #ifdef DEBUG_CRMQ
        printf("In subblock RMQ(%lu, %lu) (inside subblock)\n", v + block * sampleRate, w + block * sampleRate);
    #endif
    return lookupSubblock(block, x, v % subSampleRate, w % subSampleRate);
}

// Get the index of a minimum value inside a subblock i of block j
// within interval [v, w]
ulong CRMQ::lookupSubblock(ulong block, ulong subblock, ulong v, ulong w) const
{
    ulong x = v / blockSampleRate,
          y = w / blockSampleRate;
    ulong minSubblock = ~0, indexSubblock = 0,    // Minimum value/index of subblocks between x and y
          minStart, indexStart,   // Minimum values/indexes of start and end subblocks
          minEnd, indexEnd;
    
    #ifdef DEBUG_CRMQ
        printf("block = %lu, subblock = %lu, x = %lu, y = %lu\n", block, subblock, x, y);
    #endif
    
    // Find minimum in subblocks between x and y
    if (y - x > 1)
    {
        ulong subblockStart = x + 1 + subblock * subSampleRate / blockSampleRate + block * sampleRate / blockSampleRate;
        indexSubblock = srmq->lookup(Tools::GetField(P, blockSampleRate, subblockStart), 0, blockSampleRate - 1) + subblockStart * blockSampleRate;
        if (indexSubblock >= n)
            indexSubblock = n - 1;
        minSubblock = GetValue(indexSubblock);
//         printf("j = %lu, i = %lu, subblockStart = %lu, minIndex = %lu (%lu)\n", j, i, subblockStart, minIndex, minValue);
        for (ulong k = 1; k < (y - x - 1); k ++)
        {
            ulong index = srmq->lookup(Tools::GetField(P, blockSampleRate, subblockStart + k), 0, blockSampleRate - 1) + (subblockStart + k) * blockSampleRate;
//             printf("subblockStart = %lu (%lu), index = %lu (%lu)\n", subblockStart + k, (subblockStart + k) * blockSampleRate, index, GetValue(index));
            if (index >= n)
                index = n - 1;
            if (minSubblock > GetValue(index))
            {
                minSubblock = GetValue(index);
                indexSubblock = index;
            }
        }

        #ifdef DEBUG_CRMQ
            printf("minSubblock = %lu, indexSubblock = %lu\n", minSubblock, indexSubblock);
        #endif
    }

    // Find minimum values for start and end subblocks x and y
    if (x != y)
    {
        #ifdef DEBUG_CRMQ
            
            //printf("Calculating RMQ(%lu, %lu) (%lu, %lu, %lu) (%lu, %lu)\n",   v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate, block, v, w, x, y);
        #endif
        
        indexStart = srmq->lookup(Tools::GetField(P, blockSampleRate, block * sampleRate / blockSampleRate + subblock * subSampleRate / blockSampleRate + x), v % blockSampleRate, blockSampleRate - 1) + block * sampleRate + subblock * subSampleRate + x * blockSampleRate;
        if (indexStart > n - 1)
            indexStart = n - 1;
        minStart = GetValue(indexStart);
        #ifdef DEBUG_CRMQ
            /*printf("GetField from P index %lu\n", block * sampleRate / subSampleRate + x);
            printf("SubblockRMQ(%lu, %lu, %lu) = %lu (value = %lu)\n", Tools::GetField(P, subSampleRate, block * sampleRate / subSampleRate + x), v % subSampleRate, subSampleRate - 1, indexStart, minStart);
        
            if (indexStart != trmq->lookup(v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate))
            {
                printf("Error in subblock RMQ(%lu, %lu) = %lu\n",   v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate, trmq->lookup(v + block * sampleRate, (x + 1) * subSampleRate - 1 + block * sampleRate));
                exit(0);
            }*/
        #endif
            
        indexEnd = srmq->lookup(Tools::GetField(P, blockSampleRate, block * sampleRate / blockSampleRate + subblock * subSampleRate / blockSampleRate + y), 0, w % blockSampleRate) + block * sampleRate + subblock * subSampleRate + y * blockSampleRate;
        if (indexStart > n - 1)
            indexStart = n - 1;
        minEnd = GetValue(indexEnd);
        #ifdef DEBUG_CRMQ
            //printf("In subblock RMQ(%lu, %lu) = %lu (value = %lu)\n", y * subSampleRate + block * sampleRate, w + block * sampleRate, indexEnd, minEnd);
        #endif
        
        ulong returnIndex, returnValue;
        if (minStart <= minEnd)
        {
            returnIndex = indexStart;
            returnValue = minStart;
        }
        else
        {
            returnIndex = indexEnd;
            returnValue = minEnd;
        }
        
        if (y - x <= 1)
            return returnIndex;
        
        if (returnValue < minSubblock)
            return returnIndex;
        
        if (returnValue == minSubblock && returnIndex < indexSubblock)
            return returnIndex;
            
        return indexSubblock;
    }
    
    #ifdef DEBUG_CRMQ
        //printf("In subblock RMQ(%lu, %lu) (inside subblock)\n", v + block * sampleRate, w + block * sampleRate);
    #endif
    return srmq->lookup(Tools::GetField(P, blockSampleRate, block * sampleRate / blockSampleRate + subblock * subSampleRate / blockSampleRate + x), v % blockSampleRate, w % blockSampleRate) + block * sampleRate + subblock * subSampleRate + x * blockSampleRate;
}
   
