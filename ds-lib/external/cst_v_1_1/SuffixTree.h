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

#ifndef _SUFFIXTREE_H_
#define _SUFFIXTREE_H_
#include <iostream>
#include "Tools.h"

#define COMPARE_TO_SUFFIX_TREE__OK 1
#define COMPARE_TO_SUFFIX_TREE__ROOT_FAILED 2
#define COMPARE_TO_SUFFIX_TREE__ISLEAF_FAILED 3
#define COMPARE_TO_SUFFIX_TREE__DEPTH_FAILED 4
#define COMPARE_TO_SUFFIX_TREE__PARENT_FAILED 5
#define COMPARE_TO_SUFFIX_TREE__SIBLING_FAILED 6
#define COMPARE_TO_SUFFIX_TREE__SUFFIX_LINK_FAILED 7
#define COMPARE_TO_SUFFIX_TREE__CHILD_FAILED 8
#define COMPARE_TO_SUFFIX_TREE__EDGE_FAILED 9

class SuffixTree
{
private:
    unsigned CompareTo(SuffixTree *, ulong);
public:
    virtual ~SuffixTree() {}
    virtual ulong root() = 0;
    virtual bool isleaf(ulong) = 0;
    virtual ulong child(ulong, uchar) = 0;
    virtual ulong sibling(ulong) = 0;
    virtual ulong parent(ulong ) = 0;
    virtual uchar edge(ulong, ulong) = 0;
    virtual ulong depth(ulong) = 0;
    virtual ulong lca(ulong, ulong) = 0;
    virtual ulong sl(ulong) = 0;
    virtual uchar *pathlabel(ulong) = 0;
    virtual ulong search(uchar *, ulong) = 0;
    unsigned CompareTo(SuffixTree *);
};

#endif
