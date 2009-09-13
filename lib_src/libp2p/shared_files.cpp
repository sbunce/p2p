#include "shared_files.hpp"

//BEGIN shared_files::file
shared_files::file::file():
	file_size(0)
{}

shared_files::file::file(
	const std::string & hash_in,
	const std::string & path_in,
	const boost::uint64_t file_size_in
):
	hash(hash_in),
	path(path_in),
	file_size(file_size_in)
{}

shared_files::file::file(
	const shared_files::file & File
):
	hash(File.hash),
	path(File.path),
	file_size(File.file_size)
{}

shared_files::file::file(
	const file_internal & FI
):
	hash(FI.hash),
	path(FI.path),
	file_size(FI.file_size)
{}

bool shared_files::file::operator == (const shared_files::file & rval) const
{
	return hash == rval.hash && path == rval.path && file_size == rval.file_size;
}

bool shared_files::file::operator != (const shared_files::file & rval) const
{
	return !(*this == rval);
}
//END shared_files::file

//BEGIN shared_files::const_iterator
shared_files::const_iterator::const_iterator():
	Shared_Files(NULL)
{}

shared_files::const_iterator::const_iterator(
	shared_files * Shared_Files_in,
	const shared_files::file & File_in
):
	Shared_Files(Shared_Files_in),
	File(File_in)
{}

shared_files::const_iterator & shared_files::const_iterator::operator = (
	const shared_files::const_iterator & rval)
{
	Shared_Files = rval.Shared_Files;
	File = rval.File;
	return *this;
}

bool shared_files::const_iterator::operator == (
	const shared_files::const_iterator & rval) const
{
	if(Shared_Files == NULL && rval.Shared_Files == NULL){
		//both are end iterators
		return true;
	}else if(Shared_Files == NULL || rval.Shared_Files == NULL){
		//only one is end iterator
		return false;
	}else{
		//neither is end iterator, compare files
		return File == rval.File;
	}
}

bool shared_files::const_iterator::operator != (const shared_files::const_iterator & rval) const
{
	return !(*this == rval);
}

const shared_files::file & shared_files::const_iterator::operator * () const
{
	assert(Shared_Files);
	return File;
}

const shared_files::file * const shared_files::const_iterator::operator -> () const
{
	assert(Shared_Files);
	return &File;
}

shared_files::const_iterator & shared_files::const_iterator::operator ++ ()
{
	assert(Shared_Files);
	boost::shared_ptr<const file> temp = Shared_Files->next(File.path);
	if(temp){
		File = *temp;
	}else{
		Shared_Files = NULL;
	}
	return *this;
}

shared_files::const_iterator shared_files::const_iterator::operator ++ (int)
{
	assert(Shared_Files);
	const_iterator temp = *this;
	++(*this);
	return temp;
}
//END shared_files::const_iterator

//BEGIN shared_files::file_internal
shared_files::file_internal::file_internal(
	const std::string & hash_in,
	const std::string & path_in,
	const boost::uint64_t file_size_in
):
	hash(hash_in),
	path(path_in),
	file_size(file_size_in)
{

}
//END shared_files::file_internal

shared_files::shared_files():
	total_bytes(0),
	total_files(0)
{

}

shared_files::const_iterator shared_files::begin()
{
	boost::recursive_mutex::scoped_lock lock(file_internal_mutex);
	if(Path.empty()){
		return const_iterator();
	}else{
		return const_iterator(this, file(*Path.begin()->second));
	}
}

boost::uint64_t shared_files::bytes()
{
	return total_bytes;
}

shared_files::const_iterator shared_files::end()
{
	return const_iterator();
}

void shared_files::erase(const const_iterator & iter)
{
	erase(iter->path);
}

void shared_files::erase(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(file_internal_mutex);

	//locate Path element
	std::map<std::string, boost::shared_ptr<file_internal> >::iterator
		Path_iter = Path.find(path);
	if(Path_iter != Path.end()){
		if(!Path_iter->second->hash.empty()){
			total_bytes -= Path_iter->second->file_size;
			--total_files;
		}

		//locate Hash element
		std::pair<std::map<std::string, boost::shared_ptr<file_internal> >::iterator,
			std::map<std::string, boost::shared_ptr<file_internal> >::iterator>
			Hash_iter_pair = Hash.equal_range(Path_iter->second->hash);
		for(; Hash_iter_pair.first != Hash_iter_pair.second; ++Hash_iter_pair.first){
			if(Hash_iter_pair.first->second == Path_iter->second){
				//found element in Hash container for this file, erase it
				Hash.erase(Hash_iter_pair.first);
				/*
				Note: At this point the hash data member of the file_internal object
					can't be accessed since it was a reference to the key of the Hash
					element we just erased.
				*/
				break;
			}
		}
		//erase the Path element
		Path.erase(Path_iter);
	}
}

boost::uint64_t shared_files::files()
{
	return total_files;
}

void shared_files::insert_update(const file & File)
{
	boost::recursive_mutex::scoped_lock lock(file_internal_mutex);

	//check if there is an existing element
	std::map<std::string, boost::shared_ptr<file_internal> >::iterator
		Path_iter = Path.find(File.path);
	if(Path_iter != Path.end()){
		//previous element found, erase it
		erase(Path_iter->second->path);
		//Note: At this point Path_iter has been invalidated.
	}

	/*
	Insert new element in to Hash container.
	Note: An empty shared_ptr is inserted because we dont yet have a hash
		reference to pass the file_internal constructor.
	*/
	std::multimap<std::string, boost::shared_ptr<file_internal> >::iterator
		Hash_iter = Hash.insert(std::make_pair(File.hash, boost::shared_ptr<file_internal>()));

	/*
	Insert new element in to Path container.
	Note: An empty shared_ptr is inserted because we dont yet have a path
		reference to pass the file_internal constructor.
	*/
	std::pair<std::map<std::string, boost::shared_ptr<file_internal> >::iterator,
		bool> ret = Path.insert(std::make_pair(File.path, boost::shared_ptr<file_internal>()));
	assert(ret.second);

	//create file_internal and associate it with the Hash and Path elements
	boost::shared_ptr<file_internal> FI(new file_internal(Hash_iter->first,
		ret.first->first, File.file_size));
	Hash_iter->second = FI;
	ret.first->second = FI;

	if(!File.hash.empty()){
		total_bytes += File.file_size;
		++total_files;
	}
}

boost::shared_ptr<const shared_files::file> shared_files::lookup_hash(const std::string & hash)
{
	boost::recursive_mutex::scoped_lock lock(file_internal_mutex);

	//find any file with the hash, which one doesn't matter
	std::multimap<std::string, boost::shared_ptr<file_internal> >::iterator
		iter = Hash.find(hash);
	if(iter == Hash.end()){
		//no file found
		return boost::shared_ptr<file>();
	}else{
		return boost::shared_ptr<file>(new file(*iter->second));
	}
}

boost::shared_ptr<const shared_files::file> shared_files::lookup_path(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(file_internal_mutex);

	//find file with path
	std::map<std::string, boost::shared_ptr<file_internal> >::iterator
		iter = Path.find(path);
	if(iter == Path.end()){
		//no file found
		return boost::shared_ptr<file>();
	}else{
		return boost::shared_ptr<file>(new file(*iter->second));
	}
}

boost::shared_ptr<const shared_files::file> shared_files::next(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(file_internal_mutex);

	//find path one greater than specified path
	std::map<std::string, boost::shared_ptr<file_internal> >::iterator
		iter = Path.upper_bound(path);
	if(iter == Path.end()){
		//no file found
		return boost::shared_ptr<file>();
	}else{
		return boost::shared_ptr<file>(new file(*iter->second));
	}
}
