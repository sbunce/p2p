/*
This program is a C++ implementation of the Secure Hash Algorithm (SHA)
that handles the variations from the original 160 bit to 224, 256, 384
and 512 bit.  The program is intended to be platform independant and
has been tested on little-endian (Intel) and big-endian (Sun) machines.

This program is based on a C version written by Aaron D. Gifford
(as of 11/22/2004 his code could be found at http://www.adg.us/computers/sha.html).
Attempts to contact him were unsuccessful.	I greatly condensed his version
and shared as much code and data as I could think of.  I also inlined
a lot of code that were macros in his version.  My version detects
endian-ness automatically and adjusts itself accordingly.  This program
has been tested with Visual C++ versions 6/7 and Dev-C++ on Windows,
g++ on Linux and CC on Solaris (g++ on Solaris gave a bus error).

While I did make half-hearted attempts to optimize as I went along
(testing on Wintel), any serious attempt at fast implementation is
probably going to need to make use of in-lined assembly which is not
very portable.

The goal of this implementation is ease of use.  As much as possible
I tried to hide implementation details while making it trivial to change
the size of the hash and get the results.  The string and charactar
array value of the hash is supplied as human-readable hex; the raw value
can also be obtained.

If you use this implementation somewhere I would like to be credited
with my work (a link to my page below is fine).  I add no license
restriction beyond any that is made by the original author.  This
code comes with no warrenty expressed or implied, use at your own
risk!

Keith Oxenrider
koxenrider[at]sol[dash]biotech[dot]com
The latest version of this code should be available via the page
sol-biotech.com/code.
*/

#ifndef H_SHA2
#define H_SHA2

#include <stdexcept>

//NOTE: You may need to define things by hand for your system:
typedef unsigned char sha_byte;	//exactly 1 byte
typedef unsigned int sha_word32;	//exactly 4 bytes

#ifdef WIN32
	#include <windows.h>
	typedef ULONG64 sha_word64; //8-bytes (64-bits)
#else
	typedef unsigned long long sha_word64; //8-bytes (64-bits)
#endif

//digest lengths for SHA-1/224/256/384/512
const sha_word32 SHA1_DIGESTC_LENGTH = 20;
const sha_word32 SHA1_DIGESTC_STRING_LENGTH   = (SHA1_DIGESTC_LENGTH   * 2 + 1);
const sha_word32 SHA224_DIGESTC_LENGTH = 28;
const sha_word32 SHA224_DIGESTC_STRING_LENGTH = (SHA224_DIGESTC_LENGTH * 2 + 1);
const sha_word32 SHA256_DIGESTC_LENGTH = 32;
const sha_word32 SHA256_DIGESTC_STRING_LENGTH = (SHA256_DIGESTC_LENGTH * 2 + 1);
const sha_word32 SHA384_DIGESTC_LENGTH = 48;
const sha_word32 SHA384_DIGESTC_STRING_LENGTH = (SHA384_DIGESTC_LENGTH * 2 + 1);
const sha_word32 SHA512_DIGESTC_LENGTH = 64;
const sha_word32 SHA512_DIGESTC_STRING_LENGTH = (SHA512_DIGESTC_LENGTH * 2 + 1);

class sha
{
public:
	sha();

	enum SHA_TYPE{
		enuSHA_NONE,
		enuSHA1,
		enuSHA160 = enuSHA1,
		enuSHA224,
		enuSHA256,
		enuSHA384,
		enuSHA512,
		enuSHA_LAST
	};

	SHA_TYPE GetEnumType()
	{
		return m_Type;
	};
	bool IsBigEndian()
	{
		return m_boolIsBigEndian;
	};
	const char * GetTypeString()
	{
		switch (m_Type){
		case sha::enuSHA1	: return "SHA160";
		case sha::enuSHA224 : return "SHA224";
		case sha::enuSHA256 : return "SHA256";
		case sha::enuSHA384 : return "SHA384";
		case sha::enuSHA512 : return "SHA512";
		default : return "Unknown!";
		}
	};

	/*
	These three functions can be called in order to calculate a hash for multiple
	chunks. The update() function can be called as many times as needed. The init()
	function must be called before every new set of updates.
	*/
	void init(SHA_TYPE type);
	void update(const sha_byte *data, size_t len);
	void end();

	/*
	hex_hash         - returns hexadecimal representation of the hash, NULL terminated
	string_hash      - returns hexadecimal representation of the hash
	raw_hash         - returns raw hash, NOT NULL TERMINATED
	raw_hash_no_null - returns raw hash, substitutes a hash of all (char)1 in place of hash of all (char)0
	string_raw_hash  - returns the raw hash in a std::string
	*/
	const char * hex_hash();
	const std::string & string_hex_hash();
	const char * raw_hash();
	const char * raw_hash_no_null();
	const std::string & string_raw_hash();

	//returns how many bytes the binary representation of the hash takes up
	const int & hash_length();

private:
	SHA_TYPE m_Type;
	std::string m_strHash;
	std::string m_strRawHash;
	bool m_boolEnded, m_boolIsBigEndian;
	char m_chrRawHash[SHA512_DIGESTC_LENGTH];
	char m_chrHexHash[SHA512_DIGESTC_STRING_LENGTH];
	sha_byte m_digest[SHA512_DIGESTC_LENGTH];

	int hash_len; //length of current initialized hash type

	//these are common buffers for maintaining the hash
	struct SHA_CTX{
		sha_byte state[sizeof(sha_word64) * 8]; //maximum size
		sha_word64 bitcount[2]; //sha1, 224 and 256 only use the first entry
		sha_byte buffer[128];
	}ctx;

	void SHA256_Internal_Last(bool isSha1 = false);
	void SHA512_Internal_Last();

	void SHA1_Internal_Transform(const sha_word32 *data);
	void SHA256_Internal_Transform(const sha_word32* data);
	void SHA512_Internal_Transform(const sha_word64*);

	void SHA32bit_Update(const sha_byte *data, size_t len, bool isSha1=false);
	void SHA64bit_Update(const sha_byte *data, size_t len);

	//macro replacements
	inline void MEMSET_BZERO(void *p, size_t l){memset(p, 0, l);};
	inline void MEMCPY_BCOPY(void *d,const void *s, size_t l) {memcpy(d, s, l);};

	/*
	For incrementally adding the unsigned 64-bit integer n to the
	unsigned 128-bit integer (represented using a two-element array of
	64-bit words).
	*/
	inline void ADDINC128(sha_word64 *w, sha_word32 n)	{
		w[0] += (sha_word64)(n);
		if(w[0] < (n)){
			++w[1];
		}
	}

	//shift-right (used in SHA-256, SHA-384, and SHA-512)
	inline sha_word32 SHR(sha_word32 b,sha_word32 x){return (x >> b);};
	inline sha_word64 SHR(sha_word64 b,sha_word64 x){return (x >> b);};
	//32-bit Rotate-right (used in SHA-256)
	inline sha_word32 ROTR32(sha_word32 b,sha_word32 x){return ((x >> b) | (x << (32 - b)));};
	//64-bit Rotate-right (used in SHA-384 and SHA-512)
	inline sha_word64 ROTR64(sha_word64 b,sha_word64 x){return ((x >> b) | (x << (64 - b)));};
	//32-bit Rotate-left (used in SHA-1)
	inline sha_word32 ROTL32(sha_word32 b,sha_word32 x){return ((x << b) | (x >> (32 - b)));};

	//two logical functions used in SHA-1, SHA-254, SHA-256, SHA-384, and SHA-512
	inline sha_word32 Ch(sha_word32 x,sha_word32 y,sha_word32 z){return ((x & y) ^ ((~x) & z));};
	inline sha_word64 Ch(sha_word64 x,sha_word64 y,sha_word64 z){return ((x & y) ^ ((~x) & z));};
	inline sha_word32 Maj(sha_word32 x,sha_word32 y,sha_word32 z){return ((x & y) ^ (x & z) ^ (y & z));};
	inline sha_word64 Maj(sha_word64 x,sha_word64 y,sha_word64 z){return ((x & y) ^ (x & z) ^ (y & z));};

	//function used in SHA-1
	inline sha_word32 Parity(sha_word32 x,sha_word32 y,sha_word32 z){return (x ^ y ^ z);};

	//four logical functions used in SHA-256
	inline sha_word32 Sigma0_256(sha_word32 x){return (ROTR32(2, x) ^ ROTR32(13, x) ^ ROTR32(22, x));};
	inline sha_word32 Sigma1_256(sha_word32 x){return (ROTR32(6, x) ^ ROTR32(11, x) ^ ROTR32(25, x));};
	inline sha_word32 sigma0_256(sha_word32 x){return (ROTR32(7, x) ^ ROTR32(18, x) ^ SHR(   3 , x));};
	inline sha_word32 sigma1_256(sha_word32 x){return (ROTR32(17,x) ^ ROTR32(19, x) ^ SHR(   10, x));};

	//four of six logical functions used in SHA-384 and SHA-512
	inline sha_word64 Sigma0_512(sha_word64 x){return (ROTR64(28, x) ^ ROTR64(34, x) ^ ROTR64(39, x));};
	inline sha_word64 Sigma1_512(sha_word64 x){return (ROTR64(14, x) ^ ROTR64(18, x) ^ ROTR64(41, x));};
	inline sha_word64 sigma0_512(sha_word64 x){return (ROTR64( 1, x) ^ ROTR64( 8, x) ^ SHR(    7, x));};
	inline sha_word64 sigma1_512(sha_word64 x){return (ROTR64(19, x) ^ ROTR64(61, x) ^ SHR(    6, x));};

	inline void REVERSE32(sha_word32 w, sha_word32 &x){
		w = (w >> 16) | (w << 16);
		x = ((w & 0xff00ff00UL) >> 8) | ((w & 0x00ff00ffUL) << 8);
	}
	#ifdef _VC6
	inline void REVERSE64(sha_word64 w, sha_word64 &x){
		w = (w >> 32) | (w << 32);
		w =   ((w & 0xff00ff00ff00ff00ui64) >> 8) |
			((w & 0x00ff00ff00ff00ffui64) << 8);
			(x) = ((w & 0xffff0000ffff0000ui64) >> 16) |
			((w & 0x0000ffff0000ffffui64) << 16);
	}
	#else
	inline void REVERSE64(sha_word64 w, sha_word64 &x){
		w = (w >> 32) | (w << 32);
		w =   ((w & 0xff00ff00ff00ff00ULL) >> 8) |
			((w & 0x00ff00ff00ff00ffULL) << 8);
			(x) = ((w & 0xffff0000ffff0000ULL) >> 16) |
			((w & 0x0000ffff0000ffffULL) << 16);
	}
	#endif
};
#endif
