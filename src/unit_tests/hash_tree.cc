//custom
#include "../global.h"
#include "../hash_tree.h"
#include "../logger.h"

void create_test_file(const std::string & name, const unsigned & bytes)
{
	std::fstream f((global::SHARE_DIRECTORY + name).c_str(), std::ios::in);
	if(f.is_open()){
		return;
	}

	f.open((global::SHARE_DIRECTORY + name).c_str(), std::ios::out | std::ios::binary);
	f.clear();
	for(int x=0; x<bytes; ++x){
		if(x % 80 == 0 && x != 0){
			f.put('\n');
		}else{
			f.put('0');
		}
	}
	f.flush();
}

int main()
{

	boost::filesystem::create_directory(global::SHARE_DIRECTORY.c_str());
	database DB;
	DB_hash DB_Hash(DB);
	hash_tree Hash_Tree;

	create_test_file("1_block", 1 * global::FILE_BLOCK_SIZE);
	create_test_file("2_block", 2 * global::FILE_BLOCK_SIZE);
	create_test_file("3_block", 3 * global::FILE_BLOCK_SIZE);
	create_test_file("4_block", 4 * global::FILE_BLOCK_SIZE);
	create_test_file("256_block", 256 * global::FILE_BLOCK_SIZE);
	create_test_file("257_block", 257 * global::FILE_BLOCK_SIZE);

	std::string root_hash;
	boost::uint64_t bad_block;
	if(!Hash_Tree.create(global::SHARE_DIRECTORY + "1_block", root_hash)){
		LOGGER; exit(1);
	}
	hash_tree::tree_info TE_1(root_hash, 1 * global::FILE_BLOCK_SIZE);
	if(!Hash_Tree.check(TE_1, bad_block)){
		LOGGER; exit(1);
	}

	if(!Hash_Tree.create(global::SHARE_DIRECTORY + "2_block", root_hash)){
		LOGGER; exit(1);
	}

	hash_tree::tree_info TE_2(root_hash, 2 * global::FILE_BLOCK_SIZE);
	if(!Hash_Tree.check(TE_2, bad_block)){
		LOGGER; exit(1);
	}

	if(!Hash_Tree.create(global::SHARE_DIRECTORY + "3_block", root_hash)){
		LOGGER; exit(1);
	}
	hash_tree::tree_info TE_3(root_hash, 3 * global::FILE_BLOCK_SIZE);
	if(!Hash_Tree.check(TE_3, bad_block)){
		LOGGER; exit(1);
	}

	if(!Hash_Tree.create(global::SHARE_DIRECTORY + "4_block", root_hash)){
		LOGGER; exit(1);
	}
	hash_tree::tree_info TE_4(root_hash, 4 * global::FILE_BLOCK_SIZE);
	if(!Hash_Tree.check(TE_4, bad_block)){
		LOGGER; exit(1);
	}

	if(!Hash_Tree.create(global::SHARE_DIRECTORY + "256_block", root_hash)){
		LOGGER; exit(1);
	}
	hash_tree::tree_info TE_256(root_hash, 256 * global::FILE_BLOCK_SIZE);
	if(!Hash_Tree.check(TE_256, bad_block)){
		LOGGER; exit(1);
	}

	if(!Hash_Tree.create(global::SHARE_DIRECTORY + "257_block", root_hash)){
		LOGGER; exit(1);
	}
	hash_tree::tree_info TE_257(root_hash, 257 * global::FILE_BLOCK_SIZE);
	if(!Hash_Tree.check(TE_257, bad_block)){
		LOGGER; exit(1);
	}

	return 0;
}
