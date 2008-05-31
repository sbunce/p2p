#include "logger.h"

volatile logger * logger::logger_instance = NULL;
boost::mutex logger::Mutex;

logger::logger()
{

}

void logger::write_log(const std::string & message)
{
	std::cout << message << "\n";
}
