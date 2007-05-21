//this is a binary search tree
#ifndef H_BST
#define H_BST

//std
#include <string>
#include <vector>

#include "globals.h"

class bst
{
public:
	//the data type to pass to the tree
	class treeNode
	{
	public:
		int key;
		std::string data;
		treeNode * left;
		treeNode * right;
	};

	bst();
	~bst();
	/*
	addNode          - function for adding a node, wrapper for recursive function
	getByKey         - check if node is present by key, wrapper for recursive function checkKey
	destroy          - deletes all nodes in tree, wrapper for recursive function
	                 - must be called before getting rid of this class(also automatically called by destructor)
	findMissing      - finds fileBlocks missing from the tree
	getLowestData    - return data of node with lowest key, wrapper for recursive function
	getLowestKey     - return key of lowest node, wrapper for recursive function
	getHighestKey    - return key of highest node, wrapper for recursive function
	empty            - returns true if the tree is empty
	deleteLastLowest - delete the lowest node found by the last run of getLowest()
	                 - needed so caller can determine when to delete the string getLowest returns reference to
	inOrder          - in-order traversal, wrapper for recursive function
	size             - returns the number of nodes in the tree
	*/
	void addNode(bst::treeNode * newNode);
	std::string & getData(int key);
	void deleteNode(int key);
	void destroy();
	bool empty();
	void findMissing(std::vector<int> & request, int lowestRequest, int highestRequest);
	std::string & getLowestData();
	int getLowestKey();
	int getHighestKey();
	void deleteLastLowest();
	void inOrder();
	int size();

private:
	bst::treeNode * root; //root of the tree
	int treeSize;    //number of nodes in tree

	//used by deleteLastLowest to delete the last node found by getLowest
	bst::treeNode ** lastLowest;

	/*
	destroy          - recursive function for destroying entire tree
	deleteNode       - recursive function for deleting a node
	inOrderSuccessor - checks for a node to swap in delete operation
	findNode         - recursive function to find one node
	findMissing      - recursive function to find all missing nodes
	getLowestData    - recursive function to find the node with the lowest key
	getLowestKey     - recursive function to find the lowest key
	getHighestKey    - recursive function to find the highest key
	inOrder          - recursive function for traversing in-Order
	*/
	void addNode(bst::treeNode *& root, bst::treeNode * newNode);
	void destroy(bst::treeNode *& node);
	void deleteNode(bst::treeNode *& node);
	bst::treeNode *& inOrderSuccessor(bst::treeNode *& node, int key);
	bst::treeNode *& findNode(bst::treeNode *& node, int key);
	/*
	WARNING!
	findMissing() will segfault if any keys are let in to the tree that are outside the range 
	of lowestRequest to highestRequest!
	*/
	void findMissing(bst::treeNode * node, int previous, std::vector<int> & request);
	std::string & getLowestData(bst::treeNode *& node, bool & foundNode);
	int getLowestKey(bst::treeNode *& node, bool & foundNode);
	int getHighestKey(bst::treeNode *& node, bool & foundNode);
	void inOrder(bst::treeNode * node);
};
#endif
