//include
#include <buffer.hpp>
#include <logger.hpp>

int main()
{
	buffer Buffer;
	for(int x=0; x<163; ++x){
		Buffer.append('x');
	}
	Buffer.tail_reserve(4096);
	for(int x=0; x<4096; ++x){
		Buffer.tail_start()[x] = 'x';
	}
}
