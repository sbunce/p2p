#ifndef H_RC4
#define H_RC4

class RC4
{
public:
	RC4():
		seeded(false)
	{}

	void seed(unsigned char * key, unsigned key_length)
	{
		assert(key_length > 0);
		seeded = true;

		for(i=0; i < 256; ++i){
			S[i] = i;
		}

		for(i=j=0; i < 256; ++i){
			unsigned char temp;
			j = (j + key[i % key_length] + S[i]) & 255;
			temp = S[i];
			S[i] = S[j];
			S[j] = temp;
		}
		i = j = 0;

		//drop 768 bytes to avoid Fluhrer, Mantin, and Shamir attack
		for(int x=0; x<768; ++x){
			get_byte();
		}
	}

	unsigned char get_byte()
	{
		assert(seeded);

		unsigned char temp;
		i = (i + 1) & 255;
		j = (j + S[i]) & 255;
		temp = S[i];
		S[i] = S[j];
		S[j] = temp;
		return S[(S[i] + S[j]) & 255];
	}

private:
	unsigned char S[256];
	unsigned i, j;

	//true if seeded
	bool seeded;
};
#endif
