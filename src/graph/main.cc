//dot -Tps test.dot -o test.ps

#include <fstream>
#include <iostream>
#include <list>
#include <string>

//trims leading spaces
void trim_begin(std::string & buff)
{
	//front
	while(!buff.empty() && *buff.begin() == ' '){
		buff.erase(buff.begin());
	}
}

void process_target(std::string & target)
{
	//retrieve object name, and erase from target
	std::string object = target.substr(0, target.find(".o:", 0));
	target.erase(0, object.size()+4);

	//tokenize dependencies
	std::list<std::string> deps;
	std::string::iterator iter = target.begin();
	while(iter != target.end()){
		if(*iter == ' '){
			deps.push_back(std::string(target.begin(), iter));
			iter = target.erase(target.begin(), iter+1);
		}else{
			++iter;
		}
	}

	//remove elements that are *.cc
	std::list<std::string>::iterator iter_cur, iter_end;
	iter_cur = deps.begin();
	iter_end = deps.end();
	while(iter_cur != iter_end){
		int pos;
		if((pos = iter_cur->find(".h", 0)) == std::string::npos){
			iter_cur = deps.erase(iter_cur);
		}else{
			iter_cur->erase(iter_cur->end()-2, iter_cur->end());
			if(object == *iter_cur){
				//objects don't depend on themselves
				iter_cur = deps.erase(iter_cur);
			}else{
				++iter_cur;
			}
		}
	}

	//output
	iter_cur = deps.begin();
	iter_end = deps.end();
	while(iter_cur != iter_end){
		std::cout << "\t" << object << " -> " << *iter_cur << ";\n";
		++iter_cur;
	}

}

int main(int argc, char * argv[])
{
	std::cout << "digraph G {\n";

	std::string buff;     //one line of the file
	std::string one_line; //entire target (compensating for \ to join lines)
	while(std::getline(std::cin, buff)){

		if(buff.find(".o:", 0) != std::string::npos){
			//on new target
			one_line.clear();
		}

		trim_begin(buff);

		if(*buff.rbegin() == '\\'){
			//there is another line for the target
			buff.erase(buff.end()-1);
			trim_begin(buff);
			one_line += buff;
		}else{
			//last line of the target
			if(!one_line.empty()){
				process_target(one_line);
			}
		}
	}

	std::cout << "}\n";

	return 0;
}
