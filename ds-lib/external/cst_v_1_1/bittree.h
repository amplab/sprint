/***************************************************************************
 *   Copyright (C) 2006 by Wolfgang Gerlach   *
 *   No object matches key 'wgerlach'.   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// Implementation of the Dynamic Bit Vector with Indels problem
// space: O(nH_0) time: O(H_0)
// papers: V. Maekinen, G. Navarro. Dynamic Entropy-Compressed Sequences and Full-Text
//           Indexes. CPM 2006, Chapter 3.4 Dynamic Structures for Bit Vectors
//   also: W.-L. Chan, W.-K. Hon, and T.-W. Lam. Compressed index for a dynamic collection
//           of texts. In Proc. CPM04, LNCS 3109, pages 445-456, 2004 

#ifndef BVTree
#define BVTree BVTree

#include <iostream>
#include <bitset>
#include <cstdlib>
#include <map>
#include <stack>
#include <cmath>
#include <fstream>
#include <vector>
#include <cstdio>

#include "rbtree.h"
#include "Tools.h"

#ifndef uchar
#define uchar unsigned char
#endif
#ifndef ulong
#define ulong unsigned long
#endif

class BVNode;
class BVTree;

const int logn = (W * 2);

void callUpdateCounters(RBNode *n, RBTree *T);
void callUpdateCountersOnPathToRoot(RBNode *n, RBTree *T);


class BVTree : public RBTree{
public:

  //Constructors
  BVTree(){
  	tempnil = (BVNode*) ((RBTree*) this)->nil;
  	tempbit = new bitset<2*logn>;
  }

  //Destructor:
  ~BVTree();

  bool operator[](ulong);


  // inserts bit at position i, countings begins with 1:
  void appendBit(bool bit);
  void insertBit(bool bit, ulong i);
  void deleteBit(ulong i);

  ulong rank0(ulong i);
  ulong rank1(ulong i);
  ulong rank(bool b, ulong i){return b?rank1(i):rank0(i);}
  
  ulong select0(ulong i);
  ulong select1(ulong i);
  ulong select(bool b, ulong i){return b?select1(i):select0(i);}

	void setRoot(BVNode* n){
		((RBTree*) this)->root=(RBNode*)n;
	}
	
	BVNode* getRoot(){
		return ((BVNode*) ((RBTree*) this)->root);
	}

	void setNil(BVNode* n){
		tempnil = n;
		((RBTree*) this)->nil=(RBNode*)n;
	}

	BVNode* getNil(){
		return tempnil;
		
	}

  // write bits back into a stream:  
  ulong* getBits();
  void writeTree(char *writefile); 
  void writeTree(std::ostream& stream); //e.g. stream = cout

  int getTreeMaxDepth();
  int getTreeMinDepth();
  ulong getPositions();
  ulong getRank();
  
  void iterateReset();
  bool iterateGetBit();
  bool iterateNext();
  ulong iterateGetRank(bool bit);
  
  bool getLastBitDeleted(){return lastBitDeleted;}
  ulong getLastRank(){return lastRank;}
  
  void checkSubTree(BVNode *n);

  void updateCounters(BVNode *n);
  void updateCountersOnPathToRoot(BVNode *walk);
  
  //debug:
  void printNode(ulong i);

protected:

  ulong iterate;
  ulong iterateLocal;
  ulong iterateRank;

  BVNode *iterateNode;

  BVNode *tempnil;

  bool lastBitDeleted;
  ulong lastRank;
  

  bitset<2*logn> *tempbit;

  // content of BVNode, for debugging:
  void printNode(BVNode *n);
  
  // other operations:
  ulong getLocalRank(BVNode* n, ulong position);
  ulong getLocalSelect0(BVNode* n, ulong query);
  ulong getLocalSelect1(BVNode* n, ulong query);
  
  void deleteNode(BVNode *n);
  void deleteLeaf(BVNode *leaf);
};

class BVNode : public RBNode
{
	public:
	ulong myPositions;
	ulong myRank;
	ulong subTreePositions; //number of positions stored in the subtree rooted at this node
	ulong subTreeRank;      //number of bits set in the subtree rooted at this node

	bitset<2*logn> *block;
	


	BVNode(BVNode* n)
		: RBNode(n), myPositions(0), myRank(0), subTreePositions(0), subTreeRank(0), block(0) {
	}

	~BVNode(){
		delete block;
	}

		
	BVNode* getParent(){
		return ((BVNode*) ((RBNode*) this)->parent);
	}

	BVNode* getLeft(){
		return ((BVNode*) ((RBNode*) this)->left);
	}

	BVNode* getRight(){
		return ((BVNode*) ((RBNode*) this)->right);
	}

	void setParent(BVNode* n){
		((RBNode*) this)->parent=(RBNode*)n;
	}

	void setLeft(BVNode* n){
		((RBNode*) this)->left=(RBNode*)n;
	}

	void setRight(BVNode* n){
		((RBNode*) this)->right=(RBNode*)n;
	}
		
};

#endif

