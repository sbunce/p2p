#include "scan.hpp"

scan::scan()
{
	//start of dot file
	std::cout << "digraph G {\n";
}

scan::~scan()
{
	//end of dot file
	std::cout << "}\n";
}

void scan::eat_comment(std::fstream & fin)
{
	//this function is called after a forward slash is read from fin
	char ch;
	if(fin.get(ch)){
		if(ch == '/'){
			//start of line comment, read until \n
			while(fin.get(ch)){
				if(ch == '\n'){
					return;
				}
			}
		}else if(ch == '*'){
			//start of block comment, read until "*/"
			bool dot(false);
			while(fin.get(ch)){
				if(ch == '*'){
					dot = true;
				}else if(ch == '/' && dot){
					return;
				}else{
					dot = false;
				}
			}
		}
	}
}

void scan::read_preprocessor(std::fstream & fin, std::string & root_file, std::set<std::string> & scan)
{
	//check to see if preprocessor directive is "include"
	const char * inc = "include";
	char ch;
	for(int x=0; x<7; ++x){
		if(fin.get(ch)){
			if(ch != inc[x]){
				return;
			}
		}
	}

	if(!fin.get(ch)){
		return;
	}

	//may be a space between #include and "header.h"
	if(ch == ' '){
		//space, read again
		if(!fin.get(ch)){
			return;
		}
	}

	//only read includes that begin with '"' (exclude system headers)
	if(ch == '"'){
		std::string name;
		while(fin.get(ch)){
			if(ch == '"'){ //read until closing quote found
				//echo dot file stuff
				std::cout << "\"" << root_file << "\" -> \"" << name << "\";\n";

				//queue file to be scanned
				scan.insert(name);
				return;
			}else{
				name += ch;
			}
		}
	}
}

void scan::run(std::string root_file)
{
	//do not scan files which have already been scanned (memoize)
	std::set<std::string>::iterator iter = scanned_files.find(root_file);
	if(iter != scanned_files.end()){
		//file already scanned
		return;
	}else{
		scanned_files.insert(root_file);
	}

	std::fstream fin(root_file.c_str(), std::ios::in);
	if(!fin.is_open()){
		std::cout << "can not open root_file " << root_file << "\n";
		return;
	}

	//holds files that need to be scanned
	std::set<std::string> scan;

	char ch;
	while(fin.get(ch)){
		if(ch == '/'){
			eat_comment(fin);
		}else if(ch == '#'){
			read_preprocessor(fin, root_file, scan);
		}
	}
	fin.close();

	std::set<std::string>::iterator iter_cur, iter_end;
	iter_cur = scan.begin();
	iter_end = scan.end();
	while(iter_cur != iter_end){
		run(*iter_cur);
		++iter_cur;
	}
}
