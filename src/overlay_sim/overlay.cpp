#include "overlay.hpp"

overlay::overlay()
{
	main_thread_terminate = false;
	main_thread = boost::thread(boost::bind(&overlay::process, this));
}

overlay::~overlay()
{
	main_thread_terminate = true;
std::cout << "set term\n";
	main_thread.join();
std::cout << "joined\n";

	std::vector<node *>::iterator iter_cur, iter_end;
	iter_cur = Pool.begin();
	iter_end = Pool.end();
	while(iter_cur != iter_end){
		delete *iter_cur;
		++iter_cur;
	}
}

void overlay::join(node * Node)
{
	//use ptr to ptr to be able to detect when Node leaves overlay
	std::cout << Node->get_ID() << "\n";

	//add node to the pool
	Pool.push_back(Node);
}

void overlay::output(std::string file_name)
{
	std::cout << "digraph G {\n";


	std::cout << "}\n";
}

void overlay::process()
{
	while(!main_thread_terminate){

		std::cout << "POIK\n";
	}
	std::cout << "DONE\n";
}
