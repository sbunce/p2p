#include "slot.hpp"

slot::slot():
	Speed_Calculator(settings::SPEED_AVERAGE)
{

}

const std::string & slot::get_hash()
{
	return root_hash;
}
