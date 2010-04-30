#ifndef H_OPT_PARSE
#define H_OPT_PARSE

//include
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <logger.hpp>

//standard
#include <algorithm>
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

class opt_parse
{
public:
	opt_parse(const int argc, char ** argv):
		args(argv+1, argv+argc)
	{

	}

	/*
	After checking for all options this function should be called. If there are
	unused options they will be printed and true will be returned.
	*/
	bool unparsed()
	{
		if(args.empty()){
			return false;
		}else{
			std::cout << "unknown options \"";
			for(std::list<std::string>::iterator it_cur = args.begin(),
			it_end = args.end(); it_cur != it_end; ++it_cur)
			{
				std::list<std::string>::iterator it_next = it_cur;
				++it_next;
				std::cout << *it_cur;
				if(it_next != it_end){
					std::cout << " ";
				}
			}
			std::cout << "\"\n";
			return true;
		}
	}

	/*
	Returns a boolean flag. An option that doesn't have any string associated
	with it.
	*/
	bool flag(const std::string & opt)
	{
		for(std::list<std::string>::iterator it_cur = args.begin(),
			it_end = args.end(); it_cur != it_end; ++it_cur)
		{
			if(*it_cur == opt){
				std::string tmp = *it_cur;
				args.erase(it_cur);
				return true;
			}
		}
		return false;
	}

	/*
	Returns string option of the form:
	<opt><string>
	<opt>=<string>
	<opt> <string>
	*/
	boost::optional<std::string> string(const std::string & opt)
	{
		assert(!contains_regex_char(opt));
		std::stringstream ss;
		ss << "^" << opt << "(={0,1})(.*)";
		boost::regex expr(ss.str());
		for(std::list<std::string>::iterator it_cur = args.begin(),
			it_end = args.end(); it_cur != it_end; ++it_cur)
		{
			boost::match_results<std::string::iterator> what;
			if(boost::regex_search(it_cur->begin(), it_cur->end(), what, expr)){
				if(what[2] != ""){
					//string in current element (option looks like --opt123 or --opt=123)
					args.erase(it_cur);
					return boost::optional<std::string>(what[2]);
				}else{
					//string in next element (option looks like --opt 123)
					std::list<std::string>::iterator it_next = it_cur;
					++it_next;
					if(it_next == it_end){
						std::cout << "error, expected string after \"" << opt << "\"\n";
						exit(1);
					}else{
						std::string tmp = *it_next;
						++it_next;
						args.erase(it_cur, it_next);
						return boost::optional<std::string>(tmp);
					}
				}
			}
		}
		return boost::optional<std::string>();
	}

	/*
	Does lexical cast on a string option. See string() for how to specify the
	option.
	*/
	template<typename T>
	boost::optional<T> lexical_cast(const std::string & opt)
	{
		boost::optional<std::string> tmp = string(opt);
		if(tmp){
			try{
				return boost::optional<T>(boost::lexical_cast<T>(*tmp));
			}catch(const boost::bad_lexical_cast & e){
				std::cout << "error, lexical_cast option \"" << opt << "\" value \""
					<< *tmp << "\"\n";
				std::cout << e.what() << "\n";
				exit(1);
			}
		}else{
			return boost::optional<T>();
		}
	}

private:
	std::list<std::string> args;

	bool contains_regex_char(const std::string & opt)
	{
		return std::strcspn(opt.c_str(), ".[{()\\*+?|^$") != opt.size();
	}
};
#endif
