// Implements operations over a sequence of balanced parentheses


/***************************************************************************
 *   Copyright (C) 2004 by Gonzalo Navarro                                 *
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
 
#include "Parentheses.h"

  // I have decided not to implement Munro et al.'s scheme, as it is too
  // complicated and the overhead is not so small in practice. I have opted 
  // for a simpler scheme. Each open (closing) parenthesis will be able to
  // find its matching closing (open) parenthesis. If the distance is shorter
  // than b, we will do it by hand, traversing the string. Otherwise, the
  // answer will be stored in a hash table. In fact, only subtrees larger than 
  // s will have the full distance stored, while those between b and s will
  // be in another table with just log s bits. The traversal by hand proceeds
  // in fact by chunks of k bits, whose answers are precomputed.
  // Space: there cannot be more than n/s subtrees larger than s, idem b.
  //   So we have (n/s)log n bits for far pointers and (n/b)log s for near 
  //   pointers. The space for the table is 2^k*k*log b. The optimum s=b log n,
  //   in which case the space is n/b(1 + log b + log log n) + 2^k*k*log b.
  // Time: the time is O(b/k), and we want to keep it O(log log n), so
  //   k = b/log log n.
  // (previous arguments hold if there are no unary nodes, but we hope that 
  //  there are not too many -- in revtrie we compress unary paths except when
  //  they have id)
  // Settings: using b = log n, we have 
  //   space = n log log n / log n + 2^(log n / log log n) log n
  //   time = log log n
  // In practice: we use k = 8 (bytes table), b = W (so work = 4 or 8)
  //   and space ~= n/3 + 10 Kbytes (fixed table). 
  // Notice that we need a hash table that stores only the deltas and does not
  // store the keys! (they would take log n instead of log s!). Collisions are
  // resolved as follows: see all the deltas that could be and pick the smallest
  // one whose excess is the same of the argument. To make this low we use a
  // load factor of 2.0, so it is 2n/3 after all.
  // We need the same for the reverses, for the forward is only for ('s and
  // reverses for )'s, so the proportion stays the same.
  // We also need the stream to be a bitmap to know how many open parentheses
  // we have to the left. The operations are as follows:
  // findclose: use the mechanism described above
  // findparent: similar, in reverse, looking for the current excess - 1
  //       this needs us to store the (near/far) parent of each node, which may
  //       cost more than the next sibling.
  // excess: using the number of open parentheses
  // enclose: almost findparent

	// creates a parentheses structure from a bitstring, which is shared
        // n is the total number of parentheses, opening + closing
   static unsigned char FwdPos[256][W/2];
   static unsigned char BwdPos[256][W/2];
   static char Excess[256];
   static bool tablesComputed = false;

Parentheses::Parentheses (ulong *string, ulong n, bool bwd, BitRank *br)
{ 
     ulong i,s,nb,ns,nbits;
     this->bp = string;
     this->n = n;
     this->br = br;
     nbits = Tools::bits(n-1);
     s = nbits*W;
     this->sbits = Tools::bits(s-1);
     s = 1lu << sbits; // to take the most advantage of what we can represent
     ns = (n+s-1)/s; nb = (s+W-1)/W; // adjustments
     near = far = pnear = pfar = 0lu;
     calcsizes();
#ifdef INDEXREPORT
     printf ("   Parentheses: total %i, near %i, far %i, pnear %i, pfar %i\n",n,near,far,pnear,pfar);
#endif
     this->sftable = new Hash (far,nbits,1.8);
     this->bftable = new Hash (near,sbits,1.8);
     if (bwd)
    { this->sbtable = new Hash (pfar,nbits,1.8);
          this->bbtable = new Hash (pnear,sbits,1.8);
    }
     else sbtable = bbtable = 0;
     filltables (bwd);
     if (!tablesComputed)
    { tablesComputed = true;
      for (i=0;i<256;i++) 
          { fcompchar (i,FwdPos[i],Excess+i); //printf("i = %d\t, FwdPos[i] = %c\t, Excess+i = %c\n", i,FwdPos[i],Excess+i);
            bcompchar (i,BwdPos[i]);//printf("i = %d\t, BwdPos[i] = %c\t, Excess+i = %c\n", i,BwdPos[i],Excess+i);
          }
    }
     
   }

    // frees parentheses structure, including the bitstream

Parentheses::~Parentheses ()

   { 
     delete sftable;
     if (sbtable) delete sbtable;
     delete bftable;
     if (bbtable) delete bbtable;
   }

void Parentheses::calcsizes()
{
    std::stack<ulong> *node = new std::stack<ulong>();
    node->push(~0); // Parent that does not exist
    
    // Iterate bit vector
    for (ulong i = 0; i < n; i ++)
        if (br->IsBitSet(i))
            node->push(i);  // Push node position        
        else
        {
            ulong posopen = node->top();   // Pop Open-leaf position from stack
            node->pop();
            ulong posparent = node->top(); // Peak for parent node position
            
            if ((i < n) && (i-posopen > W)) // exists and not small
            {
                if (i-posopen < (ulong)(1lu <<sbits))
                    this->near++; // near pointer
                else 
                    this->far++;
            }
                
            if ((posopen > 0) && (posopen-posparent > W)) // exists and not small
            {
                if (posopen-posparent < (ulong)(1lu <<sbits)) 
                    this->pnear++; // near pointer
                else 
                    this->pfar++;
            }
        }
    delete node;
}
   
/* Old recursive version:
ulong Parentheses::calcsizes (ulong posparent, ulong posopen)
{   
    ulong posclose,newpos;
    if ((posopen == n) || !br->IsBitSet(posopen))
        return posopen; // no more trees
        
    newpos = posopen;
    do { 
        posclose = newpos+1;
        newpos = calcsizes (posopen,posclose);
    } while (newpos != posclose);

    if ((posclose < n) && (posclose-posopen > W)) // exists and not small panga!!
        if (posclose-posopen < (ulong)(1lu <<sbits))
            this->near++; // near pointer
        else 
            this->far++;

    if ((posopen > 0) && (posopen-posparent > W)) // exists and not small
        if (posopen-posparent < (ulong)(1lu <<sbits)) 
            this->pnear++; // near pointer
        else 
            this->pfar++;
    return posclose;
}*/

void Parentheses::filltables(bool bwd)
{
    std::stack<ulong> *node = new std::stack<ulong>();
    node->push(~0); // Parent that does not exist
    
    // Iterate bit vector
    for (ulong i = 0; i < n; i ++)
        if (br->IsBitSet(i))
            node->push(i);  // Push node position        
        else
        {
            ulong posopen = node->top();   // Pop Open-leaf position from stack
            node->pop();
            ulong posparent = node->top(); // Peak for parent node position
            
            if ((i < n) && (i-posopen > W)) // exists and not small
            { 
                if (i-posopen < (ulong)(1lu <<sbits)) // near pointers
                    bftable->insertHash(posopen,i-posopen);
                else // far pointers
                    sftable->insertHash (posopen,i-posopen); 
            }
            
            if (bwd && (posopen > 0) && (posopen-posparent > W)) //exists and not small
            { 
                if (posopen-posparent < (ulong)((1lu <<sbits))) // near pointer
                    bbtable->insertHash (posopen,posopen-posparent);
                else // far pointers
                    sbtable->insertHash (posopen,posopen-posparent);
            }
        }

    delete node;
}

/* Old recursive version:
ulong Parentheses::filltables (ulong posparent, ulong posopen, bool bwd)
{ 
    ulong posclose,newpos;
    if ((posopen == n) || !br->IsBitSet(posopen))
	   return posopen; // no more trees
     
    newpos = posopen;
    do { 
        posclose = newpos+1;
        newpos = filltables (posopen,posclose,bwd);
	} while (newpos != posclose);
    
    if ((posclose < n) && (posclose-posopen > W)) // exists and not small
    { 
        if (posclose-posopen < (ulong)(1lu <<sbits)) // near pointers
           bftable->insertHash(posopen,posclose-posopen);
        else // far pointers
           sftable->insertHash (posopen,posclose-posopen); 
	}
     
    if (bwd && (posopen > 0) && (posopen-posparent > W)) //exists and not small
    { 
        if (posopen-posparent < (ulong)((1lu <<sbits))) // near pointer
	       bbtable->insertHash (posopen,posopen-posparent);
        else // far pointers
           sbtable->insertHash (posopen,posopen-posparent);
    }
    
    return posclose;
}*/
      
void Parentheses::fcompchar (unsigned char x, unsigned char *pos, char *excess)

   { int exc = 0;
     unsigned i;
     for (i=0;i<W/2;i++) pos[i] = 0;
     for (i=0;i<8;i++)
	 { if (x & 1) // closing
	      { exc--; 
		if ((exc < 0) && !pos[-exc-1]) pos[-exc-1] = i+1;
	      }
	   else exc++;
	   x >>= 1;
	 }
     *excess = exc;
   }

void Parentheses::bcompchar (unsigned char x, unsigned char *pos)

   { int exc = 0;
     unsigned i;
     for (i=0;i<W/2;i++) pos[i] = 0;
     for (i=0;i<8;i++)
	 { if (x & 128) // opening, will be used on complemented masks
	      { exc++; 
		if ((exc > 0) && !pos[exc-1]) pos[exc-1] = i+1;
	      }
	   else exc--;
	   x <<= 1;
	 }
   }

	// the position of the closing parenthesis corresponding to (opening)
	// parenthesis at position i

ulong Parentheses::findclose (ulong i)

   { ulong bitW;
     ulong len,res,minres,exc;
     unsigned char W1;
     ulong h;
     ulong myexcess;
     
     // Closing parenthesis for root
     if (i == 0)
        return n - 1;
	// first see if it is at small distance
     len = W; if (i+len >= n) len = n-i-1;
     bitW = ~(Tools::GetVariableField(bp,len,i+1));
     exc = 0; len = 0;
     while (bitW && (exc < W/2))
		// either we shift it all or it only opens parentheses or too
		// many open parentheses
        { W1 = bitW & 255;
          if ((res = FwdPos[W1][exc])) return i+len+res;
          bitW >>= 8; exc += Excess[W1];
	  len += 8;
	}
	// ok, it's not a small distance, try with hashing btable
     minres = 0;
     myexcess = excess (i);
     res = bftable->searchHash (i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if ((i+res+1 < n) && (excess(i+res+1) == myexcess)) 
		minres = res;
	  res = bftable->nextHash (&h);
	}
     if (minres) return i+minres;
	// finally, it has to be a far pointer
     res = sftable->searchHash (i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if ((i+res+1 < n) && (excess(i+res+1) == myexcess))
	        minres = res;
	  res = sftable->nextHash (&h);
	}
     return i+minres; // there should be one if the sequence is balanced!
   }

	// find enclosing parenthesis for an open parenthesis
	// assumes that the parenthesis has an enclosing pair

ulong Parentheses::findparent (ulong i)

   { ulong bitW;
     ulong len,res,minres,exc;
     unsigned char W1;
     ulong h;
     ulong myexcess;
     
	// first see if it is at small distance
     len = W; if (i < len) len = i-1;
     bitW = Tools::GetVariableField (bp,len, i-len) << (W-len);
     exc = 0; len = 0;
     while (bitW && (exc < W/2))  
		// either we shift it all or it only closes parentheses or too
		// many closed parentheses
        { W1 = (bitW >> (W-8));
          if ((res = BwdPos[W1][exc])) return i-len-res;
          bitW <<= 8; exc += Excess[W1]; // note W1 is complemented!
	  len += 8;
	}
	// ok, it's not a small distance, try with hashing btable
     minres = 0;
     myexcess = excess (i) - 1;
     res = bbtable->searchHash (i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if (excess(i-res) == myexcess) 
		minres = res;
	  res = bbtable->nextHash (&h);
	}
     if (minres) return i-minres;
	// finally, it has to be a far pointer
     res = sbtable->searchHash (i,&h);
     while (res)
	{ if (!minres || (res < minres)) 
	     if (excess(i-res) == myexcess)
		minres = res;
	  res = sbtable->nextHash (&h);
	}
     return i-minres; // there should be one if the sequence is balanced!
   }

	// # open - # close at position i, not included

ulong Parentheses::excess (ulong i)

   { if (i == 0)
        return 0;
     return br->rank(i-1)*2 - i;
   }

        // open position of closest parentheses pair that contains the pair
        // that opens at i, ~0 if no parent

ulong Parentheses::enclose (ulong i)

   { if (i == 0) return 0; // no parent!
     if (excess(i) == 1)
        return 0;
     return findparent (i);
   }

ulong Parentheses::isOpen (ulong i)

   { return br->IsBitSet(i);
   }


