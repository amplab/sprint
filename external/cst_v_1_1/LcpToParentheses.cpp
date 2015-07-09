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

#include "LcpToParentheses.h"

////////////////////////////////////////////////////////////////////////////
// Class LcpToParentheses::DeltaArray
LcpToParentheses::DeltaArray::DeltaArray(ulong size)
{    
    // pathological cases size should be 7*size 
    // this is taken into account when startPos gets too small:
    // the memory size is doubled
    this->size = size / ((ulong)log2(size)*(ulong)log2(size));
    if (this->size < 1024) this->size = 1024;
    A = new ulong[this->size / W + 1];
    startPos = this->size - 1;
}

LcpToParentheses::DeltaArray::~DeltaArray()
{
    delete [] A;
}

void LcpToParentheses::DeltaArray::Add(ulong value)
{
    ulong l = Tools::bits(value);
    ulong m = Tools::bits(l);
    if (startPos < 2*m+1+l) {
       // we will run out of space, let's double the space
       // this is not tested yet completely...
       //std::cout << "-------------------doubling space for deltas\n";
       ulong *B;
       B = A;
       A = new ulong[size/W + 1 + size/W +1];
       for (ulong i=0; i <= size/W; i++)
          A[size/W+i+1]=B[i];
       delete [] B;
       startPos = (size/W+1)*W+startPos;
       size = (size/W+1)*W+size;    
    }       
    startPos -= l;
    Tools::SetVariableField(A, l, startPos, value);
    startPos -= m;
    Tools::SetVariableField(A, m, startPos, l);
    startPos -= m + 1;
    Tools::SetVariableField(A, m + 1, startPos, (1 << m) - 1);
}
    
void LcpToParentheses::DeltaArray::Remove(ulong n)
{
    for (ulong i = 0; i < n; i++)
    {
        ulong k = 0;
        while (Tools::GetVariableField(A, 1, startPos + k))
            k++;
        startPos += k + 1;
        ulong j = Tools::GetVariableField(A, k, startPos);
        startPos += k + j;
    }
}

// Returns sum of n values, and the index after the last value (see GetNext())
ulong LcpToParentheses::DeltaArray::Sum(ulong n, ulong &index)
{
    ulong sum = 0;
    index = startPos; // Reset index parameter
    
    for (ulong i = 0; i < n; i++)
    {
        ulong k = 0;
        while (Tools::GetVariableField(A,1,index+k)) 
            k++;
        index += k + 1;   
        ulong j = Tools::GetVariableField(A, k, index);
        index += k;
        sum += Tools::GetVariableField(A, j, index);
        index += j;
    }
    return sum;
}

// Iterate through the array with the index parameter: 
// Returns the value of the given index (and the index of the next value)
ulong LcpToParentheses::DeltaArray::GetNext(ulong &index)
{
    ulong k = 0;
    while (Tools::GetVariableField(A,1,index+k)) 
        k++;
    index += k + 1;   
    ulong j = Tools::GetVariableField(A, k, index);
    index += k;
    ulong sum = Tools::GetVariableField(A, j, index);
    index += j;

    return sum;
}


////////////////////////////////////////////////////////////////////////////
// Class LcpToParentheses

ulong * LcpToParentheses::GetBalancedParentheses(CHgtArray *hgt, ulong n, ulong &bitsInP)
{
    if (n == 0)
        return 0;

    ulong sumIndex, sumValue;

    // Add the root and the first leaf
    BVTree *P = new BVTree();
    #ifdef LCPTOPARENTHESES_DEBUG
        printf("Append (()\n");
    #endif
    P->appendBit(true);
    P->appendBit(true);
    P->appendBit(false);
    
    // Init values and arrays
    ulong p = 1;
    DeltaArray *D = new DeltaArray(n);
    D->Add(1);
    DeltaArray *E = new DeltaArray(n);
    E->Add(n);
    
    // Add rest of the leafs
    for (ulong i = 1; i < n; i ++)
    {
        #ifdef LCPTOPARENTHESES_DEBUG
            printf("p = %lu, i = %lu\n", p, i);
        #endif
        ulong lcp = hgt->GetPos(i - 1);
        
        // Find the number of nodes to skip
        ulong j = 1;
        sumValue = E->Sum(j, sumIndex); // Passing sumIndex by reference
        while (n - sumValue > lcp)
        {
            j ++;
            sumValue += E->GetNext(sumIndex);
        }

        ulong splitLcp = n - sumValue;
        
        #ifdef LCPTOPARENTHESES_DEBUG
            printf("j = %lu\n", j);
            printf("Append %lu closing parentheses\n", j - 1);
        #endif
        
        // Append j - 1 closing parentheses
        for (ulong k = 0; k < j - 1; k ++)
            P->appendBit(false);
    
        if (splitLcp < lcp)
        {
            // Get split node's index
            ulong r = p - D->Sum(j - 1, sumIndex);
            #ifdef LCPTOPARENTHESES_DEBUG
                printf("r = %lu\n", r);
                printf("Inserting ( to index %lu\n", r);
                printf("Removing %lu from E\n", j);
                printf("Adding %lu to E\n", lcp - splitLcp);
                printf("Adding %lu to E\n", n - lcp);
                printf("Removing %lu from D\n", j - 1);
                printf("Adding %lu to D\n", p + j + 2 - r);
            #endif
            
            P->insertBit(true, r + 1);
            
            E->Remove(j);
            E->Add(lcp - splitLcp);
            E->Add(n - lcp);
            
            D->Remove(j - 1);
            D->Add(p + j + 2 - r);
            p++;
        }
        else
        {
            // Get split node's index
            ulong r = p - D->Sum(j, sumIndex);
            #ifdef LCPTOPARENTHESES_DEBUG
                printf("r = %lu\n", r);
                printf("Removing %lu from E\n", j);
                printf("Adding %lu to E\n", n - lcp);
                printf("Removing %lu from D\n", j);
                printf("Adding %lu to D\n", p + j + 1 - r);
            #endif
                
            E->Remove(j);
            E->Add(n - lcp);

            D->Remove(j);
            D->Add(p + j + 1 - r);
        }
                
        #ifdef LCPTOPARENTHESES_DEBUG
            printf("Append ()\n");
        #endif
        P->appendBit(true);
        P->appendBit(false);
        p += 2 + j - 1;
    }
    
    // Find the number of nodes to close
    ulong j = 1;
    sumValue = E->Sum(j, sumIndex); // Passing sumIndex by reference
    while (n - sumValue > 0)
    {
        j ++;
        sumValue += E->GetNext(sumIndex);
    }

    #ifdef LCPTOPARENTHESES_DEBUG
        printf("Append %lu closing parentheses to end\n", j);
    #endif
    for (ulong k = 0; k < j; k ++)
        P->appendBit(false);
    
    ulong *bp = P->getBits();
    bitsInP = P->getPositions();
    //printf("bitsInP = %lu, getPositions() = %lu\n", bitsInP, P->getPositions());
    
    delete D;
    delete E;
    
    delete P;
    return bp;
}

// Construct from a file
ulong * LcpToParentheses::GetBalancedParentheses(const char *filename, ulong &bitsInP)
{
    std::ifstream file (filename, ios::in|ios::binary);
    if (file.is_open())
    {
        std::cout << "Loading balanced parentheses from file: " << filename << std::endl;
        file.read((char *)&bitsInP, sizeof(ulong));
        ulong *bp = new ulong[bitsInP / W + 1];
        for (ulong offset = 0; offset < (bitsInP/W + 1); offset ++)
            file.read((char *)(bp + offset), sizeof(ulong));
        file.close();
        return bp;
    }
    else 
    {
        std::cout << "Unable to open file " << filename << std::endl;
        exit(1);
    }
    return 0;
}

void LcpToParentheses::SaveToFile(const char *filename, ulong *bp, ulong bitsInP)
{
    std::ofstream file (filename, ios::out|ios::binary|ios::trunc);
    if (file.is_open())
    {
        std::cout << "Writing balanced parentheses to file: " << filename << std::endl;
        file.write((char *)&bitsInP, sizeof(ulong));
        std::cout << "Writing balanced parentheses of " << (bitsInP/W + 1) << " words." << std::endl;
        for (ulong offset = 0; offset < (bitsInP/W + 1); offset ++)
            file.write((char *)(bp + offset), sizeof(ulong));
        file.close();
    }
    else 
    {
        std::cout << "Unable to open file " << filename << std::endl;
        exit(1);
    }
}
