//std
#include <iostream>

#include "logger.h"

/*
Set static logger pointer to zero to indicate the singleton hasn't yet been
instantiated
*/
volatile logger * logger::logger_instance = NULL;

logger::logger()
{

}

void logger::write_log(std::string message)
{
	std::cout << message << "\n";
}
