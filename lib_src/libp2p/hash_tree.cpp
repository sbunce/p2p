#include "hash_tree.hpp"

//BEGIN static_wrap
boost::once_flag hash_tree::static_wrap::once_flag = BOOST_ONCE_INIT;

hash_tree::static_wrap::static_objects::static_objects():
	stopped(false)
{

}

hash_tree::static_wrap::static_objects & hash_tree::static_wrap::get()
{
	boost::call_once(once_flag, &_get);
	return _get();
}

hash_tree::static_wrap::static_objects & hash_tree::static_wrap::_get()
{
	static static_objects SO;
	return SO;
}
//END static_wrap

hash_tree::hash_tree(const file_info & FI):
	TI(FI)
{
	assert(TI.hash.size() == SHA1::hex_size);
	boost::shared_ptr<db::table::hash::info> info = db::table::hash::find(TI.hash);
	if(info){
		//opened existing hash tree
		boost::uint64_t size;
		if(!db::pool::get()->blob_size(info->blob, size)){
			throw std::runtime_error("error checking tree size");
		}
		//only open tree if the size is what we expect
		if(TI.tree_size != size){
			/*
			It is possible someone might encounter two files of different sizes
			that have the same hash. We don't account for this because it's so
			rare. We just fail to create a slot for one of the files.
			*/
			throw std::runtime_error("incorrect tree size");
		}
		const_cast<db::blob &>(blob) = info->blob;
	}else{
		//allocate space to reconstruct hash tree
		if(db::table::hash::add(TI.hash, TI.tree_size)){
			db::table::hash::set_state(TI.hash, db::table::hash::downloading);
			info = db::table::hash::find(TI.hash);
			if(info){
				const_cast<db::blob &>(blob) = info->blob;
			}else{
				throw std::runtime_error("failed hash tree open");
			}
		}else{
			throw std::runtime_error("failed hash tree allocate");
		}
	}
}

hash_tree::status hash_tree::check() const
{
	net::buffer buf;
	buf.reserve(protocol_tcp::file_block_size);
	for(boost::uint32_t block_num=0; block_num<TI.tree_block_count; ++block_num){
		buf.clear();
		status Status = read_block(block_num, buf);
		if(Status == bad){
			return io_error;
		}
		Status = check_tree_block(block_num, buf);
		if(Status == bad){
			return bad;
		}else if(Status == io_error){
			return io_error;
		}else if(block_num == TI.tree_block_count - 1){
			//last block checked good
			db::table::hash::set_state(TI.hash, db::table::hash::complete);
		}
	}
	return good;
}

hash_tree::status hash_tree::check_file_block(const boost::uint64_t file_block_num,
	const net::buffer & buf) const
{
	char parent_buf[SHA1::bin_size];
	if(!db::pool::get()->blob_read(blob, parent_buf,
		SHA1::bin_size, TI.file_hash_offset + file_block_num * SHA1::bin_size))
	{
		return io_error;
	}
	SHA1 SHA(reinterpret_cast<const char *>(buf.data()),
		buf.size());
	if(std::memcmp(parent_buf, SHA.bin(), SHA1::bin_size) == 0){
		return good;
	}else{
		return bad;
	}
}

hash_tree::status hash_tree::check_tree_block(const boost::uint64_t block_num,
	const net::buffer & buf) const
{
	if(block_num == 0){
		//special requirements to check root_hash, see documentation for root_hash
		assert(buf.size() == SHA1::bin_size);
		char special[SHA1::bin_size + 8];
		std::memcpy(special, convert::int_to_bin(TI.file_size).data(), 8);
		std::memcpy(special + 8, buf.data(), SHA1::bin_size);
		SHA1 SHA(special, SHA1::bin_size + 8);
		return SHA.hex() == TI.hash ? good : bad;
	}else{
		std::pair<boost::uint64_t, unsigned> info;
		boost::uint64_t parent;
		if(!TI.block_info(block_num, info, parent)){
			//invalid block
			LOG << "invalid block";
			exit(1);
		}
		assert(buf.size() == info.second);
		//create hash for children
		SHA1 SHA(reinterpret_cast<const char *>(buf.data()),
			buf.size());
		//verify parent hash is a hash of the children
		char parent_hash[SHA1::bin_size];
		if(!db::pool::get()->blob_read(blob, parent_hash, SHA1::bin_size, parent)){
			return io_error;
		}
		if(std::memcmp(parent_hash, SHA.bin(), SHA1::bin_size) == 0){
			return good;
		}else{
			return bad;
		}
	}
}

hash_tree::status hash_tree::create(file_info & FI)
{
	tree_info TI(FI);
	assert(TI.hash.empty());

	//open file to generate hash tree for
	std::fstream file(FI.path.c_str(), std::ios::in | std::ios::binary);
	if(!file.is_open()){
		LOG << "error opening " << FI.path;
		return io_error;
	}

	/*
	Open tmp file to write hash tree to. A tmp file is used to avoid database
	contention.
	*/
	std::fstream tmp(path::tree_file().c_str(), std::ios::in | std::ios::out
		| std::ios::trunc | std::ios::binary);
	if(!tmp.is_open()){
		LOG << "error opening tmp file";
		return io_error;
	}

	char buf[protocol_tcp::file_block_size];
	SHA1 SHA;

	//do file hashes
	std::time_t time(0);
	for(boost::uint64_t x=0; x<TI.row.back(); ++x){
		//check once per second to make sure file not copying
		if(std::time(NULL) != time){
			try{
				if(TI.file_size < boost::filesystem::file_size(FI.path)){
					LOG << "copying " << FI.path;
					return copying;
				}
				time = std::time(NULL);
			}catch(const std::exception & e){
				LOG << "error reading " << FI.path;
				return io_error;
			}
		}

		if(static_wrap::get().stopped){
			return io_error;
		}
		file.read(buf, protocol_tcp::file_block_size);
		if(file.gcount() == 0){
			LOG << "error reading " << FI.path;
			return io_error;
		}
		SHA.init();
		SHA.load(buf, file.gcount());
		SHA.end();
		tmp.seekp(TI.file_hash_offset + x * SHA1::bin_size, std::ios::beg);
		tmp.write(SHA.bin(), SHA1::bin_size);
		if(!tmp.good()){
			LOG << "error writing tmp file";
			return io_error;
		}
	}

	//do all other tree hashes
	for(boost::uint64_t x=TI.tree_block_count - 1; x>0; --x){
		std::pair<boost::uint64_t, unsigned> info;
		boost::uint64_t parent;
		if(!TI.block_info(x, info, parent)){
			LOG << "invalid block";
			exit(1);
		}
		tmp.seekg(info.first, std::ios::beg);
		tmp.read(buf, info.second);
		if(tmp.gcount() != info.second){
			LOG << "error reading tmp file";
			return io_error;
		}
		SHA.init();
		SHA.load(buf, info.second);
		SHA.end();
		tmp.seekp(parent, std::ios::beg);
		tmp.write(SHA.bin(), SHA1::bin_size);
		if(!tmp.good()){
			LOG << "error writing tmp file";
			return io_error;
		}
	}

	//calculate hash
	std::memcpy(buf, convert::int_to_bin(TI.file_size).data(), 8);
	std::memcpy(buf + 8, SHA.bin(), SHA1::bin_size);
	SHA.init();
	SHA.load(buf, SHA1::bin_size + 8);
	SHA.end();
	FI.hash = SHA.hex();

	/*
	Copy hash tree in to database. No transaction is used because it this can
	tie up the database for too long when copying a large hash tree. The
	writes are already quite large so a transaction doesn't make much
	performance difference.
	*/

	//check if tree already exists
	if(db::table::hash::find(FI.hash)){
		return good;
	}

	//tree doesn't exist, allocate space for it
	if(!db::table::hash::add(FI.hash, TI.tree_size)){
		LOG << "error adding hash tree";
		return io_error;
	}

	//copy tree to database using large buffer for performance
	boost::shared_ptr<db::table::hash::info> Info = db::table::hash::find(FI.hash);

	tmp.seekg(0, std::ios::beg);
	boost::uint64_t offset = 0, bytes_remaining = TI.tree_size, read_size;
	while(bytes_remaining){
		if(boost::this_thread::interruption_requested()){
			db::table::hash::remove(FI.hash);
			return io_error;
		}
		if(bytes_remaining > protocol_tcp::file_block_size){
			read_size = protocol_tcp::file_block_size;
		}else{
			read_size = bytes_remaining;
		}
		tmp.read(buf, read_size);
		if(tmp.gcount() != read_size){
			LOG << "error reading tmp file";
			db::table::hash::remove(TI.hash);
			return io_error;
		}else{
			if(!db::pool::get()->blob_write(Info->blob, buf, read_size, offset)){
				LOG << "error writing blob";
				db::table::hash::remove(FI.hash);
				return io_error;
			}
			offset += read_size;
			bytes_remaining -= read_size;
		}
	}
	return good;
}

hash_tree::status hash_tree::read_block(const boost::uint64_t block_num,
	net::buffer & buf) const
{
	std::pair<boost::uint64_t, unsigned> info;
	if(TI.block_info(block_num, info)){
		buf.tail_reserve(info.second);
		if(!db::pool::get()->blob_read(blob, reinterpret_cast<char *>(buf.tail_start()),
			info.second, info.first))
		{
			buf.tail_resize(0);
			return io_error;
		}
		buf.tail_resize(info.second);
		return good;
	}else{
		LOG << "invalid block";
		exit(1);
	}
}

boost::optional<std::string> hash_tree::root_hash() const
{
	char buf[8 + SHA1::bin_size];
	std::memcpy(buf, convert::int_to_bin(TI.file_size).data(), 8);
	if(!db::pool::get()->blob_read(blob, buf+8, SHA1::bin_size, 0)){
		return boost::optional<std::string>();
	}
	SHA1 SHA(buf, 8 + SHA1::bin_size);
	if(SHA.hex() != TI.hash){
		return boost::optional<std::string>();
	}
	return convert::bin_to_hex(std::string(buf+8, SHA1::bin_size));
}

void hash_tree::stop_create()
{
	static_wrap::get().stopped = true;
}

hash_tree::status hash_tree::write_block(const boost::uint64_t block_num,
	const net::buffer & buf)
{
	std::pair<boost::uint64_t, unsigned> info;
	boost::uint64_t parent;
	if(TI.block_info(block_num, info, parent)){
		assert(info.second == buf.size());
		if(block_num != 0){
			char parent_buf[SHA1::bin_size];
			if(!db::pool::get()->blob_read(blob, parent_buf, SHA1::bin_size, parent)){
				return io_error;
			}
			SHA1 SHA(reinterpret_cast<const char *>(buf.data()),
				info.second);
			if(std::memcmp(parent_buf, SHA.bin(), SHA1::bin_size) != 0){
				return bad;
			}
		}
		if(!db::pool::get()->blob_write(blob,
			reinterpret_cast<const char *>(buf.data()), buf.size(), info.first))
		{
			return io_error;
		}
		return good;
	}else{
		LOG << "invalid block";
		exit(1);
	}
}
