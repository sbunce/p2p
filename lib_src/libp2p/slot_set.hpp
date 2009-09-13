#ifndef H_SLOT_ELEMENT_SET
#define H_SLOT_ELEMENT_SET

//custom
#include "shared_files.hpp"
#include "slot.hpp"

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

class slot_set : private boost::noncopyable
{
public:
	slot_set(shared_files & Shared_Files_in);
	~slot_set();

	/*
	add:
		If the slot exists in memory it will be returned. If it doesn't exist info
		to create the slot will be looked up in the database.
	remove:
		Removes the slot that corresponds to hash. This is done when a download is
		cancelled, or a remote host is done downloading a file from us.
	resume:
		Spawns a thread to read all downloading files in share and hash check
		them. This is called once on program start in the p2p_real ctor.
	*/
	boost::shared_ptr<slot> add(const std::string & hash);
	void remove(const std::string & hash);
	void resume(network::proactor & Proactor);

private:
	//handle for thread that resume() starts
	boost::thread resume_thread;

	shared_files & Shared_Files;

	/*
	Hash mapped to a slot element. The slot is guaranteed to be unique. There
	will only exist one slot_element for a file at one time in either the Slot or
	Slot_Resume container.

	Slot:
		Container for slots which aren't hash checking.
	Slot_Resume:
		Container for slots which are being hash checked. Elements from this
		container are not allowed outside the slot_set until hash checking is
		complete and they are put in the Slot container.

	Note: Slot_mutex must be used for all modification to either container.
	*/
	boost::mutex Slot_mutex;
	std::map<std::string, boost::shared_ptr<slot> > Slot;
	std::map<std::string, boost::shared_ptr<slot> > Slot_Resume;

	/*
	check:
		Checks the hash_tree and file on resume downloads.
		Note: The shared_ptr is passed by value because the shared_ptr in Slot
		might be removed when hash check is happening. We don't want to free an
		object with a thread in it.
	resume_priv:
		The resume() function spawns a thread in this function to do the actual
		resume work.
	*/
	void check(boost::shared_ptr<slot> SE);
	void resume_priv(network::proactor & Proactor);
};
#endif
