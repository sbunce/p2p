#ifndef H_PATH
#define H_PATH

//include
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <logger.hpp>

//standard
#include <cstdlib>
#include <sstream>
#include <string>

class path
{
public:
	/*
	create_required_directories:
		Creates directories different parts of the program expect. Called from the
		p2p_real ctor.
	remove_temporary_hash_tree_files:
		Remove all temporary files used for generating hash trees. This is called
		by the share dtor and hash_tree unit test.
	*/
	static void create_required_directories();
	static void remove_temporary_hash_tree_files();

	/* Files
	database:
		Path to database.
	hash_tree_temp:
		Temporary location of hash tree. The thread id is appended so different
		threads will get directed to different files.
	*/
	static std::string database();
	static std::string hash_tree_temp();

	/* Directories
	download:
		Directory for finished downloads.
	share:
		Directory for shared files.
	temp:
		Returns location of temporary directory.
	*/
	static std::string download();
	static std::string share();
	static std::string temp();

	/* Testing
	Note: These functions are not thread safe.
	override_DB_name:
		Override the name of the database file.
	override_program_directory:
		Override path of program directory.
	*/
	static void override_database_name(const std::string & DB_name);
	static void override_program_directory(const std::string & path);

private:
	path(){}

	/*
	The init function is called with boost::call_once at the top of every
	function to make sure the static variables are initialized in a thread safe
	way.
	*/
	static boost::once_flag once_flag;
	static void init();

	static std::string & database_name();
	static std::string & program_directory();
};
#endif
