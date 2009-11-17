#include "database_table_hash_tree.hpp"

bool database::table::hash_tree::add(const std::string & hash,
	const boost::uint64_t & tree_size, database::pool::proxy DB)
{
	if(tree_size == 0 || tree_size > 2147483647ULL){
		return false;
	}else{
		std::stringstream ss;
		ss << "INSERT INTO hash_tree(key, hash, state, tree) VALUES(NULL, '"
			<< hash << "', " << reserved << ", ?)";
		return DB->blob_allocate(ss.str(), tree_size);
	}
}

static int lookup_call_back(int columns, char ** response, char ** column_name,
	boost::shared_ptr<database::table::hash_tree::info> & Info)
{
	assert(columns == 2);
	assert(std::strcmp(column_name[0], "key") == 0);
	assert(std::strcmp(column_name[1], "state") == 0);
	Info = boost::shared_ptr<database::table::hash_tree::info>(
		new database::table::hash_tree::info());
	try{
		Info->blob.rowid = boost::lexical_cast<boost::int64_t>(response[0]);
		int temp = boost::lexical_cast<int>(response[1]);
		Info->tree_state = reinterpret_cast<database::table::hash_tree::state &>(temp);
	}catch(const std::exception & e){
		LOGGER << e.what();
		Info = boost::shared_ptr<database::table::hash_tree::info>();
	}
	return 0;
}

boost::shared_ptr<database::table::hash_tree::info> database::table::hash_tree::lookup(
	const std::string & hash, database::pool::proxy DB)
{
	boost::shared_ptr<info> Info;
	std::stringstream ss;
	ss << "SELECT key, state FROM hash_tree WHERE hash = '" << hash << "' LIMIT 1";
	DB->query(ss.str(), boost::bind(&lookup_call_back, _1, _2, _3, boost::ref(Info)));
	if(Info){
		Info->hash = hash;
		Info->blob.table = "hash_tree";
		Info->blob.column = "tree";
	}else{
		return boost::shared_ptr<info>();
	}
	if(DB->blob_size(Info->blob, Info->tree_size)){
		return Info;
	}else{
		return boost::shared_ptr<info>();
	}
}

void database::table::hash_tree::remove(const std::string & hash,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "DELETE FROM hash_tree WHERE hash = '" << hash << "'";
	DB->query(ss.str());
}

void database::table::hash_tree::set_state(const std::string & hash,
	const state tree_state, database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "UPDATE hash_tree SET state = " << tree_state << " WHERE hash = '" << hash << "'";
	DB->query(ss.str());
}
