#ifndef H_OVERLAY
#define H_OVERLAY

//boost
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

//custom
#include "node.h"

//std
#include <iostream>
#include <string>
#include <vector>

class overlay
{
public:
	overlay();
	~overlay();

	/*
	join    - join node to overlay
	process - give processing time to nodes
	*/
	void join(node * Node);
	void process();

	/*
	ouput - outputs a dot file with a specified name (exits program if file exists)
	*/
	void output(std::string file_name);

private:

	//all nodes which have been join'd
	std::vector<node *> Pool;

	boost::thread main_thread;
	bool main_thread_terminate;
};
#endif
