

// Implements operations over a sequence of balanced parentheses

#ifndef PARENTHESESINCLUDED
#define PARENTHESESINCLUDED
/*
#include "basics.h"
#include "bitmap.h"
#include "hash.h"

typedef struct sparentheses
   { uint *data;  	// string
     bitmap bdata;   	// bitmap of string BitRank br;
     uint n;    	// # of parentheses 
     uint sbits;    	// bits for near pointers
     hash sftable;	// table of far forward pointers
     hash sbtable;	// table of far backward pointers
     hash bftable;	// table of near forward pointers
     hash bbtable;	// table of near backward pointers
   } *parentheses;

	// creates a parentheses structure from a bitstring, which gets owned
        // n is the total number of parentheses, opening + closing
	// bwd says if you will want to perform findopen and enclose
parentheses createParentheses (uint *string, uint n, bool bwd);
	// frees parentheses structure, including the bitstream
void destroyParentheses (parentheses P);
	// the position of the closing parenthesis corresponding to (opening)
	// parenthesis at position i
uint findclose (parentheses P, uint i);
	// respectively, for closing parenthesis i
uint findopen (parentheses P, uint i);
	// # open - # close at position i, not included
uint excess (parentheses P, uint i);
	// open position of closest parentheses pair that contains the pair
	// that opens at i, ~0 if no parent
uint enclose (parentheses P, uint i);

uint sizeofParentheses(parentheses P);*/
#include <stack>
#include "Tools.h"
#include "BitRank.h"
#include "Hash.h"
#define bitget1(e,p) ((e)[(p)/W] & (1lu<<((p)%W)))
class Parentheses
{
private:
    ulong *bp;
    BitRank *br;
    ulong n, near, far, pnear, pfar;
    unsigned sbits;
    Hash *sftable;
    Hash *sbtable;
    Hash *bftable;
    Hash *bbtable;
    void calcsizes();
    void filltables (bool);
    void fcompchar (unsigned char, unsigned char*, char*);
    void bcompchar (unsigned char, unsigned char* );
public:
    Parentheses(ulong* bp, ulong n, bool bwd, BitRank *);
    ~Parentheses();
    ulong findopen(ulong);
    ulong findclose(ulong);
    ulong excess(ulong);
    ulong enclose(ulong);
    ulong findparent(ulong);
    ulong isOpen(ulong);
};
#endif
