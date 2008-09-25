#include "scan.h"

scan::scan()
{
	std::cout << "digraph G {\n";
}

scan::~scan()
{
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

void scan::read_preprocessor(std::fstream & fin, std::string & root_file)
{
	//this function is called after a # is read from fin
	char * inc = "include";
	char ch;
	for(int x=0; x<7; ++x){
		if(fin.get(ch)){
			if(ch != inc[x]){
				//not an include macro
				return;
			}
		}
	}

	//after include but before " or < there might be a space
	if(!fin.get(ch)){
		//eof
		return;
	}

	if(ch == ' '){
		//space, read again
		if(!fin.get(ch)){
			//eof
			return;
		}
	}

	//only read custom includes that begin with '"'
	if(ch == '"'){
		//read header file name
		std::string name;
		while(fin.get(ch)){
			if(ch == '"'){
				std::cout << "\"" << root_file << "\" -> \"" << name << "\";\n";
				run(name);
				return;
			}else{
				name += ch;
			}
		}
	}
}

void scan::run(std::string root_file)
{
	//do not scan files which have already been scanned
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
		exit(1);
	}

	char ch;
	while(fin.get(ch)){
		if(ch == '/'){
			eat_comment(fin);
		}else if(ch == '#'){
			read_preprocessor(fin, root_file);
		}
	}
}
