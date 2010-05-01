#ifndef H_OPT_PARSE
#define H_OPT_PARSE

//include
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>

//standard
#include <iostream>
#include <sstream>
#include <string>

class opt_parse
{
public:
	opt_parse(const int argc, char ** argv):
		valid_opt_expr("[a-zA-Z0-9_]+"),
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
				std::cout << *it_cur;
				std::list<std::string>::iterator it_next = it_cur;
				++it_next;
				if(it_next != it_end){
					std::cout << " ";
				}
			}
			std::cout << "\"\n";
			return true;
		}
	}

	/*
	Returns true if flag present. Flag option looks like:
	<base>     = <"-"><opt_list>
	<opt_list> = <opt><opt_list>|<opt>
	*/
	bool flag(const char opt)
	{
		validate_opt(opt);
		std::stringstream ss;
		ss << "^-.*(" << opt << ").*";
		boost::regex expr(ss.str());
		for(std::list<std::string>::iterator it_cur = args.begin(),
			it_end = args.end(); it_cur != it_end; ++it_cur)
		{
			boost::match_results<std::string::iterator> what;
			if(boost::regex_search(it_cur->begin(), it_cur->end(), what, expr)){
				if(it_cur->size() == 2){
					//flag looks like -f
					args.erase(it_cur);
				}else{
					//flag looks like -fgh (composite flag)
					it_cur->erase(what.position(1), 1);
				}
				return true;
			}
		}
		return false;
	}

	/*
	Returns true if flag present. Flag options looks like:
	<base> = <"--"><opt>
	*/
	bool flag(const std::string & opt)
	{
		validate_opt(opt);
		std::stringstream ss;
		ss << "^--" << opt;
		boost::regex expr(ss.str());
		for(std::list<std::string>::iterator it_cur = args.begin(),
			it_end = args.end(); it_cur != it_end; ++it_cur)
		{
			if(boost::regex_match(it_cur->begin(), it_cur->end(), expr)){
				args.erase(it_cur);
				return true;
			}
		}
		return false;
	}

	/*
	Returns string in string option. String option looks like:
	<base>   = <"--"><spacer><string>
	<spacer> = <"">|<" ">|<"=">
	*/
	boost::optional<std::string> string(const std::string & opt)
	{
		validate_opt(opt);
		std::stringstream ss;
		ss << "^--" << opt << "(={0,1})(.*)";
		boost::regex expr(ss.str());
		boost::match_results<std::string::iterator> what;
		for(std::list<std::string>::iterator it_cur = args.begin(),
			it_end = args.end(); it_cur != it_end; ++it_cur)
		{
			if(boost::regex_search(it_cur->begin(), it_cur->end(), what, expr)){
				if(what[2] != ""){
					//string in current element (option looks like --opt123 or --opt=123)
					std::string tmp = what[2];
					args.erase(it_cur);
					return boost::optional<std::string>(tmp);
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

	//does lexical cast on a string option
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
	boost::regex valid_opt_expr;
	std::list<std::string> args;

	void validate_opt(const std::string & opt)
	{
		if(!boost::regex_match(opt.begin(), opt.end(), valid_opt_expr)){
			std::cout << "invalid char in opt \"" << opt << "\"\n";
		}
	}

	void validate_opt(const char opt)
	{
		std::string tmp;
		tmp += opt;
		validate_opt(tmp);
	}
};
#endif
