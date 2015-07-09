/***************************************************************************
 *   Copyright (C) 2006 by Veli Mäkinen and Niko Välimäki                  *
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
 
#include "CSA.h"

////////////////////////////////////////////////////////////////////////////
// Class CSA::THuffAlphabetRank

CSA::THuffAlphabetRank::THuffAlphabetRank(uchar *s, ulong n, TCodeEntry *codetable, unsigned level) {
    left = NULL;
    right = NULL;
    bitrank = NULL;
    ch = s[0];
    leaf = false;
    this->codetable = codetable;
    
    bool *B = new bool[n];
    ulong sum=0,i;
    /*
    for (i=0; i< n; i++) {
       printf("%c:", (char)((int)s[i]-128));
       for (r=0;r<codetable[(int)s[i]].bits;r++)
          if (codetable[(int)s[i]].code & (1u <<r))
	     printf("1");
	  else printf("0");
       printf("\n");	  
    }
    printf("\n");
    if (level > 100) return;*/
    for (i=0;i<n;i++) 
       if (codetable[(int)s[i]].code & (1u << level)) {
          B[i] = true;
          sum++;
       }
       else B[i] = false;
    if (sum==0 || sum==n) {
        delete [] B;
	leaf = true;
        return;
    } 
    uchar *sfirst, *ssecond;
    //if (n-sum > 0)  
        sfirst = new uchar[n-sum];
    //if (sum > 0) 
        ssecond = new uchar[sum];
    unsigned j=0,k=0;
    for (i=0;i<n;i++)
        if (B[i]) ssecond[k++] = s[i];
        else sfirst[j++] = s[i];
    ulong *Binbits = new ulong[n/W+1];
    for (i=0;i<n;i++)
        Tools::SetField(Binbits,1,i,B[i]); 
    delete [] B;
    bitrank = new BitRank(Binbits,n,true);
    //if (j > 0) { 
        left = new THuffAlphabetRank(sfirst,j,codetable,level+1); 
        delete [] sfirst;
    //}
    //if (k>0) {
        right = new THuffAlphabetRank(ssecond,k,codetable,level+1); 
        delete [] ssecond;
    //}
}


bool CSA::THuffAlphabetRank::Test(uchar *s, ulong n) {
    // testing that the code works correctly
    int C[256];
    unsigned i,j;
    bool correct=true;
    for (j=0;j<256;j++)
        C[j] = 0;
    for (i=0;i<n;i++) {
        C[(int)s[i]]++;
        if (C[(int)s[i]] != (int)rank((int)s[i],i)) {
        correct = false;
        printf("%d (%c): %d<>%d\n",i,(int)s[i]-128,C[(int)s[i]],(int)rank((int)s[i],i)); 
        }         
    }
    return correct;            
}

CSA::THuffAlphabetRank::~THuffAlphabetRank() {
    if (left!=NULL) delete left;
    if (right!=NULL) delete right;
    if (bitrank!=NULL)   
        delete bitrank;
}


////////////////////////////////////////////////////////////////////////////
// Class CSA

CSA::CSA(uchar *text, ulong n, unsigned samplerate, const char *loadFromFile, const char *saveToFile) {
    this->n = n;
    this->samplerate = samplerate;

    uchar *bwt;
    if (loadFromFile != 0)
        bwt = LoadFromFile(loadFromFile);
    else
        bwt = BWT(text);
    if (saveToFile != 0)
        SaveToFile(saveToFile, bwt);

    ulong i,min = 0,
             max;
    for (i=0;i<256;i++)
        C[i]=0;
    for (i=0;i<n;++i)
        C[(int)bwt[i]]++;
    for (i=0;i<256;i++)
        if (C[i]>0) {min = i; break;}          
    for (i=255;i>=min;--i)
        if (C[i]>0) {max = i; break;}                    
    ulong prev=C[0], temp;
    C[0]=0;
    for (i=1;i<256;i++) {          
        temp = C[i];
        C[i]=C[i-1]+prev;
        prev = temp;
    }
    this->codetable = node::makecodetable(bwt,n);
    alphabetrank = new THuffAlphabetRank(bwt,n, this->codetable,0);   
    //if (alphabetrank->Test(bwt,n)) printf("alphabetrank ok\n");    
    delete [] bwt;

    // Make tables
    maketables();
    // to avoid running out of unsigned, the sizes are computed in specific order (large/small)*small
    // |class CSA| +256*|TCodeEntry|+|C[]|+|suffixes[]+positions[]|+...       
    //printf("FMindex takes %d B\n",
    //    6*W/8+256*3*W/8+256*W/8+ (2*n/(samplerate*8))*W+sampled->SpaceRequirementInBits()/8+alphabetrank->SpaceRequirementInBits()/8+W/8);
}

ulong CSA::lookup(ulong i) // Time complexity: O(samplerate log \sigma)
{
    ulong dist=0;
    while (!sampled->IsBitSet(i)) 
    {
        int c = alphabetrank->charAtPos(i);
        i = C[c]+alphabetrank->rank(c,i)-1; // LF-mapping  
        ++dist;
    }
    
    return suffixes[sampled->rank(i)-1]+dist;
}

ulong CSA::Psi(ulong i)   // Time complexity: O(samplerate log \sigma)
{
    // Return 0 if SA[i] = n
    if (lookup(i) == n - 1)
        return 0;

    // Search sampled position so that SA[j] less than or equal to SA[i]    
    int c;
    unsigned j = i;
    while (!sampled->IsBitSet(j)) 
    {
        c = alphabetrank->charAtPos(j);
        j = C[c]+alphabetrank->rank(c,j)-1; // LF-mapping  
    }

    // Move to j =  inverse SA[ SA[j] + samplerate ]
    // or to the end of the BWT
    if (suffixes[sampled->rank(j)-1] / samplerate + 1 >= n / samplerate)
        j = bwtEndPos;
    else
        j = positions[suffixes[sampled->rank(j)-1] / samplerate + 1];
    
    // Search position SA[prev] = SA[i] + 1
    unsigned prev;
    do
    {
        prev = j;
        c = alphabetrank->charAtPos(j);
        j = C[c]+alphabetrank->rank(c,j)-1; // LF-mapping  
    } while (j != i);

    // Return the previous j value prev
    return prev;
}


uchar * CSA::substring(ulong i, ulong l)
{
    uchar *result = new uchar[l + 1];
    if (l == 0)
    {
        result[0] = 0u;
        return result;
    }
      
    ulong dist;
    ulong k = i + l - 1;
    // Check for end of the string
    if (k > n - 1)
    {
        l -= k - n + 1;
        k = n - 1;
    }
    
    ulong skip = samplerate - k % samplerate - 1;
    ulong j;
    if (k / samplerate + 1 >= n / samplerate)
    {
        j = bwtEndPos;
        skip = n - k - 1;
    }
    else
    {
        j = positions[k/samplerate+1];
        //cout << samplerate << ' ' << j << '\n';	
    }    
    
    for (dist = 0; dist < skip + l; dist++) 
    {
        int c = alphabetrank->charAtPos(j);
        j = C[c]+alphabetrank->rank(c,j)-1; // LF-mapping
        if (dist >= skip)
            result[l + skip - dist - 1] = c;
    }
    result[l] = 0u;
    return result;
}

ulong CSA::inverse(ulong i)
{
    ulong skip = samplerate - i % samplerate;
    ulong j;
    if (i / samplerate + 1 >= n / samplerate)
    {
        j = bwtEndPos;
        skip = n - i;
    }
    else
    {
        j = positions[i/samplerate+1];
        //cout << samplerate << ' ' << j << '\n';   
    }    
    
    while (skip > 0)
    {
        int c = alphabetrank->charAtPos(j);
        j = C[c]+alphabetrank->rank(c,j)-1; // LF-mapping
        skip --;
    }
    return j;
}

ulong CSA::Search(uchar *pattern, ulong m, ulong *spResult, ulong *epResult) {
    // use the FM-search replacing function Occ(c,1,i) with alphabetrank->rank(c,i)
    int c = (int)pattern[m-1]; 
    int i=m-1;
    int sp = C[c];
    int ep = C[c+1]-1;
    while (sp<=ep && i>=1) 
    {
        c = (int)pattern[--i];
        sp = C[c]+alphabetrank->rank(c,sp-1);
        ep = C[c]+alphabetrank->rank(c,ep)-1;
    }
    *spResult = sp;
    *epResult = ep;
    if (sp<=ep)
        return ep - sp + 1;
    else
        return 0;
}

CSA::~CSA() {
    delete alphabetrank;       
    delete sampled;
    delete [] suffixes;
    delete [] positions;
    delete [] codetable;
}


void CSA::maketables()
{
    ulong sampleLength = (n%samplerate==0) ? n/samplerate : n/samplerate+1;
   
    ulong *sampledpositions = new ulong[n/W+1];
    suffixes = new ulong[sampleLength];   
    positions = new ulong[sampleLength];
   
    ulong i,j=0;
    for (i=0;i<n/W+1;i++)
        sampledpositions[i]=0lu;
    
    ulong x,p=bwtEndPos;
    ulong ulongmax = 0;
    ulongmax--;

   //positions:
    for (i=n-1;i<ulongmax;i--) { // TODO bad solution with ulongmax?
      // i substitutes SA->GetPos(i)
        x=(i==n-1)?0:i+1;

        if (x % samplerate == 0) {
            Tools::SetField(sampledpositions,1,p,1);
            positions[x/samplerate] = p;
//         printf("positions[%lu] = %lu\n", x/samplerate, p);
        }

        //p= wt->LFmapping(p+1)-1;
        uchar c = alphabetrank->charAtPos(p);
        p = C[c]+alphabetrank->rank(c, p)-1;
    }

//      printf("Sampled positions:\n0123456789012345678901234567890123456789\n");
//      Tools::PrintBitSequence(sampledpositions,n);
    sampled = new BitRank(sampledpositions,n,true);   
   
/*     printf("Is bit set test:\n");
    for (i = 0; i< n; i++)
    if ((*sampled)->IsBitSet(i))
    printf("1");
    else
    printf("0");
    printf("\n");*/
    
   //suffixes:   
    for(i=0; i<sampleLength; i++) {
        j = sampled->rank(positions[i]);
        if (j==0) j=sampleLength;
        suffixes[ j-1] = (i*samplerate==n)?0:i*samplerate;
//         printf("suffixes[%lu] = %lu\n", j-1, (i*samplerate==n)?0:i*samplerate);
    }   
}

uchar * CSA::LoadFromFile(const char *filename)
{
    uchar *s;
    std::ifstream file (filename, ios::in|ios::binary);
    if (file.is_open())
    {
        std::cerr << "Loading CSA from file: " << filename << std::endl;
        file.read((char *)&bwtEndPos, sizeof(ulong));
        s = new uchar[n];
        for (ulong offset = 0; offset < n; offset ++)
            file.read((char *)(s + offset), sizeof(char));
        file.close();
    }
    else 
    {
        std::cerr << "Unable to open file " << filename << std::endl;
        exit(1);
    }
    return s;
}

void CSA::SaveToFile(const char *filename, uchar *bwt)
{
    std::ofstream file (filename, ios::out|ios::binary|ios::trunc);
    if (file.is_open())
    {
        std::cerr << "Writing CSA to file: " << filename << std::endl;
        file.write((char *)&bwtEndPos, sizeof(ulong));
        std::cerr << "Writing BWT of " << n << " bytes." << std::endl;
        for (ulong offset = 0; offset < n; offset ++)
            file.write((char *)(bwt + offset), sizeof(char));
        file.close();
    }
    else 
    {
        std::cerr << "Unable to open file " << filename << std::endl;
        exit(1);
    }
}


uchar * CSA::BWT(uchar *text) 
{
    uchar *s;

    DynFMI *wt = new DynFMI((uchar *) text, n);
    s = wt->getBWT();
    for (ulong i=0;i<n;i++) 
        if (s[i]==0u) {
            bwtEndPos = i;  // TODO: better solution ?
            i=n;
        }

    delete wt;
    return s;
}



CSA::TCodeEntry * CSA::node::makecodetable(uchar *text, ulong n)
{
    TCodeEntry *result = new TCodeEntry[ 256 ];
    
    count_chars( text, n, result );
    std::priority_queue< node, std::vector< node >, std::greater<node> > q;
//
// First I push all the leaf nodes into the queue
//
    for ( unsigned int i = 0 ; i < 256 ; i++ )
        if ( result[ i ].count )
            q.push(node( i, result[ i ].count ) );
//
// This loop removes the two smallest nodes from the
// queue.  It creates a new internal node that has
// those two nodes as children. The new internal node
// is then inserted into the priority queue.  When there
// is only one node in the priority queue, the tree
// is complete.
//

    while ( q.size() > 1 ) {
        node *child0 = new node( q.top() );
        q.pop();
        node *child1 = new node( q.top() );
        q.pop();
        q.push( node( child0, child1 ) );
    }
//
// Now I compute and return the codetable
//
    q.top().maketable(0u,0u, result);
    q.pop();
    return result;
}


void CSA::node::maketable(unsigned code, unsigned bits, TCodeEntry *codetable) const
{
    if ( child0 ) 
    {
        child0->maketable( SetBit(code,bits,0), bits+1, codetable );
        child1->maketable( SetBit(code,bits,1), bits+1, codetable );
        delete child0;
        delete child1;
    } 
    else 
    {
        codetable[value].code = code;    
        codetable[value].bits = bits;
    }
}

void CSA::node::count_chars(uchar *text, ulong n, TCodeEntry *counts )
{
    ulong i;
    for (i = 0 ; i < 256 ; i++ )
        counts[ i ].count = 0;
    for (i=0; i<n; i++)
        counts[(int)text[i]].count++; 
}

unsigned CSA::node::SetBit(unsigned x, unsigned pos, unsigned bit) {
      return x | (bit << pos);
}

