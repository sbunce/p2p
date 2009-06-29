//include
#include <atomic_bool.hpp>
#include <logger.hpp>

//standard
#include <sstream>

void assignment()
{
	atomic_bool x, y;

	//=
	x = false; y = true;
	x = y;
	if(x != true){
		LOGGER; exit(1);
	}
}

void relational()
{
	atomic_bool x, y;

	//==
	x = true; y = false;
	if(x == y){
		LOGGER; exit(1);
	}
	if(x == false){
		LOGGER; exit(1);
	}

	//!=
	x = true; y = true;
	if(x != y){
		LOGGER; exit(1);
	}
	if(x != true){
		LOGGER; exit(1);
	}
}

void conditional()
{
	atomic_bool x;

	//()
	x = false;
	if(x){
		LOGGER; exit(1);
	}
}

void stream()
{
	atomic_bool x(false);
	std::stringstream ss;
	ss << x;
	if(ss.str() != "0"){
		LOGGER; exit(1);
	}

	ss >> x;
	if(x){
		LOGGER; exit(1);
	}
}

int main()
{
	assignment();
	relational();
	conditional();
	stream();

	return 0;
}
