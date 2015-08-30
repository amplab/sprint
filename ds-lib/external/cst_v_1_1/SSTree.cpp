/***************************************************************************
 *   Sadakane's Compressed suffix tree                                     *
 *                                                                         *
 *   Copyright (C) 2006 by Niko Välimäki, Kashyap Dixit                    *
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

// References:
// 
// K. Sadakane. Compressed suffix trees with full functionality. Theory of 
// Computing Systems, 2007. To appear, preliminary version available at 
// http://tcslab.csce.kyushu-u.ac.jp/~sada/papers/cst.ps

#include "SSTree.h"

#ifdef SSTREE_HEAPPROFILE
#   include "HeapProfiler.h"
#endif

/**
 * Constructor.
 *
 * Compressed suffix tree is a self-index so text is not needed after construction.
 * Text can be deleted during construction to save some construction space.
 *
 * Parameter <filename> is a prefix for filenames used for IO operation.
 * Filename suffixes are ".bwt", ".lcp" and ".bp".
 *
 * @param text a pointer to the text.
 * @param n number of symbols in the text.
 * @param deletetext delete text as soon as possible.
 * @param samplerate sample rate for Compressed suffix array.
 * @param action IO action for <filename>. Defaults to no operation.
 * @param filename Prefix for the filenames used.
 */
SSTree::SSTree(uchar *text, ulong n, bool deletetext, unsigned samplerate, io_action IOaction, const char *filename)
{
    #ifdef SSTREE_HEAPPROFILE
        ulong heapCon;
    #endif
    this->n = n;
    unsigned floorLog2n = Tools::FloorLog2(n);
    if (floorLog2n < 4)
        floorLog2n = 4;
    unsigned rmqSampleRate = floorLog2n / 2;
    if (samplerate != 0)
        floorLog2n = samplerate; // Samplerate override, affects only CSA
    
    #ifdef SSTREE_TIMER
    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
        heapCon = HeapProfiler::GetHeapConsumption();
    #endif

        printf("Creating CSA with samplerate %u\n", floorLog2n);
        fflush(stdout);
        Tools::StartTimer();    
    #endif
    if (IOaction == load_from && filename != 0)
        sa = new CSA(text, n, floorLog2n, (string(filename)+".csa").c_str()); 
    else if (IOaction == save_to && filename != 0)
        sa = new CSA(text, n, floorLog2n, 0, (string(filename)+".csa").c_str()); 
    else // No IO operation
        sa = new CSA(text, n, floorLog2n); 
    #ifdef SSTREE_TIMER
        printf("CSA created in %.0f seconds.\n", Tools::GetTime());

    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl; 
        heapCon = HeapProfiler::GetHeapConsumption();   
    #endif

        printf("Creating CHgtArray\n");
        fflush(stdout);
        Tools::StartTimer();    
    #endif
    if (IOaction == load_from && filename != 0)
        hgt = new CHgtArray(sa, (string(filename)+".lcp").c_str());
    else
        hgt = new CHgtArray(sa, text, n);

    if (IOaction == save_to && filename != 0)
        hgt->SaveToFile((string(filename)+".lcp").c_str());
    #ifdef SSTREE_TIMER
        printf("CHgtArray created in %.0f seconds.\n", Tools::GetTime());
   
    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
        heapCon = HeapProfiler::GetHeapConsumption();
    #endif

        printf("Creating parentheses sequence (LcpToParentheses)\n");
        fflush(stdout);
        Tools::StartTimer();
    #endif
    if (deletetext)
       delete [] text;
    
    #ifdef SSTREE_HEAPPROFILE
        heapCon = HeapProfiler::GetHeapConsumption(); 
    #endif
    
    ulong bitsInP;
    if (IOaction == load_from && filename != 0)
        P = LcpToParentheses::GetBalancedParentheses((string(filename)+".bp").c_str(), bitsInP);
    else
        P = LcpToParentheses::GetBalancedParentheses(hgt, n, bitsInP);

    if (IOaction == save_to && filename != 0)
        LcpToParentheses::SaveToFile((string(filename)+".bp").c_str(), P, bitsInP);
    #ifdef SSTREE_TIMER
        printf("Parentheses sequence created in %.0f seconds.\n", Tools::GetTime());
    
    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
        heapCon = HeapProfiler::GetHeapConsumption();
    #endif

        //printf("Creating CSA with sample rate %d\n", floorLog2n);
        //Tools::StartTimer();    
    #endif
    //delete sa;
    //sa = new CSA(text, n, floorLog2n);
    //hgt->SetSA(sa);  // Update SA pointer
    #ifdef SSTREE_TIMER
        //printf("CSA created in %f seconds.\n", Tools::GetTime());

        printf("Creating BitRank\n");
        fflush(stdout);
        Tools::StartTimer();    
    #endif
    br = new BitRank(P, bitsInP, false);
    #ifdef SSTREE_TIMER
        printf("BitRank created in %.0f seconds.\n", Tools::GetTime());

    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
        heapCon = HeapProfiler::GetHeapConsumption();
    #endif

        printf("Creating Parentheses\n");
        fflush(stdout);
        Tools::StartTimer();    
    #endif
    Pr = new Parentheses(P, bitsInP, true, br);
    #ifdef SSTREE_TIMER
        printf("Parentheses created in %.0f seconds.\n", Tools::GetTime());

    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
        heapCon = HeapProfiler::GetHeapConsumption();
    #endif

        printf("Creating ReplacePatterns\n");
        fflush(stdout);
        Tools::StartTimer();    
    #endif
    rpLeaf = new ReplacePattern(1, 8);
    rpSibling = new ReplacePattern(0, 8);
    #ifdef SSTREE_TIMER
        printf("ReplacePatterns created in %.0f seconds.\n", Tools::GetTime());

    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
        heapCon = HeapProfiler::GetHeapConsumption();
    #endif

        printf("Creating BitRanks\n");
        fflush(stdout);
        Tools::StartTimer();    
    #endif
    brLeaf = new BitRank(P, bitsInP, false, rpLeaf);       //for ()
    brSibling = new BitRank(P, bitsInP, false, rpSibling); //for )(
    
    if (rmqSampleRate < 4)
        rmqSampleRate = 4;
    #ifdef SSTREE_TIMER
        printf("BitRanks created in %.0f seconds.\n", Tools::GetTime());
//         printf("<enter>\n");
//         std::cin.get();

    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
        heapCon = HeapProfiler::GetHeapConsumption();
    #endif

        printf("Creating CRMQ with sample rates %d, %d and %d\n", rmqSampleRate * rmqSampleRate * rmqSampleRate, rmqSampleRate * rmqSampleRate, rmqSampleRate);
        Tools::StartTimer();    
        fflush(stdout);
    #endif
    rmq = new CRMQ(br, P, bitsInP, rmqSampleRate * rmqSampleRate * rmqSampleRate, rmqSampleRate * rmqSampleRate, rmqSampleRate);
    #ifdef SSTREE_TIMER
    #ifdef SSTREE_HEAPPROFILE
        std::cout << "--> HeapProfiler: " << HeapProfiler::GetHeapConsumption() - heapCon << ", " << HeapProfiler::GetMaxHeapConsumption() << std::endl;
    #endif
        printf("CRMQ created in %.0f seconds.\n", Tools::GetTime());
        fflush(stdout);
    #endif
}

/**
 * A destructor. 
 */
SSTree::~SSTree()
{
    delete rmq;
    delete sa;
    delete rpLeaf;
    delete rpSibling;
    delete brLeaf;
    delete brSibling;
    delete br;
    delete hgt;
    delete Pr;
    delete [] P;
}

/**
 * Returns the position of the root node in the parentheses sequence.
 */
ulong SSTree::root()
{
    return 0;
}

/** 
 * Check if node v is a leaf node.
 *
 * Method doesn't check for an open parenthesis at position v (because we can assume that v is a node).
 */
bool SSTree::isleaf(ulong v)
{
    return !br->IsBitSet(v + 1);
}

/**
 * Returns the child node of v so that edge leading to the child node starts with the symbol c.
  */
ulong SSTree::child(ulong v, uchar c)
{
    if (isleaf(v))
        return 0;
        
    v++;   // First child of v
    while (v != 0)
    {
        if (c == edge(v,1)) 
            return v;
        v = sibling(v);
    }
    return 0;
}

/**
 * Returns the first child of the (internal) node v.
 */
ulong SSTree::firstChild(ulong v)
{
    if (isleaf(v))
        return 0;
        
    return v+1;
}

/**
 * Returns the next sibling of the node v.
 */
ulong SSTree::sibling(ulong v)
{
    if (v == 0)
        return 0;

    ulong w = Pr->findclose(parent(v));
    ulong i = Pr->findclose(v) + 1;
    if (i < w)
        return i;

    return 0;   // Returns zero if no next sibling
}

/**
 * Returns the parent of the node v.
 */
ulong SSTree::parent(ulong v)
{
    return Pr->enclose(v);
}

/**
 * Returns the d-th character of the label of the node v's incoming edge.
 */
uchar SSTree::edge(ulong v, ulong d) 
{
    uchar *ss; 
    if (isleaf(v))
    {
        ulong i = leftrank(v);
        ulong j = depth(parent(v));
        ulong d1 = sa->lookup(i) + j;
        if (d > n - d1)
            return 0u;
        ss = sa->substring(d1 + d - 1,1);
        uchar result = ss[0];
        delete [] ss;
        return result;
    }
    
    ulong d1 = hgt->GetPos(inorder(parent(v)));
    ulong d2 = hgt->GetPos(inorder(v));
    if (d > d2 - d1)
        return 0u;
    ss = sa->substring(sa->lookup(inorder(v)) + d1 + d - 1,1);
    uchar result = ss[0];
    delete [] ss;
    return result;    
}

/**
 * Returns the edge label of the incoming edge of the node v.
 */
uchar* SSTree::edge(ulong v) 
{
    if (isleaf(v))
    {
        ulong i = leftrank(v);
        ulong j = depth(parent(v));
    ulong k = depth(v);
        ulong d1 = sa->lookup(i) + j;
        return sa->substring(d1,k-j);
    }
    
    ulong d1 = hgt->GetPos(inorder(parent(v)));
    ulong d2 = hgt->GetPos(inorder(v));
   
    return sa->substring(sa->lookup(inorder(v))+d1, d2-d1);    
}

/**
 * Returns the path label from root to the node v.
 */
uchar* SSTree::pathlabel(ulong v) 
{
    if (isleaf(v))
    {
        ulong i = leftrank(v);
        ulong k = depth(v);
        ulong d1 = sa->lookup(i);
        return sa->substring(d1,k);
    }
    ulong d2 = hgt->GetPos(inorder(v));
    return sa->substring(sa->lookup(inorder(v)),d2);
}

/**
 * Returns a substring of the original text sequence.
 *
 * @param i starting position.
 * @param k length of the substring.
 */
uchar *SSTree::substring(ulong i, ulong k)
{
   return sa->substring(i,k);
}

/**
 * Returns the string depth of the node v.
 */
ulong SSTree::depth(ulong v)
{
    if (v == 0)
        return 0;

    if (isleaf(v))
    {
        ulong i = leftrank(v);
        return n - sa->lookup(i);
    }
        
    v = inorder(v);
    return hgt->GetPos(v);  
}

/**
 * Returns the node depth of the node v.
 *
 * Node depth for the root node is zero.
 */
ulong SSTree::nodeDepth(ulong v)
{
    return 2 * br->rank(v) - v - 2;
}

/**
 * Returns the Lowest common ancestor of nodes v and w.
 */
ulong SSTree::lca(ulong v, ulong w)
{
    if (v == 0 || w == 0)
        return 0;

    if (v == w || v == root())
        return v;
    if (w == root())
        return root();

    if (v > w)
    {
        ulong temp = w;
        w = v;
        v = temp;
    }
    
    if (Pr->findclose(v) > Pr->findclose(w))
        return v;
    
    return parent(rmq->lookup(v, w) + 1);
}


/**
 * Longest common extension of text positions i and j.
 *
 * Linear time solution.
 */
ulong SSTree::lceLinear(uchar *text, ulong i, ulong j)
{
    ulong k = 0;
    while (text[i+k] == text[j+k])
        k++;
    
    return k;
}

/**
 * Returns the Longest common extension of text positions i and j.
 */
ulong SSTree::lce(ulong i, ulong j)
{
    i = sa->inverse(i);
    ulong v = brLeaf->select(i + 1);
    j = sa->inverse(j);
    ulong w = brLeaf->select(j + 1);
    return depth(lca(v, w));
}

/**
 * Suffix link for internal nodes
 */
ulong SSTree::sl(ulong v)
{
    if (v == 0 || v == root() || isleaf(v))
        return 0;
    
    ulong x = brLeaf->rank(v - 1) + 1;
    ulong y = brLeaf->rank(Pr->findclose(v));
    x = sa->Psi(x - 1);
    y = sa->Psi(y - 1);
    return lca(brLeaf->select(x + 1), brLeaf->select(y + 1));
}

/**
 * Prints Hgt array values
 */
void SSTree::PrintHgt()
{
    std::cout << "Hgt: [ ";
    for (ulong i = 0; i < n; i++)
        std::cout << i << "=" << hgt->GetPos(i) << " ";
    std::cout << "]\n";
}

/**
 * Prints Suffix array values
 */
void SSTree::PrintSA()
{
    std::cout << "SA: [ ";
    for (ulong i = 0; i < n; i++)
        std::cout << i << "=" << sa->lookup(i) << " ";
    std::cout << "]\n";
}

/**
 * Prints the edge label of the incoming edge
 */
void SSTree::PrintEdge(ulong v)
{
    ulong k = 1;
    while (edge(v, k) != 0u)
            {
                printf("%c", (int)edge(v, k));
                k++;
            }
           printf("\n");
}

/**
 * Returns the Lowest common ancestor of nodes v and w.
 *
 * Linear time solution for debuging.
 */
ulong SSTree:: lcaParen(ulong v, ulong w)
{
      
    ulong temp;
    if (v < w) temp = Pr->findclose(w);
    else temp = Pr->findclose(v);
    if (v == w) return w;
    if (v > w)
        v = w;
    
    while (v > 0)
    {
        if (Pr->findclose(v) > temp) return v; 
        v = parent(v); 
    }
    
    return 0;
}

/**
 * Debug function for Lowest common ancestor
 */
void SSTree::CheckLCA(ulong v)
{
    ulong len = br->rank(rightmost(v));
    v++;ulong w, temp, v1;
    for (w = 1; w < len - 1; w++)
    {
        temp = br->select(w);
        v1 = br->select(w+1);
        while(v1 < len)
        {   
            if (lca(temp, v1) != lcaParen(temp, v1))
            {
                printf("conflict at (%lu, %lu)::lcaParen() = %lu and lca() = %lu\n", temp, v1, lcaParen(temp, v1), lca(temp, v1));
                exit(0);
            }
            v1 = br->select(br->rank(v1)+1);
        }
        
        // Check for the value v1 == len
        if (lca(temp, v1) != lcaParen(temp, v1))
        {
            printf("conflict at (%lu, %lu)::lcaParen() = %lu and lca() = %lu\n", temp, v1, lcaParen(temp, v1), lca(temp, v1));
            exit(0);
        }        
    }
}


/**
 * Prints edge labels of the Suffix tree
 */
void SSTree::PrintTree(ulong v, int depth)
{
    for (int i=0;i< depth;i++)
      printf(" ");
   
    if (v != 0) 
    {
         PrintEdge(v);
    }
    if(!(isleaf(v)))
    {
        PrintTree(v + 1, depth+1);
    }
    v = sibling(v);
    if (v != 0) 
        PrintTree(v, depth);
}

/**
 * Returns the Right most leaf of the (internal) node v.
 */
ulong SSTree::rightmost(ulong v)
{
    return brLeaf->select(brLeaf->rank(Pr->findclose(v)));
}

/**
 * Returns the Left most leaf of the (internal) node v.
 */
ulong SSTree::leftmost(ulong v)
{
    return brLeaf->select(brLeaf->rank(v)+1);
}

/**
 * Returns the Left rank of a (leaf) node.
 *
 * @see textpos()
 */
ulong SSTree::leftrank(ulong v)
{
    return brLeaf->rank(v - 1);
}

/**
 * Returns the Inorder number of a node
 */
ulong SSTree::inorder(ulong v)
{
    return brLeaf->rank(Pr->findclose(v+1)) - 1;
}

/**
 * Returns the Number of nodes in a subtree rooted at the node v.
 */
ulong SSTree::numberofnodes(ulong v)
{
    return (Pr->findclose(v)-v-1)/2+1;
}

/**
 * Returns the Number of leaf nodes in a subtree rooted at the node v.
 */
ulong SSTree::numberofleaves(ulong v)
{
    return leftrank(Pr->findclose(v))-leftrank(v);
}

/**
 * Returns the Suffix array value of the leaf node v.
 */
ulong SSTree::textpos(ulong v)
{
    // works correctly if v is a leaf
    // otherwise returns the textpos of leaf previous to v in preorder  
    return sa->lookup(this->leftrank(v));
}

/**
 * Check for an open parentheses at index v.
 */
ulong SSTree::isOpen(ulong v)
{
    return Pr->isOpen(v);
}

/**
 * Search for a given pattern of length l from the suffix array.
 * Returns a node from the suffix tree that is LCA of the matching leaf interval.
 */
ulong SSTree::search(uchar *pattern, ulong l)
{
    if (l == 0)
        return 0;
        
    ulong sp, ep;
    if (!sa->Search(pattern, l, &sp, &ep))
        return root();       // Return empty match
    
    // Fetch leaf nodes
    sp = brLeaf->select(sp+1);
    ep = brLeaf->select(ep+1);

    // Check if sp and ep are the same leaf node
    if (sp == ep)
        return sp;
    ep ++;

    // Get rank_1's
    ulong r_sp = br->rank(sp),
          r_ep = br->rank(ep);
    // Calculate number of open and closed parentheses on the interval [sp, ep]
    ulong open = r_ep - r_sp + 1,
          close = (ep - r_ep) - (sp - r_sp);
    
    // Correct boundaries
    if (open < close) 
    {
        sp   -= close - open;
        r_sp -= close - open;  // Rank changes also
    }
    if (open > close)
        ep += open - close;    // Rank (r_ep) doesn't change
    
    // Number of close parentheses on the interval [0, sp]
    close = sp - r_sp + 1;
    // Index of the nearest closed parenthesis to the left from the index sp
    if (close > 0)
        close = br->select0(close);
    else
        close = 0;  // Safe choice when there is no closed parenthesis at left side

    // Number of 1-bits in the balanced parentheses sequence --- the last bit is always 0
    open = br->rank(br->NumberOfBits() - 2);
    // Index of the nearest open parenthesis to the right from the index ep
    if (r_ep + 1 <= open)
        open = br->select(r_ep + 1);
    else
        open = br->NumberOfBits() - 1; // Safe choice when there is no open parenthesis at right side
    
    // Select the closest of these two limits
    if (sp - close <= open - ep)
        return close + 1;
    else
        return sp - (open - ep) + 1;
}

