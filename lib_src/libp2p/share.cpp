#include "share.hpp"

//BEGIN share::const_file_iterator
share::const_file_iterator::const_file_iterator():
	Share(NULL)
{}

share::const_file_iterator::const_file_iterator(
	share * Share_in,
	const file_info & FI_in
):
	Share(Share_in),
	FI(FI_in)
{}

share::const_file_iterator & share::const_file_iterator::operator = (
	const share::const_file_iterator & rval)
{
	Share = rval.Share;
	FI = rval.FI;
	return *this;
}

bool share::const_file_iterator::operator == (
	const share::const_file_iterator & rval) const
{
	if(Share == NULL && rval.Share == NULL){
		//both are end iterators
		return true;
	}else if(Share == NULL || rval.Share == NULL){
		//only one is end iterator
		return false;
	}else{
		//neither is end iterator, compare paths
		return FI.path == rval.FI.path;
	}
}

bool share::const_file_iterator::operator != (
	const share::const_file_iterator & rval) const
{
	return !(*this == rval);
}

const file_info & share::const_file_iterator::operator * () const
{
	assert(Share);
	return FI;
}

const file_info * const share::const_file_iterator::operator -> () const
{
	assert(Share);
	return &FI;
}

share::const_file_iterator & share::const_file_iterator::operator ++ ()
{
	assert(Share);
	boost::shared_ptr<const file_info> temp = Share->next_file_info(FI.path);
	if(temp){
		FI = *temp;
	}else{
		Share = NULL;
	}
	return *this;
}

share::const_file_iterator share::const_file_iterator::operator ++ (int)
{
	assert(Share);
	const_file_iterator temp = *this;
	++(*this);
	return temp;
}
//END share::const_file_iterator

//BEGIN share::slot_iterator
share::slot_iterator::slot_iterator():
	Share(NULL)
{}

share::slot_iterator::slot_iterator(
	share * Share_in,
	const boost::shared_ptr<slot> & Slot_in
):
	Share(Share_in),
	Slot(Slot_in)
{}

share::slot_iterator & share::slot_iterator::operator = (
	const share::slot_iterator & rval)
{
	Share = rval.Share;
	Slot = rval.Slot;
	return *this;
}

bool share::slot_iterator::operator == (
	const share::slot_iterator & rval) const
{
	if(Share == NULL && rval.Share == NULL){
		//both are end iterators
		return true;
	}else if(Share == NULL || rval.Share == NULL){
		//only one is end iterator
		return false;
	}else{
		//neither is end iterator, compare paths
		return Slot->hash == rval.Slot->hash;
	}
}

bool share::slot_iterator::operator != (
	const share::slot_iterator & rval) const
{
	return !(*this == rval);
}

slot & share::slot_iterator::operator * ()
{
	assert(Share);
	return *Slot;
}

boost::shared_ptr<slot> & share::slot_iterator::operator -> ()
{
	assert(Share);
	return Slot;
}

share::slot_iterator & share::slot_iterator::operator ++ ()
{
	assert(Share);
	boost::shared_ptr<slot> temp = Share->next_slot(Slot->hash);
	if(temp){
		Slot = temp;
	}else{
		Share = NULL;
	}
	return *this;
}

share::slot_iterator share::slot_iterator::operator ++ (int)
{
	assert(Share);
	slot_iterator temp = *this;
	++(*this);
	return temp;
}
//END share::slot_iterator

share::share():
	total_bytes(0),
	total_files(0)
{

}

share::const_file_iterator share::begin_file()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(Path.empty()){
		return const_file_iterator();
	}else{
		return const_file_iterator(this, *Path.begin()->second);
	}
}

share::slot_iterator share::begin_slot()
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	if(Slot.empty()){
		return slot_iterator();
	}else{
		return slot_iterator(this, Slot.begin()->second);
	}
}

boost::uint64_t share::bytes()
{
	return total_bytes;
}

share::const_file_iterator share::end_file()
{
	return const_file_iterator();
}

share::slot_iterator share::end_slot()
{
	return slot_iterator();
}

void share::erase(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);

	//locate Path element
	std::map<std::string, boost::shared_ptr<file_info> >::iterator
		Path_iter = Path.find(path);
	if(Path_iter != Path.end()){
		if(!Path_iter->second->hash.empty()){
			total_bytes -= Path_iter->second->file_size;
			--total_files;
		}

		//locate Hash element
		std::pair<std::map<std::string, boost::shared_ptr<file_info> >::iterator,
			std::map<std::string, boost::shared_ptr<file_info> >::iterator>
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
	}
}

boost::uint64_t share::files()
{
	return total_files;
}

boost::shared_ptr<slot> share::get_slot(const std::string & hash)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);

	//check for existing slot
	std::map<std::string, boost::shared_ptr<slot> >::iterator Slot_iter = Slot.find(hash);
	if(Slot_iter != Slot.end()){
		//existing slot found
		return Slot_iter->second;
	}

	//no slot yet exists, see if file with hash exists
	std::multimap<std::string, boost::shared_ptr<file_info> >::iterator
		Hash_iter = Hash.find(hash);
	if(Hash_iter == Hash.end()){
		//no file with hash exists, cannot create slot
		return boost::shared_ptr<slot>();
	}else{
		//create slot
		std::pair<std::map<std::string, boost::shared_ptr<slot> >::iterator, bool>
			ret = Slot.insert(std::make_pair(Hash_iter->first,
			boost::shared_ptr<slot>(new slot(*Hash_iter->second))));
		assert(ret.second);
		return ret.first->second;
	}
}

void share::insert_update(const file_info & FI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);

	//check if there is an existing element
	std::map<std::string, boost::shared_ptr<file_info> >::iterator
		Path_iter = Path.find(FI.path);
	if(Path_iter != Path.end()){
		//previous element found, erase it
		erase(Path_iter->second->path);
	}

	std::multimap<std::string, boost::shared_ptr<file_info> >::iterator
		Hash_iter = Hash.insert(std::make_pair(FI.hash,
		boost::shared_ptr<file_info>(new file_info(FI))));
	std::pair<std::map<std::string, boost::shared_ptr<file_info> >::iterator,
		bool> ret = Path.insert(std::make_pair(FI.path, Hash_iter->second));
	assert(ret.second);

	if(!FI.hash.empty()){
		total_bytes += FI.file_size;
		++total_files;
	}
}

share::const_file_iterator share::lookup_hash(const std::string & hash)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	std::multimap<std::string, boost::shared_ptr<file_info> >::iterator
		iter = Hash.find(hash);
	if(iter == Hash.end()){
		return end_file();
	}else{
		return const_file_iterator(this, file_info(*iter->second));
	}
}

share::const_file_iterator share::lookup_path(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	std::map<std::string, boost::shared_ptr<file_info> >::iterator
		iter = Path.find(path);
	if(iter == Path.end()){
		return end_file();
	}else{
		return const_file_iterator(this, file_info(*iter->second));
	}
}

boost::shared_ptr<const file_info> share::next_file_info(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	std::map<std::string, boost::shared_ptr<file_info> >::iterator
		iter = Path.upper_bound(path);
	if(iter == Path.end()){
		return boost::shared_ptr<file_info>();
	}else{
		//return copy
		return boost::shared_ptr<file_info>(new file_info(*iter->second));
	}
}

boost::shared_ptr<slot> share::next_slot(const std::string & hash)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	std::map<std::string, boost::shared_ptr<slot> >::iterator
		iter = Slot.upper_bound(hash);
	if(iter == Slot.end()){
		return boost::shared_ptr<slot>();
	}else{
		return iter->second;
	}
}
