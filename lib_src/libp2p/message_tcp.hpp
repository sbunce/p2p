#ifndef H_MESSAGE_TCP
#define H_MESSAGE_TCP

//custom
#include "file.hpp"
#include "hash_tree.hpp"
#include "protocol.hpp"

//include
#include <bit_field.hpp>
#include <boost/shared_ptr.hpp>
#include <convert.hpp>
#include <network/network.hpp>

//standard
#include <list>
#include <map>

namespace message_tcp{
namespace recv{

enum status{
	blacklist,   //host needs to be blacklisted
	incomplete,  //incomplete message on front of buf
	complete,    //received complete message (stored in base::buf)
	not_expected //messages doesn't expect bytes on front of buf
};

class base
{
public:
	//speed calculator for download (empty if no speed to track)
	boost::shared_ptr<network::speed_calculator> Download_Speed;

	/*
	expect:
		Returnst true if we expect message on front of recv_buf.
	recv:
		Tries to parse message on front of buffer. Returns a status.  Message is
		removed from front of recv_buf if true returned.
		Note: It is not necessary to call expect() before this.
	*/
	virtual bool expect(network::buffer & recv_buf) = 0;
	virtual status recv(network::buffer & recv_buf) = 0;
};

class block : public base
{
public:
	typedef boost::function<bool (network::buffer & block)> handler;
	block(handler func_in, const unsigned block_size_in,
		boost::shared_ptr<network::speed_calculator> Download_Speed_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
	unsigned block_size; //size of block field
	unsigned bytes_seen; //used to update speed
};

class close_slot : public base
{
public:
	typedef boost::function<bool (const unsigned char slot_num)> handler;
	explicit close_slot(handler func_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
};

//the composite message is used to handle multiple possible responses
class composite : public base
{
public:
	//add messages to expect() and recv()
	void add(boost::shared_ptr<base> M);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	std::vector<boost::shared_ptr<base> > possible_response;
};

class error : public base
{
public:
	typedef boost::function<bool ()> handler;
	explicit error(handler func_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
};

class have_file_block : public base
{
public:
	typedef boost::function<bool (const unsigned char slot_num,
		const boost::uint64_t block_num)> handler;
	have_file_block(handler func_in,
		const unsigned char slot_num_in, const boost::uint64_t file_block_count_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
	const unsigned char slot_num;
	const boost::uint64_t file_block_count;
};

class have_hash_tree_block : public base
{
public:
	typedef boost::function<bool (const unsigned char slot_num,
		const boost::uint64_t block_num)> handler;
	have_hash_tree_block(handler func_in,
		const unsigned char slot_num_in, const boost::uint64_t tree_block_count_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
	const unsigned char slot_num;
	const boost::uint64_t tree_block_count;
};

class initial : public base
{
public:
	typedef boost::function<bool (const std::string & ID)> handler;
	explicit initial(handler func_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
};

class key_exchange_p_rA : public base
{
public:
	typedef boost::function<bool (network::buffer &)> handler;
	explicit key_exchange_p_rA(handler func_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
};

class key_exchange_rB : public base
{
public:
	typedef boost::function<bool (network::buffer &)> handler;
	explicit key_exchange_rB(handler func_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
};

class request_file_block : public base
{
public:
	typedef boost::function<bool (const unsigned char slot_num,
		const boost::uint64_t block_num)> handler;
	request_file_block(handler func_in,
		const unsigned char slot_num_in, const boost::uint64_t file_block_count);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
	unsigned char slot_num;
	unsigned VLI_size;
};

class request_hash_tree_block : public base
{
public:
	typedef boost::function<bool (const unsigned char slot_num,
		const boost::uint64_t block_num)> handler;
	request_hash_tree_block(handler func_in, const unsigned char slot_num_in,
		const boost::uint64_t tree_block_count);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
	unsigned char slot_num;
	unsigned VLI_size;
};

class request_slot : public base
{
public:
	typedef boost::function<bool (const std::string & hash)> handler;
	explicit request_slot(handler func_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
};

class slot : public base
{
public:
	typedef boost::function<bool (const unsigned char slot_num,
		const boost::uint64_t file_size, const std::string & root_hash,
		bit_field & tree_BF, bit_field & file_BF)> handler;
	slot(handler func_in, const std::string & hash_in);
	virtual bool expect(network::buffer & recv_buf);
	virtual status recv(network::buffer & recv_buf);
private:
	handler func;
	std::string hash;
	bool checked;     //true when file_size/root_hash checked
};

}//end of namespace recv

namespace send{

class base
{
public:
	//contains bytes to send
	network::buffer buf;

	//speed calculator for upload (empty if no speed to track)
	boost::shared_ptr<network::speed_calculator> Upload_Speed;

	/*
	encrypt:
		Returns true if message should be encrypted before sending. The default is
		true. The key_exchange messages override this and return false.
	*/
	virtual bool encrypt();
};

class block : public base
{
public:
	block(network::buffer & block,
		boost::shared_ptr<network::speed_calculator> Upload_Speed_in);
};

class close_slot : public base
{
public:
	close_slot(const unsigned char slot_num);
};

class error : public base
{
public:
	error();
};

class have_file_block : public base
{
public:
	have_file_block(const unsigned char slot_num,
		const boost::uint64_t block_num, const boost::uint64_t file_block_count);
};

class have_hash_tree_block : public base
{
public:
	have_hash_tree_block(const unsigned char slot_num,
		const boost::uint64_t block_num, const boost::uint64_t tree_block_count);
};

class initial : public base
{
public:
	explicit initial(const std::string & ID);
};

class key_exchange_p_rA : public base
{
public:
	explicit key_exchange_p_rA(const network::buffer & buf_in);
	virtual bool encrypt();
};

class key_exchange_rB : public base
{
public:
	explicit key_exchange_rB(const network::buffer & buf_in);
	virtual bool encrypt();
};

class request_file_block : public base
{
public:
	request_file_block(const unsigned char slot_num,
		const boost::uint64_t block_num, const boost::uint64_t file_block_count);
};

class request_hash_tree_block : public base
{
public:
	request_hash_tree_block(const unsigned char slot_num,
		const boost::uint64_t block_num, const boost::uint64_t tree_block_count);
};

class request_slot : public base
{
public:
	explicit request_slot(const std::string & hash);
};

class slot : public base
{
public:
	slot(const unsigned char slot_num, const boost::uint64_t file_size,
		const std::string & root_hash, const bit_field & tree_BF,
		const bit_field & file_BF);
};

}//end of namespace send
}//end namespace message
#endif
