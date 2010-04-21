//custom
#include "../kad_func.hpp"

int fail(0);

int main()
{
	std::string ID_0 = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF";
	std::string ID_1 = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEE";

	//ID_to_mpint
	mpa::mpint m_ID = kad_func::ID_to_mpint(ID_0);
	if(m_ID.str(16) != ID_0){
		LOG; ++fail;
	}

	//distance to self
	if(kad_func::distance(ID_0, ID_0) != "0"){
		LOG; ++fail;
	}

	//distance to neighbor
	if(kad_func::distance(ID_0, ID_1) != "1"){
		LOG; ++fail;
	}

	return fail;
}
