//std
#include <iostream>
#include <string>
#include <vector>

#include "bst.h"

bst * Tree = new bst;

void addNode(int key)
{
	bst::treeNode * temp = new bst::treeNode;
	temp->key = key;
	temp->data = "BLARG";

	Tree->addNode(temp);
}

int main(int argc, char * argv[])
{
	std::string input;
	int key;

	while(true){

		Tree->inOrder();

		std::cout << "Input: ";
		getline(std::cin, input);
		key = atoi(input.c_str());

		if(input == "L"){
			std::cout << "lowest: " << Tree->getLowestKey() << std::endl;
			continue;
		}

		if(input == "DLL"){
			Tree->deleteLastLowest();
			continue;
		}

		if(input == "d"){
			std::cout << "Key to delete: ";
			getline(std::cin, input);
			key = atoi(input.c_str());
			Tree->deleteNode(key);
			continue;
		}

		if(input == "FM"){
			std::vector<int> myVec;
			Tree->findMissing(myVec, 50, 60);

			for(int x=0; x<myVec.size(); x++){
				std::cout << "missing: " << myVec.at(x) << std::endl;
			}

			myVec.clear();

			continue;
		}

		if(input == "D"){
			Tree->destroy();
			continue;
		}

		if(input.empty()){
			std::cout << "No Input!" << std::endl;
			continue;
		}
		else{
			addNode(key);
		}
	}

	delete Tree;

	return 0;
}
