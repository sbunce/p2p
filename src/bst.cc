//std
#include <iostream>
#include <cmath>
#include <string>

#include "bst.h"

bst::bst()
{
	root = 0;
	treeSize = 0;
}

bst::~bst()
{
	destroy();
}

void bst::addNode(bst::treeNode * newNode)
{
	addNode(root, newNode);
}

void bst::addNode(bst::treeNode *& root, bst::treeNode * newNode)
{
	if(root==0){
		root = newNode;
		root->left = root->right = 0;
		treeSize++;
	}
	else if(root->key == newNode->key){
#ifdef DEBUG
		std::cout << "info: binarySearchTree::addNode() attempted to add node that already exists, key: " << newNode->key << std::endl;
#endif
		//delete duplicate data
		delete newNode;
	}
	else if(root->key > newNode->key){
		addNode(root->left, newNode);
	}
	else{
		addNode(root->right, newNode);
	}
}

void bst::deleteNode(int key)
{
	deleteNode(findNode(root, key));
}

void bst::deleteNode(bst::treeNode *& node)
{
	if(node->left == 0){
		//delete node and replace with right child
		bst::treeNode * temp = node;
		node = node->right;
		delete temp;
		treeSize--;
	}
	else if(node->right == 0){
		//delete node and replace with left child
		bst::treeNode * temp = node;
		node = node->left;
		delete temp;
		treeSize--;
	}
	else{
		//swap node with it's in order successor then delete node
		bst::treeNode *& successor = inOrderSuccessor(node->right, node->key);

		//store the node pointers(to be restored after swap)
		bst::treeNode * left = node->left;
		bst::treeNode * right = node->right;

		//swap
		bst::treeNode * nodeTemp = node;
		node = successor;
		successor = nodeTemp;

		//restore node pointers to node(one not being deleted)
		node->left = left;
		node->right = right;

		//delete
		deleteNode(successor);
	}
}

bst::treeNode *& bst::inOrderSuccessor(bst::treeNode *& node, int key)
{
	if(node != 0){
		if(node->left == 0){
			return node;
		}

		if(key < node->key){
			return findNode(node->left, key);
		}

		if(key > node->key){
			return findNode(node->right, key);
		}
	}
}

void bst::destroy()
{
	destroy(root);
}

void bst::destroy(bst::treeNode *& node)
{
	if(node != 0){
		destroy(node->left);
		destroy(node->right);
		delete node;
		node = 0;
	}
}

bool bst::empty()
{
	if(root == 0){
		return true;
	}
	else{
		return false;
	}
}

std::string & bst::getData(int key)
{
	bst::treeNode * temp = findNode(root, key);
	return temp->data;
}

bst::treeNode *& bst::findNode(bst::treeNode *& node, int key)
{
	if(node != 0){
		if(node->key == key){
			return node;
		}

		if(key < node->key){
			return findNode(node->left, key);
		}

		if(key > node->key){
			return findNode(node->right, key);
		}
	}
}

void bst::findMissing(std::vector<int> & request, int lowestRequest, int highestRequest)
{
	std::vector<int> ordered;

	if(root != 0){ //make sure this is not getting called with an empty tree
		findMissing(root, root->key, ordered);
	}

	/*
	This algorithm creates an array where each element position corresponds to a key 
	number. The array is initialized to false, true is put in every position that 
	corresponds to a key in the tree. Positions remaining false indicate missing
	keys.

	WARNING!
	This will segfault if any keys are let in to the tree that are outside the range 
	of lowestRequest to highestRequest!
	*/

	//if highestRequest - lowestRequest == 0 then it's the first run and no need to check
	if(highestRequest - lowestRequest != 0){
		bool blockNumber[highestRequest - lowestRequest];
		for(int x=0; x<=highestRequest - lowestRequest; x++){
			blockNumber[x] = false;
		}

		for(int x=0; x<ordered.size(); x++){
			blockNumber[ordered.at(x) - lowestRequest] = true;
		}

		for(int x=0; x<=highestRequest - lowestRequest; x++){
			if(blockNumber[x] == false){
				request.push_back(x + lowestRequest);
			}
		}
	}
}

void bst::findMissing(bst::treeNode * node, int previous, std::vector<int> & ordered)
{
	if(node != 0){
		findMissing(node->left, node->key, ordered);
		ordered.push_back(node->key);
		findMissing(node->right, node->key, ordered);
	}
}

std::string & bst::getLowestData()
{
	bool foundNode = false;
	return getLowestData(root, foundNode);
}

std::string & bst::getLowestData(bst::treeNode *& node, bool & foundNode)
{
	if(node != 0 && !foundNode){
		if(node->left == 0){
			foundNode = true;
			lastLowest = &node;
			return node->data;
		}

		return getLowestData(node->left, foundNode);
	}
}

int bst::getLowestKey()
{
	bool foundNode = false;
	return getLowestKey(root, foundNode);
}

int bst::getLowestKey(bst::treeNode *& node, bool & foundNode)
{
	if(node != 0 && !foundNode){
		if(node->left == 0){
			foundNode = true;
			lastLowest = &node;
			return node->key;
		}

		return getLowestKey(node->left, foundNode);
	}
}

int bst::getHighestKey()
{
	bool foundNode = false;
	return getHighestKey(root, foundNode);
}

int bst::getHighestKey(bst::treeNode *& node, bool & foundNode)
{
	if(node != 0 && !foundNode){
		if(node->right == 0){
			foundNode = true;
			lastLowest = &node;
			return node->key;
		}

		return getHighestKey(node->right, foundNode);
	}
}

void bst::deleteLastLowest()
{
	deleteNode(*lastLowest);
}

//used for testing the tree
void bst::inOrder()
{
	inOrder(root);
}

//used for testing the tree
void bst::inOrder(bst::treeNode * node)
{
	if(node != 0){
		inOrder(node->left);
		std::cout << "inOrder: " << node->key << " - " << node->data << std::endl;
		inOrder(node->right);
	}
}

int bst::size()
{
	return treeSize;
}
