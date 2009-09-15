#ifndef H_SHARE
#define H_SHARE

//custom
#include "database.hpp"
#include "file_info.hpp"
#include "slot.hpp"

//include
#include <atomic_int.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/utility.hpp>
#include <database_connection.hpp>

//standard
#include <iterator>
#include <map>
#include <string>

class share : private boost::noncopyable
{
	class file_info_internal;
public:
	share();

	/*
	Modifications to share_info don't invalidate this iterator. This iterator
	works by passing the path of the current file to std::map::upper_bound() to
	get the next file.

	Note: This iterator is a input iterator so it is not guaranteed that
		++a == ++b is true even if a and b are initially equal.
	Note: This iterator is const because it allows the user access to only copies
		of data internal to share_info.
	*/
	class const_iterator : public std::iterator<std::input_iterator_tag, file_info>
	{
		friend class share;
	public:
		const_iterator(); //creates end iterator
		const_iterator & operator = (const const_iterator & rval);
		bool operator == (const const_iterator & rval) const;
		bool operator != (const const_iterator & rval) const;
		const file_info & operator * () const;
		const file_info * const operator -> () const;
		const_iterator & operator ++ ();
		const_iterator operator ++ (int);
	private:
		const_iterator(
			share * Share_in,
			const file_info & FI
		);
		share * Share;
		file_info FI;
	};
	//make friend so const_iterator can call share::next()
	friend class const_iterator;

	/*
	begin:
		Returns beginning const_iterator.
	bytes:
		Returns total number of bytes in share
	end:
		Returns end const iterator.
	erase:
		Remove file with specified path or iterator from share_info. Returns the
		number of elements erased.
		Note: The path is the only way to erase because it's guaranteed to be
			unique.
	files:
		Returns total number of files in share  excluding files with an empty
		hash.
	insert:
		Insert or update file if a file already exists with the same path.
	lookup_hash:
		Lookup file by hash. The shared_ptr will be empty if no file found.
	lookup_path:
		Lookup file by path. The shared_ptr will be empty if no file found.
	*/
	const_iterator begin();
	boost::uint64_t bytes();
	const_iterator end();
	int erase(const const_iterator & iter);
	int erase(const std::string & path);
	boost::uint64_t files();
	void insert_update(const file_info & FI);
	const_iterator lookup_hash(const std::string & hash);
	const_iterator lookup_path(const std::string & path);

private:
	/*
	This object saves space by storing a reference to the keys of Hash and Path
	instead of copies. The number of shared files can be very large so this is
	a big memory saver.
	*/
	class file_info_internal
	{
	public:
		file_info_internal(
			const std::string & hash_in,
			const std::string & path_in,
			const file_info & FI
		);
		const std::string & hash;
		const std::string & path;
		const boost::uint64_t file_size;
		const bool complete;
	};

	/*
	Different ways of looking up info.
	internal_mutex:
		Locks access to everything in this section.
	Hash:
		Associates file hashes with file_info. This is a multimap because there
		might be more than one file with the same hash (duplicate files).
	Path:
		Associates the file path with the file info.
	*/
	boost::recursive_mutex internal_mutex;
	std::multimap<std::string, boost::shared_ptr<file_info_internal> > Hash;
	std::map<std::string, boost::shared_ptr<file_info_internal> > Path;

	/*
	total_bytes:
		The sum of the size of all files in share_info.
	total_files:
		The number of hashed file in share.
	Note: Files with a empty hash don't count toward either of these totals.
		Files without a hash have not finished hashing and aren't shared.
	*/
	atomic_int<boost::uint64_t> total_bytes;
	atomic_int<boost::uint64_t> total_files;

	/*
	FII_to_FI:
		Converts a file_info_internal to a file_info. We have this instead of
		adding a ctor to file_info because we don't want file_info to know about
		file_info_internal.
	next:
		Uses the specified path for std::map::upper_bound(). Returns the file
		with the next greatest path. Used to implement const_iterator increment.
	*/
	file_info FII_to_FI(const file_info_internal & FII);
	boost::shared_ptr<const file_info> next(const std::string & path);
};
#endif
