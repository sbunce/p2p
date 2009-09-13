#ifndef H_SHARED_FILES
#define H_SHARED_FILES

//include
#include <atomic_int.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>

//standard
#include <iterator>
#include <map>
#include <string>

class shared_files
{
	class file_internal;
public:
	shared_files();

	/*
	When file info needs to be returned it is returned in a file object. The
	file_internal object is not returned because it wouldn't be thread safe.
	*/
	class file
	{
		friend class shared_files;
	public:
		file();
		file(
			const std::string & hash_in,
			const std::string & path_in,
			const boost::uint64_t file_size_in
		);
		file(const file & File);

		std::string hash;
		std::string path;
		boost::uint64_t file_size;

		bool operator == (const file & rval) const;
		bool operator != (const file & rval) const;

	private:
		file(const file_internal & FI);
	};

	/*
	Modifications to share_info don't invalidate this iterator. This iterator
	works by passing the path of the current file to std::map::upper_bound() to
	get the next file.

	Note: This iterator is a input iterator so it is not guaranteed that
		++a == ++b is true even if a and b are initially equal.
	Note: This iterator is const because it allows the user access to only copies
		of data internal to share_info.
	*/
	class const_iterator : public std::iterator<std::input_iterator_tag, file>
	{
		friend class shared_files;
	public:
		//creates an end iterator
		const_iterator();

		const_iterator & operator = (const const_iterator & rval);
		bool operator == (const const_iterator & rval) const;
		bool operator != (const const_iterator & rval) const;
		const file & operator * () const;
		const file * const operator -> () const;
		const_iterator & operator ++ ();
		const_iterator operator ++ (int);

	private:
		const_iterator(
			shared_files * Shared_Files_in,
			const file & File_in
		);

		/*
		The share_info which created the iterator. If this is NULL then the
		iterator is an end iterator.
		*/
		shared_files * Shared_Files;

		//current file the iterator is at
		file File;
	};

	/*
	begin:
		Returns beginning const_iterator.
	bytes:
		Returns total number of bytes in share
	end:
		Returns end const iterator.
	erase:
		Remove file with specified path or iterator from share_info.
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
	next:
		Uses the specified path for std::map::upper_bound(). Returns the file
		with the next greatest path. Generally the iterator should be used instead
		of this.
	*/
	const_iterator begin();
	boost::uint64_t bytes();
	const_iterator end();
	void erase(const const_iterator & iter);
	void erase(const std::string & path);
	boost::uint64_t files();
	void insert_update(const file & File);
	boost::shared_ptr<const file> lookup_hash(const std::string & hash);
	boost::shared_ptr<const file> lookup_path(const std::string & path);
	boost::shared_ptr<const file> next(const std::string & path);

private:
	/*
	The file_internal class keeps references to the keys of Hash and Path
	containers to save space.
	*/
	class file_internal
	{
	public:
		file_internal(
			const std::string & hash_in,
			const std::string & path_in,
			const boost::uint64_t file_size_in
		);

		const std::string & hash;  //reference to key in Hash
		const std::string & path;  //reference to key in Path
		boost::uint64_t file_size; //bytes
	};

	/*
	Different ways of looking up info.
	Note: Hash is a multimap because there might be multiple files with the same
		hash.
	Note: file_internal_mutex lock access to Hash, Path, and any info_internal
	*/
	boost::recursive_mutex file_internal_mutex;
	std::multimap<std::string, boost::shared_ptr<file_internal> > Hash;
	std::map<std::string, boost::shared_ptr<file_internal> > Path;

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
};
#endif
