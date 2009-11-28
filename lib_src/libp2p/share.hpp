#ifndef H_SHARE
#define H_SHARE

//custom
#include "database.hpp"
#include "file_info.hpp"
#include "slot.hpp"

//include
#include <atomic_int.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/flyweight.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/once.hpp>
#include <boost/utility.hpp>
#include <database_connection.hpp>
#include <singleton.hpp>

//standard
#include <iterator>
#include <map>
#include <set>
#include <string>

class share : public singleton_base<share>
{
	friend class singleton_base<share>;
public:
	typedef boost::flyweights::flyweight<std::string> flyweight_string;

	/*
	Modifications to share don't invalidate this iterator.
	Note: This iterator is const because it allows the user access to only copies
		of data internal to share.
	*/
	class const_file_iterator : public std::iterator<std::input_iterator_tag, file_info>
	{
		friend class share;
	public:
		const_file_iterator(); //creates end iterator
		const_file_iterator & operator = (const const_file_iterator & rval);
		bool operator == (const const_file_iterator & rval) const;
		bool operator != (const const_file_iterator & rval) const;
		const file_info & operator * () const;
		const file_info * const operator -> () const;
		const_file_iterator & operator ++ ();
		const_file_iterator operator ++ (int);
	private:
		const_file_iterator(
			share * Share_in,
			const file_info & FI_in
		);
		share * Share;
		file_info FI;
	};

	//modifications to share don't invalidate this iterator
	class slot_iterator : public std::iterator<std::input_iterator_tag, slot>
	{
		friend class share;
	public:
		slot_iterator(); //creates end iterator
		slot_iterator & operator = (const slot_iterator & rval);
		bool operator == (const slot_iterator & rval) const;
		bool operator != (const slot_iterator & rval) const;
		slot & operator * ();
		boost::shared_ptr<slot> & operator -> ();
		slot_iterator & operator ++ ();
		slot_iterator operator ++ (int);

		//returns the shared_ptr the iterator wraps
		boost::shared_ptr<slot> get();
	private:
		slot_iterator(
			share * Share_in,
			const boost::shared_ptr<slot> & Slot_in
		);
		share * Share;
		boost::shared_ptr<slot> Slot;
	};

	/* File Related
	begin_file:
		Returns iterator to beginning of files.
	end_file:
		Returns iterator to end of files.
	erase:
		Remove file by path or iterator.
	insert_update:
		Insert or update file if a file already exists with the same path.
	lookup_hash:
		Lookup file by hash. The shared_ptr will be empty if no file found.
	lookup_path:
		Lookup file by path. The shared_ptr will be empty if no file found.
	*/
	const_file_iterator begin_file();
	const_file_iterator end_file();
	void erase(const std::string & path);
	void erase(const_file_iterator CFI);
	std::pair<const_file_iterator, bool> insert(const file_info & FI);
	const_file_iterator find_hash(const std::string & hash);
	const_file_iterator find_path(const std::string & path);

	/* Slot Related
	begin_slot:
		Returns iterator to beginning of slots.
	end_slot:
		Returns iterator to end of slots.
	find_slot:
		Returns slot_iterator for specified hash. Returns end iterator if file
		with specified hash not in share.
	is_downloading:
		Returns true if slot exists and it's incomplete. This function is used by
		the share scanner so it can know not to hash downloading files even though
		their size and modification dates are changing.
	slot_modified:
		Returns true if last_seen_state doesn't match the current state. The state
		is an integer internal to the share which gets incremented whenever a slot
		is added or removed.
		Note: To return true upon the first call last_seen_state must be set to -1.
		Postcondition: last_seen_state = slot_state.
	*/
	slot_iterator begin_slot();
	slot_iterator end_slot();
	slot_iterator find_slot(const std::string & hash);
	bool is_downloading(const std::string & path);

	/* Info
	bytes:
		Returns total number of bytes in share.
	files:
		Returns total number of files in share  excluding files with an empty
		hash.
	*/
	boost::uint64_t bytes();
	boost::uint64_t files();

private:
	share();

	/*
	Different ways of looking up info.
	Recursive_Mutex:
		Locks access to everything in this section.
	Hash:
		Hash associated with file_info. This is a multimap because there
		might be more than one file with the same hash (duplicate files).
	Path:
		Associates the file path with the file info.
	Slot:
		Hash associated with slot. This is a regular map (not a multimap) because
		we don't want more than one slot per hash.
	*/
	boost::recursive_mutex Recursive_Mutex;
	std::multimap<flyweight_string, boost::shared_ptr<file_info> > Hash;
	std::map<flyweight_string, boost::shared_ptr<file_info> > Path;
	std::map<flyweight_string, boost::shared_ptr<slot> > Slot;

	/*
	_bytes:
		The sum of the size of all files in share_info.
	_files:
		The number of hashed file in share.
	Note: Files with a empty hash don't count toward either of these totals.
		Files without a hash have not finished hashing and aren't shared.
	*/
	atomic_int<boost::uint64_t> _bytes;
	atomic_int<boost::uint64_t> _files;

	/*
	next_file_info:
		Uses the specified path for std::map::upper_bound(). Returns the file
		with the next greatest path or an empty shared_ptr if there is no more.
		This is used to implement const_file_iterator increment.
	next_slot:
		Uses the specified hash for std::map::upper_bound(). Returns the slot with
		the next greatest hash or an empty shared_ptr if no more slots. This is
		used to implement slot_iterator increment.
	remove_unused_slots:
		If the shared_ptr for a slot is unique and the slot it points to is
		complete then it will be removed. This is called by begin_slot().
	resume_priv:
		Contains logic to do resume from database.
	*/
	boost::shared_ptr<const file_info> next_file_info(const std::string & path);
	boost::shared_ptr<slot> next_slot(const std::string & hash);
	void remove_unused_slots();
	void resume_priv();
};
#endif
