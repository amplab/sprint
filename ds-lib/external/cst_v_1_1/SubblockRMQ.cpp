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

#include "SubblockRMQ.h"

SubblockRMQ::SubblockRMQ(unsigned sampleRate)
{
    answerWidth = Tools::CeilLog2(sampleRate);
    this->sampleRate = sampleRate;
    
    #ifdef DEBUG_SUBBLOCK_RMQ
        printf("Allocating %d bits in %d bytes for %d x %d x %d array (width = %d)\n", (1 << sampleRate) * sampleRate * sampleRate * answerWidth, ((1 << sampleRate) * sampleRate * sampleRate * answerWidth) /  W + 1, 1 << sampleRate, sampleRate, sampleRate, answerWidth);
    #endif
    answer = new ulong[((1 << sampleRate) * sampleRate * sampleRate * answerWidth) /  W + 1];

    for (ulong block = 0; block < (unsigned)(1 << sampleRate); block++)
    {        
        #ifdef DEBUG_SUBBLOCK_RMQ
            printf("0123456789\n");
            Tools::PrintBitSequence(&block, sampleRate);
        #endif
        for (unsigned i = 0; i < sampleRate; i++)
        {
            #ifdef DEBUG_SUBBLOCK_RMQ
                printf("Setting Answer(%d, %d, %d) = %d\n", block, i, i, i);
            #endif
            Tools::SetField(answer, answerWidth, block * sampleRate * sampleRate + i * sampleRate + i, i);
            
            for (unsigned j = i + 1; j < sampleRate; j++)
            {
                BitRank *brf = new BitRank(&block, sampleRate, false);
                int minValue = 2 * brf->rank(i) - i;
                unsigned minIndex = i;
                
                for (unsigned k = i + 1; k < j + 1; k++)
                    if (minValue > (int)(2 * brf->rank(k) - k))
                    {
                        minValue = (int)(2 * brf->rank(k) - k);
                        minIndex = k;
                    }
                delete brf;

                #ifdef DEBUG_SUBBLOCK_RMQ
                    printf("Setting Answer(%d, %d, %d) = %d\n", block, i, j, minIndex);
                #endif
                Tools::SetField(answer, answerWidth, block * sampleRate * sampleRate + i * sampleRate + j, minIndex);
                Tools::SetField(answer, answerWidth, block * sampleRate * sampleRate + j * sampleRate + i, minIndex);
            }
        }
    }
}

SubblockRMQ::~SubblockRMQ()
{
    delete [] answer;
}

unsigned SubblockRMQ::lookup(unsigned block, unsigned i, unsigned j) const
{
    return Tools::GetField(answer, answerWidth, block * sampleRate * sampleRate + i * sampleRate + j);
}


