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


// this is a red-black tree implementation by wolfgang Gerlach based on the algorithm provided by 
// Cormen et al.: Introduction to Algorithms, Second Edition. MIT Press and McGraw-Hill, 2001

using namespace std;

#ifndef RBTree
#define RBTree RBTree

 


enum RBNodecolor{BLACK,RED};


// generic Red-Black Tree Node:
class RBNode
{
	public:
	RBNode* parent;
	RBNode* left;
	RBNode* right;
	
	enum RBNodecolor color;

	RBNode(){};

	RBNode(RBNode *n)
		: parent(n), left(n), right(n){
		color=RED;
	}


	virtual ~RBNode(){}

	void countBlack(int i){
		if (this->color == BLACK) i++;
		if (this->left != this) this->left->countBlack(i);
			else cout << i << ",";
		if (this->right != this) this->right->countBlack(i);
			else cout << i << ",";
	}
};

class RBTree{
	public:

	RBNode *root;
	RBNode *nil;
	

	RBTree(){
		nil = new RBNode();
		nil->parent=nil;
		nil->left=nil;
		nil->right=nil;
		nil->color = BLACK;
		root=nil;
	}

	virtual ~RBTree(){
		deleteNode(root);
		delete this->nil;
	}

	void checkTree();

	void rbInsertFixup(RBNode* z, void (*updateNode)(RBNode* n, RBTree *T));
	void rbDeleteFixup(RBNode *x, void (*updateNode)(RBNode* n, RBTree *T));
	void rbDelete(RBNode *z, void (*updateNode)(RBNode* n, RBTree *T));
	RBNode* findRightSiblingLeaf(RBNode *n);
	RBNode* findLeftSiblingLeaf(RBNode *n);
	RBNode* treeSuccessor(RBNode *x);
	RBNode* treePredeccessor(RBNode *x);
	RBNode* treeMinimum(RBNode *x);
	RBNode* treeMaximum(RBNode *x);
	
	bool isLeftChild(RBNode *n);
	bool isRightChild(RBNode *n);
	
	int getNodeMaxDepth(RBNode *n);
	int getNodeMinDepth(RBNode *n);
	
	void printSubTree(RBNode *n);
	void checkSubTree(RBNode *n);
	void checkNode(RBNode *x);

	void deleteNode(RBNode* x){
		if (x->left!=nil) deleteNode(x->left);
		if (x->right!=nil) deleteNode(x->right);
		delete x;
	}


	private:
	void leftRotate(RBNode* x, void (*updateNode)(RBNode* n, RBTree *T));
	void rightRotate(RBNode* x, void (*updateNode)(RBNode* n, RBTree *T));

};

#endif
