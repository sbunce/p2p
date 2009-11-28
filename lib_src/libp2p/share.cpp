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
		return true;
	}else if(Share == NULL || rval.Share == NULL){
		return false;
	}else{
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
		return true;
	}else if(Share == NULL || rval.Share == NULL){
		return false;
	}else{
		return Slot->hash() == rval.Slot->hash();
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
	boost::shared_ptr<slot> temp = Share->next_slot(Slot->hash());
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

boost::shared_ptr<slot> share::slot_iterator::get()
{
	return Slot;
}
//END share::slot_iterator

share::share():
	_bytes(0),
	_files(0)
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
	remove_unused_slots();
	if(Slot.empty()){
		return slot_iterator();
	}else{
		return slot_iterator(this, Slot.begin()->second);
	}
}

boost::uint64_t share::bytes()
{
	return _bytes;
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
	std::map<flyweight_string, boost::shared_ptr<file_info> >::iterator
		Path_iter = Path.find(flyweight_string(path));
	if(Path_iter != Path.end()){
		if(!Path_iter->second->hash.get().empty()){
			_bytes -= Path_iter->second->file_size;
			--_files;
		}
		std::pair<std::multimap<flyweight_string, boost::shared_ptr<file_info> >::iterator,
			std::multimap<flyweight_string, boost::shared_ptr<file_info> >::iterator>
			Hash_iter_pair = Hash.equal_range(flyweight_string(Path_iter->second->hash));
		for(; Hash_iter_pair.first != Hash_iter_pair.second; ++Hash_iter_pair.first){
			if(Hash_iter_pair.first->second == Path_iter->second){
				Hash.erase(Hash_iter_pair.first);
				break;
			}
		}
		Path.erase(Path_iter);
	}
}

void share::erase(const_file_iterator CFI)
{
	erase(CFI->path);
}

boost::uint64_t share::files()
{
	return _files;
}

share::const_file_iterator share::find_hash(const std::string & hash)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	std::multimap<flyweight_string, boost::shared_ptr<file_info> >::iterator
		iter = Hash.find(flyweight_string(hash));
	if(iter == Hash.end()){
		return end_file();
	}else{
		return const_file_iterator(this, file_info(*iter->second));
	}
}

share::const_file_iterator share::find_path(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	std::map<flyweight_string, boost::shared_ptr<file_info> >::iterator
		iter = Path.find(flyweight_string(path));
	if(iter == Path.end()){
		return end_file();
	}else{
		return const_file_iterator(this, file_info(*iter->second));
	}
}

share::slot_iterator share::find_slot(const std::string & hash)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);

	//return existing slot if it exists
	std::map<flyweight_string, boost::shared_ptr<slot> >::iterator
		Slot_iter = Slot.find(flyweight_string(hash));
	if(Slot_iter != Slot.end()){
		return slot_iterator(this, Slot_iter->second);
	}

	//get file_info to create new slot, if it exists
	std::multimap<flyweight_string, boost::shared_ptr<file_info> >::iterator
		Hash_iter = Hash.find(flyweight_string(hash));
	if(Hash_iter == Hash.end()){
		return end_slot();
	}

	//create slot
	boost::shared_ptr<slot> new_slot;
	try{
		new_slot = boost::shared_ptr<slot>(new slot(*Hash_iter->second));
	}catch(std::exception & ex){
		LOGGER << ex.what();
		return end_slot();
	}
	std::pair<std::map<flyweight_string, boost::shared_ptr<slot> >::iterator, bool>
		ret = Slot.insert(std::make_pair(hash, new_slot));
	return slot_iterator(this, ret.first->second);
}

std::pair<share::const_file_iterator, bool> share::insert(const file_info & FI)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	const_file_iterator CFI = find_path(FI.path);
	if(CFI == end_file()){
		//insert new element
		std::multimap<flyweight_string, boost::shared_ptr<file_info> >::iterator
			Hash_iter = Hash.insert(std::make_pair(FI.hash,
			boost::shared_ptr<file_info>(new file_info(FI))));
		std::pair<std::map<flyweight_string, boost::shared_ptr<file_info> >::iterator,
			bool> ret = Path.insert(std::make_pair(FI.path, Hash_iter->second));
		assert(ret.second);
		_bytes += FI.file_size;
		++_files;
		return std::make_pair(const_file_iterator(this, FI), true);
	}else{
		//existing element found
		return std::make_pair(CFI, false);
	}
}

bool share::is_downloading(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	const_file_iterator CFI = find_path(path);
	if(CFI == end_file()){
		return false;
	}else{
		std::map<flyweight_string, boost::shared_ptr<slot> >::iterator
			slot_iter = Slot.find(flyweight_string(CFI->hash));
		return slot_iter != Slot.end() && !slot_iter->second->complete();
	}
}

boost::shared_ptr<const file_info> share::next_file_info(const std::string & path)
{
	boost::recursive_mutex::scoped_lock lock(Recursive_Mutex);
	std::map<flyweight_string, boost::shared_ptr<file_info> >::iterator
		iter = Path.upper_bound(flyweight_string(path));
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
	std::map<flyweight_string, boost::shared_ptr<slot> >::iterator
		iter = Slot.upper_bound(flyweight_string(hash));
	if(iter == Slot.end()){
		return boost::shared_ptr<slot>();
	}else{
		return iter->second;
	}
}

void share::remove_unused_slots()
{
	std::map<flyweight_string, boost::shared_ptr<slot> >::iterator
		iter_cur = Slot.begin(), iter_end = Slot.end();
	while(iter_cur != iter_end){
		if(iter_cur->second.unique() && iter_cur->second->complete()){
			Slot.erase(iter_cur++);
		}else{
			++iter_cur;
		}
	}
}
