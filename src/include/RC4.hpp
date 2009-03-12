#ifndef H_RC4
#define H_RC4

class RC4
{
public:
	RC4(){}

	void seed(unsigned char * key, unsigned key_length)
	{
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
	}

	unsigned char get_byte()
	{
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
};
#endif
