#include "share.hpp"

//BEGIN share::const_iterator
share::const_iterator::const_iterator():
	Share(NULL)
{}

share::const_iterator::const_iterator(
	share * Share_in,
	const file_info & FI_in
):
	Share(Share_in),
	FI(FI_in)
{}

share::const_iterator & share::const_iterator::operator = (
	const share::const_iterator & rval)
{
	Share = rval.Share;
	FI = rval.FI;
	return *this;
}

bool share::const_iterator::operator == (
	const share::const_iterator & rval) const
{
	if(Share == NULL && rval.Share == NULL){
		//both are end iterators
		return true;
	}else if(Share == NULL || rval.Share == NULL){
		//only one is end iterator
		return false;
	}else{
		//neither is end iterator, compare files
		return FI.path == rval.FI.path;
	}
}

bool share::const_iterator::operator != (const share::const_iterator & rval) const
{
	return !(*this == rval);
}

const file_info & share::const_iterator::operator * () const
{
	assert(Share);
	return FI;
}

const file_info * const share::const_iterator::operator -> () const
{
	assert(Share);
	return &FI;
}

share::const_iterator & share::const_iterator::operator ++ ()
{
	assert(Share);
	boost::shared_ptr<const file_info> temp = Share->next(FI.path);
	if(temp){
		FI = *temp;
	}else{
		Share = NULL;
	}
	return *this;
}

share::const_iterator share::const_iterator::operator ++ (int)
{
	assert(Share);
	const_iterator temp = *this;
	++(*this);
	return temp;
}
//END share::const_iterator

//BEGIN share::file_info_internal
share::file_info_internal::file_info_internal(
	const std::string & hash_in,
	const std::string & path_in,
	const file_info & FI
):
	hash(hash_in),
	path(path_in),
	file_size(FI.file_size),
	complete(FI.complete)
{

}
//END share::file_info_internal

share::share():
	total_bytes(0),
	total_files(0)
{

}

share::const_iterator share::begin()
{
	boost::recursive_mutex::scoped_lock lock(internal_mutex);
	if(Path.empty()){
		return const_iterator();
	}else{
		return const_iterator(this, FII_to_FI(*Path.begin()->second));
	}
}

boost::uint64_t share::bytes()
{
	return total_bytes;
}

share::const_iterator share::end()
{
	return const_iterator();
}

int share::erase(const const_iterator & iter)
{
	return erase(iter->path);
}

int share::erase(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(internal_mutex);

	//locate Path element
	std::map<std::string, boost::shared_ptr<file_info_internal> >::iterator
		Path_iter = Path.find(path);
	if(Path_iter != Path.end()){
		if(!Path_iter->second->hash.empty()){
			total_bytes -= Path_iter->second->file_size;
			--total_files;
		}

		//locate Hash element
		std::pair<std::map<std::string, boost::shared_ptr<file_info_internal> >::iterator,
			std::map<std::string, boost::shared_ptr<file_info_internal> >::iterator>
			Hash_iter_pair = Hash.equal_range(Path_iter->second->hash);
		for(; Hash_iter_pair.first != Hash_iter_pair.second; ++Hash_iter_pair.first){
			if(Hash_iter_pair.first->second == Path_iter->second){
				//found element in Hash container for this file, erase it
				Hash.erase(Hash_iter_pair.first);
				break;
			}
		}
		//erase the Path element
		Path.erase(Path_iter);
		return 1;
	}
	return 0;
}

file_info share::FII_to_FI(const file_info_internal & FII)
{
	return file_info(FII.hash, FII.path, FII.file_size, FII.complete);
}

boost::uint64_t share::files()
{
	return total_files;
}

void share::insert_update(const file_info & FI)
{
	boost::recursive_mutex::scoped_lock lock(internal_mutex);

	//check if there is an existing element
	std::map<std::string, boost::shared_ptr<file_info_internal> >::iterator
		Path_iter = Path.find(FI.path);
	if(Path_iter != Path.end()){
		//previous element found, erase it
		erase(Path_iter->second->path);
		//Note: At this point Path_iter has been invalidated.
	}

	/*
	Insert in to Path and Hash. But associate with empty shared_ptr because we
	need references to the keys of Path and Hash to construct the
	file_info_internal.
	*/
	std::multimap<std::string, boost::shared_ptr<file_info_internal> >::iterator
		Hash_iter = Hash.insert(std::make_pair(FI.hash, boost::shared_ptr<file_info_internal>()));
	std::pair<std::map<std::string, boost::shared_ptr<file_info_internal> >::iterator,
		bool> ret = Path.insert(std::make_pair(FI.path, boost::shared_ptr<file_info_internal>()));
	assert(ret.second);

	//construct file_info_internal now that we have needed references
	boost::shared_ptr<file_info_internal> FII(new file_info_internal(
		Hash_iter->first, ret.first->first, FI));

	//associate Path and Hash with the file_info_internal
	Hash_iter->second = FII;
	ret.first->second = FII;

	if(!FI.hash.empty()){
		total_bytes += FI.file_size;
		++total_files;
	}
}

share::const_iterator share::lookup_hash(const std::string & hash)
{
	boost::recursive_mutex::scoped_lock lock(internal_mutex);

	//find any file with the hash, which one doesn't matter
	std::multimap<std::string, boost::shared_ptr<file_info_internal> >::iterator
		iter = Hash.find(hash);
	if(iter == Hash.end()){
		//no file found
		return end();
	}else{
		return const_iterator(this, file_info(FII_to_FI(*iter->second)));
	}
}

share::const_iterator share::lookup_path(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(internal_mutex);

	//find file with path
	std::map<std::string, boost::shared_ptr<file_info_internal> >::iterator
		iter = Path.find(path);
	if(iter == Path.end()){
		//no file found
		return end();
	}else{
		return const_iterator(this, file_info(FII_to_FI(*iter->second)));
	}
}

boost::shared_ptr<const file_info> share::next(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(internal_mutex);

	//find path one greater than specified path
	std::map<std::string, boost::shared_ptr<file_info_internal> >::iterator
		iter = Path.upper_bound(path);
	if(iter == Path.end()){
		//no file found
		return boost::shared_ptr<file_info>();
	}else{
		return boost::shared_ptr<file_info>(new file_info(FII_to_FI(*iter->second)));
	}
}
