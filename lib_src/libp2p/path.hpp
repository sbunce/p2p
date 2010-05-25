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
	create_dirs:
		Creates directories different parts of the program expect.
	remove_temporary_hash_tree_files:
		Remove all temporary files used for generating hash trees.
	set_db_file_name:
		Set name of database file.
		Note: This must be called before other functions in this class called.
		Note: Not thread safe.
	set_program_directory:
		Change the program directory.
		Note: This must be called before other functions in this class called.
		Note: Not thread safe.
	*/
	static void create_dirs();
	static void remove_tmp_tree_files();
	static void set_db_file_name(const std::string & name);
	static void set_program_dir(const std::string & path);

	/* Files
	db_file:
		Path to database file.
	download_dir:
		Path to download directory.
	share_dir:
		Path to share directory.
	tmp_dir:
		Path to temporary file directory.
	tree_file:
		Path to temporary hash tree file.
		Note: Thread ID appended for uniqueness.
	*/
	static std::string db_file();
	static std::string download_dir();
	static std::string share_dir();
	static std::string tmp_dir();
	static std::string tree_file();

private:
	path(){}

	class static_wrap
	{
	public:
		class static_objects
		{
		public:
			static_objects();
			std::string db_file_name;
			std::string program_dir;
		};

		//get access to static objects
		static static_objects & get();
	private:
		static boost::once_flag once_flag;
		static static_objects & _get();
	};
};
#endif
