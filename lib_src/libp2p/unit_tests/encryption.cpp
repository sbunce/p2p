//custom
#include "../encryption.hpp"
#include "../settings.hpp"

int fail(0);

int main()
{
	path::override_database_name("encryption.db");
	path::override_program_directory("");
	db::init::drop_all();
	db::init::create_all();

	//encryption for hosts A and B
	encryption Encryption_A;
	encryption Encryption_B;
	network::buffer buf;

	//do key exchange
	buf = Encryption_A.send_p_rA();
	if(!Encryption_B.recv_p_rA(buf)){
		LOG; ++fail;
	}
	buf = Encryption_B.send_rB();
	Encryption_A.recv_rB(buf);

	if(!Encryption_A.ready()){
		LOG; ++fail;
	}

	if(!Encryption_B.ready()){
		LOG; ++fail;
	}

	//test sending encrypted message A -> B
	std::string message = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	buf = message;
	Encryption_A.crypt_send(buf);
	Encryption_B.crypt_recv(buf);
	if(buf != message){
		LOG; ++fail;
	}

	//test sending encrypted message B -> A
	buf = message;
	Encryption_B.crypt_send(buf);
	Encryption_A.crypt_recv(buf);
	if(buf != message){
		LOG; ++fail;
	}
	return fail;
}
