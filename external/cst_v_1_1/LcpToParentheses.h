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

#ifndef _LCPTOPARENTHESES_H_
#define _LCPTOPARENTHESES_H_

#include "Tools.h"
#include "CHgtArray.h"

class LcpToParentheses
{
private:
    class DeltaArray
    {
    private:
        ulong *A;
        ulong startPos;
	ulong size; 
    public:
        DeltaArray(ulong);
        ~DeltaArray();
        void Add(ulong);
        void Remove(ulong);
        ulong Sum(ulong, ulong &);
        ulong GetNext(ulong &);
    };

public:
    static ulong * GetBalancedParentheses(CHgtArray *, ulong, ulong &);
    static ulong * GetBalancedParentheses(const char *, ulong &);
    static void SaveToFile(const char *, ulong *, ulong);
};

#endif
