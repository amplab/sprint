/***************************************************************************
 *   Copyright (C) 2006 by Wolfgang Gerlach   *
 *      *
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



#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stack>
#include <queue>
#include <functional>
#include <algorithm>

#include "wtreebwt.h"






// -------- DynFMI --------

DynFMI::~DynFMI(){
	deleteDynFMINodes(root);
	delete[] leaves; //free(leaves) // ???;
}

void DynFMI::deleteDynFMINodes(WaveletNode *n){
	if (n->right) deleteDynFMINodes(n->right);
	if (n->left) deleteDynFMINodes(n->left);
	
	delete n;
}

DynFMI::DynFMI(uchar *text, ulong n){
	initEmptyDynFMI(text);
	addText(text, n);
}


void DynFMI::iterateReset(){
	iterate = 1;
	recursiveIterateResetWaveletNode(root);
}

void DynFMI::recursiveIterateResetWaveletNode(WaveletNode *w){
	w->bittree->iterateReset();
	
	if (w->left) recursiveIterateResetWaveletNode(w->left);
	if (w->right) recursiveIterateResetWaveletNode(w->right);
}


bool DynFMI::iterateNext(){
	iterate++;
	return !(iterate > getSize());
}

uchar DynFMI::iterateGetSymbol(){

	ulong i = iterate;
	WaveletNode *walk = root;	
	bool bit;
		
	while (true) {
		
		bit = walk->bittree->iterateGetBit(); // TODO improve
		i=walk->bittree->iterateGetRank(bit);
		
		walk->bittree->iterateNext();
		
		
		if (bit) { //bit = 1
			if (walk->right == 0) return walk->c1;
			walk=walk->right;
		} else { // bit = 0
			if (walk->left == 0) return walk->c0;
			walk=walk->left;
		}
		
		
	} // end of while
	
}


uchar* DynFMI::getBWT(){
	ulong n = root->bittree->getPositions();
	
	uchar *text = new uchar[n];
	
	bool data=true;
	// old slow version:
	//for (ulong i=1; i <= root->bittree->getPositions(); i++)
	//	text[i-1]=(*this)[i];
	
	ulong i = 0;
	
	iterateReset();
	
	while (data) {
		text[i] = iterateGetSymbol();	
		data = iterateNext();
		i++;
	}
	
	
	
	return text;
}

void DynFMI::deleteLeaves(WaveletNode *node){
	bool leaf = true;

	if (node->left) {
		// internal node
		leaf = false;
		deleteLeaves(node->left);
		
	}
	if (node->right){
		leaf = false;
		deleteLeaves(node->right);
	} 
	
	if (leaf) {
		// is a leaf, delete it!
		if (node->parent) {
			if (node==node->parent->left) node->parent->left=0;
				else node->parent->right=0;
		}
		delete node;
	}
}

void DynFMI::makeCodes(ulong code, int bits, WaveletNode *node){
	#ifndef NDEBUG
	if (node == node->left) {
		cout << "makeCodes: autsch" << endl;
		exit(0);
		}
	#endif

	if (node->left) {
		makeCodes(code | (0 << bits), bits+1, node->left);
		makeCodes(code | (1 << bits), bits+1, node->right);
	} else {
		codes[node->c0] = code;
		codelengths[node->c0] = bits+1;
	}
}

void DynFMI::appendBVTrees(WaveletNode *node){
	node->bittree = new BVTree();

	if (node->left) appendBVTrees(node->left);
	if (node->right) appendBVTrees(node->right);
}

void DynFMI::initEmptyDynFMI(uchar *text){
	// pointers to the leaves for select
	leaves = (WaveletNode**) new WaveletNode*[256];
	for(int j=0; j<256; j++) leaves[j]=0;


	ulong i=0;
	while (text[i]!='\0') {
		
		if (leaves[text[i]]==0) {
			leaves[text[i]] = new WaveletNode(text[i]); 
		}
		leaves[text[i]]->weight++; 
		i++;
	}
	
	// separation symbol:
	leaves[0] = new WaveletNode((uchar)0); 
	leaves[0]->weight=1;
	
	// Veli's approach:
	priority_queue< WaveletNode*, vector<WaveletNode*>, greater<WaveletNode*> > q;
	
	
	for(int j=0; j<256; j++){
		if (leaves[j]!=0) {
			q.push( (leaves[j]) );
		}
		codes[j] = 0;
		codelengths[j] = 0;
	}
	
	// creates huffman shape:
	while (q.size() > 1) {
		
		WaveletNode *left = q.top();
		q.pop();
		
		WaveletNode *right = q.top();
		q.pop();
		
		q.push(  new WaveletNode(left, right) );
	}	
	

	root = q.top();
	q.pop();
	
			
	makeCodes(0,0, root);	// writes codes and codelengths

	
	// merge leaves	(one leaf represent two characters!)
	for(int j=0; j<256; j++){
	
		if (leaves[j]) {
		
			if (leaves[j]->parent->left==leaves[j]) {
				leaves[j]->parent->c0=j;
			} else {
				leaves[j]->parent->c1=j;
			}
			leaves[j]=leaves[j]->parent; // merge
		}
	}

	
	deleteLeaves(root);
	
	appendBVTrees(root);
	
	// array C needed for backwards search
	for(int j=0; j<256+256; j++) C[j] = 0;
	
	
}


void DynFMI::insert(uchar c, ulong i){
	#ifndef NDEBUG
	if (leaves[c]==0) {
		cerr << "error: Symbol \"" << c << "\" (" << (int)c << ") is not in the code table!" << endl;;
		exit(EXIT_FAILURE);
	}
	#endif
	
	ulong level = 0;
	ulong code = codes[c];

	bool bit;
	WaveletNode *walk = root;	
		
	while (walk) {
		
		bit = ((code & (1u << level)) != 0); 
		
		walk->bittree->insertBit(bit,i); // TODO improve
		i=walk->bittree->rank(bit, i);

		if (bit) { //bit = 1
			walk=walk->right;
		} else { // bit = 0
			walk=walk->left;
		}
		
		level++;		
	} // end of while
	
	int j = 256+c;
	while(j>1) {
		C[j]++;
		j=binaryTree_parent(j);
		}
	C[j]++;	
	
}


uchar DynFMI::operator[](ulong i){
	WaveletNode *walk = root;	
	bool bit;
		
	while (true) {
		
		bit = (*walk->bittree)[i]; //TODO improve by reducing
		i=walk->bittree->rank(bit, i);

		if (bit) { //bit = 1
			if (walk->right == 0) return walk->c1;
			walk=walk->right;
		} else { // bit = 0
			if (walk->left == 0) return walk->c0;
			walk=walk->left;
		}
		
		
	} // end of while
	cout << endl;
    return 0;
}

ulong DynFMI::rank(uchar c, ulong i){
	if (i==0) return 0;

	ulong level = 0;
	ulong code = codes[c];
	
	
	bool bit;
	WaveletNode *walk = root;	
		
	while (true) {
		
		bit = ((code & (1u << level)) != 0);
		
		i=walk->bittree->rank(bit, i);
		if (bit) { //bit = 1
			if (walk->right == 0) return i;
			walk=walk->right;
		} else { // bit = 0
			if (walk->left == 0) return i;
			walk=walk->left;
		}
	
		level++;		
	} // end of while
	
	cerr << "error: DynFMI::rank: left while loop" << endl;
	exit(EXIT_FAILURE);
	return 0; //never
}

ulong DynFMI::select(uchar c, ulong i){
	
	WaveletNode *walk = leaves[c];	
	
	bool bit = (walk->c1==c);
	
	while (walk->parent) {
		i=walk->bittree->select(bit, i);
		
		bit = (walk == walk->parent->right);
		walk=walk->parent;
	} // end of while
	
	i=walk->bittree->select(bit, i);

	return i;
}



// size must include endmarker!
void DynFMI::addText(uchar *str, ulong n){
	ulong i=1;
	
	
	
	

	insert(str[n-2],i); // insert second last character, corresponds to suffix of length 1

	for (ulong t=n-2; t > 0; t--) {
		i= 1+getNumberOfSymbolsSmallerThan(str[t]) + rank(str[t],i);
		insert(str[t-1],i);
	}
	

	i= 1+ getNumberOfSymbolsSmallerThan(str[0]) + rank(str[0],i);
	insert(str[n-1],i);

}



ulong DynFMI::getNumberOfSymbolsSmallerThan(uchar c){
	int j = 256+c;
	ulong r=0;
	while(j>1) {
		if (binaryTree_isRightChild(j)) 
			r += C[binaryTree_left(binaryTree_parent(j))];
		
		j=binaryTree_parent(j);
	}
	return r;
}




void DynFMI::printDynFMIContent(ostream& stream){
	uchar c;
	for (ulong i=1; i<=getSize(); i++) 
	{
		c =(*this)[i];
		if (c==0) c= '#';
		stream << c;
	}
}




