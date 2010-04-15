#ifndef H_LOGGER
#define H_LOGGER

//include
#include <boost/thread.hpp>

//standard
#include <iostream>
#include <sstream>

#define LOGGER(LEVEL) logger::create((LEVEL), __FILE__, __FUNCTION__, __LINE__)

class logger
{
public:
	enum level
	{
		utest, //unit test
		trace, //trace control flow (removed after debuging)
		debug, //insignificant event (left in code)
		event, //significant event (key press, network connection, etc)
		error, //unusual event (may or may not indicate a problem)
		fatal  //bad error (indicates definite problem)
	};

	~logger()
	{
		boost::mutex::scoped_lock lock(Static_Wrap.stdout_mutex());
		if(Level == utest){
			std::cout << "utest|";
		}else if(Level == trace){
			std::cout << "trace|";
		}else if(Level == debug){
			std::cout << "debug|";
		}else if(Level == event){
			std::cout << "event|";
		}else if(Level == error){
			std::cout << "error|";
		}else if(Level == fatal){
			std::cout << "fatal|";
		}
		std::cout << file << "|" << func << "|" << line << "|" << buf.str() << "\n";
	}

	static logger create(const level Level, const std::string & file,
		const std::string & func, const int line)
	{
		return logger(Level, file, func, line);
	}

	template<typename T>
	logger & operator << (const T & t)
	{
		buf << t;
		return *this;
	}

private:
	logger(
		const level Level_in,
		const std::string & file_in,
		const std::string & func_in,
		const int line_in
	):
		Level(Level_in),
		file(file_in),
		func(func_in),
		line(line_in)
	{

	}

	logger(const logger & L):
		Level(L.Level),
		file(L.file),
		func(L.func),
		line(L.line)
	{
		buf << L.buf.str();
	}

	level Level;
	std::string file;
	std::string func;
	int line;
	std::stringstream buf;

	//dummy template used to make initialization of once_flag in header possible
	template<typename T=int>
	class static_wrap
	{
	public:
		static_wrap()
		{
			//thread safe initialization of static data members
			boost::call_once(init, once_flag);
		}

		//lock for all access to stdout
		boost::mutex & stdout_mutex()
		{
			return _stdout_mutex();
		}
	private:
		static boost::once_flag once_flag;

		//initialize all static objects, called by ctor
		static void init()
		{
			_stdout_mutex();
		}

		static boost::mutex & _stdout_mutex()
		{
			static boost::mutex M;
			return M;
		}
	};
	static_wrap<> Static_Wrap;
};

template<typename T> boost::once_flag logger::static_wrap<T>::once_flag = BOOST_ONCE_INIT;
#endif
