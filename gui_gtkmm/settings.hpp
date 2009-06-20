#ifndef H_SETTINGS
#define H_SETTINGS

//std
#include <string>

namespace settings
{
const std::string NAME = "P2P";        //name of program
const std::string VERSION = "0.00.00";
const unsigned GUI_TICK = 100;         //time(milliseconds) between gui updates
const unsigned MAX_CONNECTIONS = 1000; //maximum connections
}//end of settings namespace
#endif
