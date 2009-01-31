#ifndef H_NODE
#define H_NODE

class node
{
public:
	node(const int & ID_in);

	/*
	get_ID - returns ID of the node
	*/
	int get_ID();

private:
	int ID;
};
#endif
