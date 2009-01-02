#include "DB_download.h"

DB_download::DB_download()
: DB(global::DATABASE_PATH)
{
	DB.query("CREATE TABLE IF NOT EXISTS download (hash TEXT, name TEXT, size TEXT, server TEXT)");
}

//std::pair<true if exists, file_size>
static int lookup_hash_call_back_0(std::pair<bool, boost::uint64_t *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	std::stringstream ss(response[0]);
	ss >> *info.second;
	return 0;
}

bool DB_download::lookup_hash(const std::string & hash, boost::uint64_t & file_size)
{
	std::pair<bool, boost::uint64_t *> info(false, &file_size);
	std::ostringstream query;
	query << "SELECT size FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str(), &lookup_hash_call_back_0, info);
	return info.first;
}

//std::pair<true if exists, file_path>
static int lookup_hash_call_back_1(std::pair<bool, std::string *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0]);
	info.first = true;
	*info.second = global::DOWNLOAD_DIRECTORY + response[0];
	return 0;
}

bool DB_download::lookup_hash(const std::string & hash, std::string & path)
{
	std::pair<bool, std::string *> info(false, &path);
	std::ostringstream query;
	query << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str(), &lookup_hash_call_back_1, info);
	return info.first;
}


//boost::tuple<true if exists, path, file_size>
static int lookup_hash_call_back_2(boost::tuple<bool, std::string *, boost::uint64_t *> & info,
	int columns_retrieved, char ** response, char ** column_name)
{
	assert(response[0] && response[1]);
	info.get<0>() = true;
	*info.get<1>() = global::DOWNLOAD_DIRECTORY + response[0];
	std::stringstream ss(response[1]);
	ss >> *info.get<2>();
	return 0;
}

bool DB_download::lookup_hash(const std::string & hash, std::string & path, boost::uint64_t & file_size)
{
	boost::tuple<bool, std::string *, boost::uint64_t *> info(false, &path, &file_size);
	std::ostringstream query;
	query << "SELECT name, size FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str(), &lookup_hash_call_back_2, info);
	return info.get<0>();
}

int DB_download::resume_call_back(std::vector<download_info> & resume, int columns_retrieved,
	char ** response, char ** column_name)
{
	namespace fs = boost::filesystem;
	assert(response[0] && response[1] && response[2] && response[3]);
	std::string path(response[1]);
	path = global::DOWNLOAD_DIRECTORY + path;
	std::fstream fstream_temp(path.c_str());
	if(fstream_temp.is_open()){
		//file exist in download directory
		fstream_temp.close();
		fs::path path_boost = fs::system_complete(
			fs::path(global::DOWNLOAD_DIRECTORY + response[1], fs::native));
		std::stringstream ss(response[2]);
		boost::uint64_t size;
		ss >> size;
		resume.push_back(download_info(
			response[0], //hash
			response[1], //name
			size         //size
		));

		//tokenize servers
		char delims[] = ";";
		char * result = NULL;
		result = strtok(response[3], delims);
		while(result != NULL){
			resume.back().IP.push_back(result);
			result = strtok(NULL, delims);
		}
	}else{
		//partial file removed, delete entry from database
		std::ostringstream query;
		query << "DELETE FROM download WHERE name = '" << response[1] << "'";
		DB.query(query.str());
	}
	return 0;
}

void DB_download::resume(std::vector<download_info> & resume)
{
	DB.query("SELECT hash, name, size, server FROM download",
		this, &DB_download::resume_call_back, resume);
}

static int is_downloading_call_back(bool & downloading, int columns_retrieved, char ** response, char ** column_name)
{
	downloading = true;
	return 0;
}

bool DB_download::is_downloading(const std::string & hash)
{
	bool downloading = false;
	std::ostringstream query;
	query << "SELECT 1 FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str(), &is_downloading_call_back, downloading);
	return downloading;
}

bool DB_download::start_download(const download_info & info)
{
	if(is_downloading(info.hash)){
		return false;
	}else{
		std::ostringstream query;
		query << "INSERT INTO download (hash, name, size, server) VALUES ('" << info.hash << "', '" << info.name << "', " << info.size << ", '";
		for(int x = 0; x < info.IP.size(); ++x){
			if(x+1 == info.IP.size()){
				query << info.IP[x];
			}else{
				query << info.IP[x] << ";";
			}
		}
		query << "')";
		DB.query(query.str());
		return true;
	}
}

void DB_download::terminate_download(const std::string & hash)
{
	std::ostringstream query;
	query << "DELETE FROM download WHERE hash = '" << hash << "'";
	DB.query(query.str());
}
