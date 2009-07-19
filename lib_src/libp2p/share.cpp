#include "share.hpp"

share::share()
{

}

share::~share()
{
	remove_temporary_files();
}

void share::remove_temporary_files()
{
	namespace fs = boost::filesystem;

	boost::uint64_t size;
	fs::path path = fs::system_complete(fs::path(path::temp(), fs::native));
	if(fs::exists(path)){
		try{
			fs::directory_iterator iter_cur(path), iter_end;
			while(iter_cur != iter_end){
				if(iter_cur->path().filename().find("rightside_up_", 0) == 0
					|| iter_cur->path().filename().find("upside_down_", 0) == 0)
				{
					fs::remove(iter_cur->path());
				}
				++iter_cur;
			}
		}catch(std::exception & ex){
			LOGGER << ex.what();
		}
	}
}

boost::uint64_t share::size_bytes()
{
	return Share_Pipeline_2_Write.size_bytes();
}

boost::uint64_t share::size_files()
{
	return Share_Pipeline_2_Write.size_files();
}

void share::start()
{
	Share_Pipeline_2_Write.start();
}

void share::stop()
{
	Share_Pipeline_2_Write.stop();
}
