#include "CMWC4096.hpp"

CMWC4096::CMWC4096():
	idx(4095),
	State(waiting_for_seed)
{

}

void CMWC4096::extract_bytes(std::string & bytes, const int num)
{
	uint_byte UB;
	while(bytes.size() < num){
		UB.num = get_num();
		bytes.append((char *)UB.byte, 4);
	}
}

boost::uint32_t CMWC4096::get_num()
{
	assert(State == ready_to_generate);

	boost::uint64_t t, a = 18782LL;
	boost::uint32_t x, r = 0xfffffffe;
	idx = (idx + 1) & 4095;
	t = a * Q[idx] + c;
	c = (t >> 32);
	x = t + c;
	if(x < c){ ++x; ++c; }
	return (Q[idx] = r - x);
} 

void CMWC4096::seed(const std::string & bytes)
{
	assert(bytes.size() > 0);
	uint_byte UB;
	int IB_cnt = 0;
	std::string::const_iterator iter = bytes.begin();
	//fill up Q with random bytes, repeating if necessary
	for(int x=0; x<4096; ++x){
		for(int y=0; y<4; ++y){
			if(iter == bytes.end()){
				iter = bytes.begin();
			}
			UB.byte[y] = (unsigned char)*iter++;
		}
		Q[x] = UB.num;
	}

	//set c to random int between 0 and 809430660
	for(int x=0; x<4; ++x){
		if(iter == bytes.end()){
			iter = bytes.begin();
		}
		UB.byte[x] = (unsigned char)*iter++;
	}
	c = UB.num & 809430660;

	State = ready_to_generate;
}
