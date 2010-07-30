#ifndef H_SPEED_COMPOSITE
#define H_SPEED_COMPOSITE

//include
#include <boost/shared_ptr.hpp>
#include <net/net.hpp>

//standard
#include <list>

/*
Wrapper for multiple net::speed_calc elements that need to be updated at the
same time.
*/
class speed_composite
{
public:
	/*
	add_bytes:
		Add bytes to all speed calculators in composite.
	add_speed_calc:
		Add speed_calc to be updated by add_bytes().
	*/
	void add(const unsigned n_bytes);
	void add_calc(boost::shared_ptr<net::speed_calc> SC);

private:
	std::list<boost::shared_ptr<net::speed_calc> > Elements;
};
#endif
