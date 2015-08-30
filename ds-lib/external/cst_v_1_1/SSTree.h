/***************************************************************************
 *   Sadakane's Compressed suffix tree                                     *
 *                                                                         *
 *   Copyright (C) 2006 by Niko V�lim�ki, Kashyap Dixit                    *
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

#ifndef _SSTREE_H_
#define _SSTREE_H_

#include "SuffixTree.h"
#include "CSA.h"
#include "CHgtArray.h"
#include "BitRank.h"
#include "ReplacePattern.h"
#include "CRMQ.h"
#include "Tools.h"
#include "Parentheses.h"
#include "LcpToParentheses.h"
#include <iostream>


// (Un)comment next line to get more/less debug information about the tree construction
//#define SSTREE_TIMER

// Requires HeapProfiler class:
//#define SSTREE_HEAPPROFILE


/**
 * Compressed suffix tree class
 *
 * Suffix tree is represented using several compressed       
 * data structures. Most important to understant the usage       
 * of the Balanced Parantheses (BP) representation of the tree   
 * hierarchy: the tree is traversed prefix order printing "("    
 * when a node is visited first time and printing ")" when       
 * a node is visited last time. E.g. "((()())())" is a tree with 
 * root having two children, its left child having two leaves,   
 * and its right child being a leaf.                             
 *                                                               
 * A node in the tree is represented by the index of the         
 * corresponding "(" in the balanced parentheses representation.
 *
 * References:
 *
 * Niko V�lim�ki, Wolfgang Gerlach, Kashyap Dixit, and Veli M�kinen. 
 * Engineering a Compressed Suffix Tree Implementation, Published at 
 * 6th Workshop on Experimental Algorithms (WEA 2007), June 6-8, Italy.
 *
 * K. Sadakane. Compressed suffix trees with full functionality. Theory of 
 * Computing Systems, 2006. To appear, preliminary version available at 
 * http://tcslab.csce.kyushu-u.ac.jp/~sada/papers/cst.ps
 */
class SSTree : public SuffixTree
{
private:
    ulong n;
    CHgtArray *hgt;
    CRMQ *rmq;
    ulong *P;
    ReplacePattern *rpLeaf, *rpSibling;
    BitRank *br, *brLeaf, *brSibling;
    Parentheses *Pr;
    
public:
    /**
     * IO action for constructor SSTree(), filename given as the last parameter.
     * Defaults to no operation.
     */
    enum io_action
    {
        nop,       // No operation
        load_from, // Load from file
        save_to    // Save to file
    };

    CSA *sa;

    SSTree(uchar *, ulong, bool = false, unsigned = 0, io_action = nop, const char * = 0);
    ~SSTree();
    ulong root();
    bool isleaf(ulong) ;
    ulong child(ulong, uchar);
    ulong firstChild(ulong);
    ulong sibling(ulong) ;
    ulong parent(ulong ) ;
    uchar edge(ulong, ulong) ;
    uchar* edge(ulong) ;
    uchar* pathlabel(ulong);
    uchar* substring(ulong, ulong);
    ulong depth(ulong);
    ulong nodeDepth(ulong);
    ulong lca(ulong, ulong);
    ulong lceLinear(uchar *, ulong, ulong);
    ulong lce(ulong, ulong);
    ulong sl(ulong);
    ulong inorder(ulong);
    ulong rightmost(ulong);
    ulong leftmost(ulong);
    ulong leftrank(ulong);
    ulong numberofnodes(ulong);
    ulong numberofleaves(ulong);
    ulong textpos(ulong);
    ulong isOpen(ulong);
    ulong search(uchar *, ulong);
    void PrintHgt();
    ulong lcaParen(ulong, ulong);
    void PrintSA();
    void PrintEdge(ulong);
    void CheckLCA(ulong);
    void PrintTree(ulong, int);
};

#endif
