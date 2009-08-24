#ifndef H_SLOT_ELEMENT_SET
#define H_SLOT_ELEMENT_SET

//custom
#include "slot_element.hpp"

//include
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>

//standard
#include <map>
#include <string>

class slot_element_set : private boost::noncopyable
{
public:
	slot_element_set();

	/*
	get:
		Returns the slot element that corresponds to hash, or a empty shared_ptr
		if there is no file with hash in share.
		Note: This function call can be expensive because if the slot_element
			doesn't already exist database access will be done to contruct it.
	remove:
		Removes the slot_element that corresponds to hash. This is done when a
		download is cancelled, or a remote host is done downloading a file from
		us.
	resume:
		Reads the database share table and creates slot_elements for all files
		that are not complete. All the slot elements are added to the Resume
		vector.
		Note: This is a extremely expensive call. It can takes seconds or MINUTES
			to create all the slot elements and hash check them. This function
			is called before the network is brought up.
	*/
	boost::shared_ptr<slot_element> get(const std::string & hash);
	void remove(const std::string & hash);
	void resume(std::vector<boost::shared_ptr<slot_element> > & Resume);

private:
	/*
	Hash mapped to a slot element. The slot_element is guaranteed to be unique.
	There will only exist one slot_element for a file at one time.
	Note: Slot_Element_mutex must be used for all access to Slot_Element. However
		it doesn't need to be used on a slot_element shared_ptr that has been
		copied out of the container.
	*/
	boost::mutex Slot_Element_mutex;
	std::map<std::string, boost::shared_ptr<slot_element> > Slot_Element;
};
#endif
