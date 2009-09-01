#ifndef H_SLOT_ELEMENT_SET
#define H_SLOT_ELEMENT_SET

//custom
#include "slot_element.hpp"

//include
#include <atomic_int.hpp>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <network/network.hpp>

//standard
#include <map>
#include <string>

class slot_element_set : private boost::noncopyable
{
public:
	slot_element_set();
	~slot_element_set();

	/*
	exists:
		Returns true if the slot_element exists in the Slot_Element container.
		This function won't do a database lookup like get() will. This function is
		called when a connection sees that the slot_element_set has been modified.
		It will call this function for each slot_element it contains to see if it
		was removed.
	get:
		Returns the slot element that corresponds to hash, or a empty shared_ptr
		if there is no file with hash in share.
		Note: This function call can be expensive because if the slot_element
			doesn't already exist database access will be done to contruct it.
	modified:
		Returns true if last_state_seen doesn't match the current
		slot_element_set_state.
		Note: The int passed to this function should be initialized to zero before
			the first call.
		Postcondition: last_state_seen = blacklist_state.
	remove:
		Removes the slot_element that corresponds to hash. This is done when a
		download is cancelled, or a remote host is done downloading a file from
		us.
	resume:
		Spawns a thread to read all downloading files in share and hash check
		them. This is called once on program start in the p2p_real ctor.
	*/
	bool exists(boost::shared_ptr<slot_element> & SE);
	boost::shared_ptr<slot_element> get(const std::string & hash);
	bool modified(int & last_state_seen);
	void remove(const std::string & hash);
	void resume(network::proactor & Proactor);

private:
	//handle for thread that resume() starts
	boost::thread resume_thread;

	/*
	Hash mapped to a slot element. The slot_element is guaranteed to be unique.
	There will only exist one slot_element for a file at one time.

	Slot_Element:
		Container for slot_elements which aren't hash checking.
	Slot_Element_Resume:
		Container for slot_elements which are being hash checked. Elements from
	this container are not allowed outside the slot_element_set until hash
	checking is complete and they are put in the Slot_Element container.

	Note: Slot_Element_mutex must be used for all access to Slot_Element. However
		it doesn't need to be used on a slot_element shared_ptr that has been
		copied out of the container.
	*/
	boost::mutex Slot_Element_mutex;
	std::map<std::string, boost::shared_ptr<slot_element> > Slot_Element;
	std::map<std::string, boost::shared_ptr<slot_element> > Slot_Element_Resume;

	/*
	Whenever an element is added ore removed to Slot_Element this counter is
	incremented.
	*/
	atomic_int<int> slot_element_set_state;

	/*
	check:
		Checks the hash_tree and file on resume downloads.
		Note: The shared_ptr is passed by value because the shared_ptr in
			Slot_Element might be removed when hash check is happening. We don't
			want to free an object with a thread in it.
	resume_priv:
		The resume() function spawns a thread in this function to do the actual
		resume work.
	*/
	void check(boost::shared_ptr<slot_element> SE);
	void resume_priv(network::proactor & Proactor);
};
#endif
