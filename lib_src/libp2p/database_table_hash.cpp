#include "database_table_hash.hpp"

void database::table::hash::delete_tree(const std::string & hash,
	database::pool::proxy DB)
{
	std::stringstream ss;
	ss << "DELETE FROM hash WHERE hash = '" << hash << "'";
	DB->query(ss.str());
}

void database::table::hash::set_state(const std::string & hash,
	const state State, database::pool::proxy DB)
{
	assert(State >= RESERVED && State <= COMPLETE);
	std::stringstream ss;
	ss << "UPDATE hash SET state = " << State << " WHERE hash = '"
		<< hash << "'";
	DB->query(ss.str());
}

bool database::table::hash::tree_allocate(const std::string & hash,
	const int tree_size, database::pool::proxy DB)
{
	if(tree_size != 0){
		std::stringstream ss;
		ss << "INSERT INTO hash(key, hash, state, tree_size, tree) VALUES(NULL, '"
			<< hash << "', 0, " << tree_size << ", ?)";
		return DB->blob_allocate(ss.str(), tree_size);
	}else{
		return false;
	}
}

static int tree_open_call_back(boost::shared_ptr<database::table::hash::tree_info> & TI,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1] && response[2]);

	TI = boost::shared_ptr<database::table::hash::tree_info>(
		new database::table::hash::tree_info());

	std::stringstream ss;

	//row primary key
	ss << response[0];
	ss >> TI->Blob.rowid;
	ss.str(""); ss.clear();
	
	//tree size
	ss << response[1];
	ss >> TI->tree_size;
	ss.str(""); ss.clear();

	//state
	ss << response[2];
	int temp;
	ss >> temp;
	TI->State = reinterpret_cast<database::table::hash::state &>(temp);

	return 0;
}

boost::shared_ptr<database::table::hash::tree_info> database::table::hash::tree_open(
	const std::string & hash, database::pool::proxy DB)
{
	boost::shared_ptr<tree_info> TI;
	std::stringstream ss;
	ss << "SELECT key, tree_size, state FROM hash WHERE hash = '" << hash << "'";
	DB->query(ss.str(), &tree_open_call_back, TI);
	if(TI){
		TI->hash = hash;
		TI->Blob.table = "hash";
		TI->Blob.column = "tree";
	}
	return TI;
}
