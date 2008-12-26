#ifndef H_MPINT
#define H_MPINT

#include "tommath.h"

//std
#include <iostream>

class ltmpp_error
{
public:
	ltmpp_error(int c){ type = c; }
	int getType(void){ return type; }
	const char *getTypeStr(){
		switch (type) {
		case MP_MEM: return "Out of memory";
		case MP_VAL: return "Invalid value";
		default    : return "Unknown error";
		}
   }
private:
	int type;
};

class mpint
{
public:
	mpint()
	{
		rstr = NULL;
		if(mp_init(&data) != MP_OKAY){
			throw ltmpp_error(MP_MEM);
			return;
		}
   }

	mpint(const char *str, int radix=10)
	{
		int err;
		rstr = NULL;
		if(mp_init(&data) != MP_OKAY){
			throw ltmpp_error(MP_MEM);
			return;
		}
		if((err = mp_read_radix(&data, str, radix)) != MP_OKAY){
			throw ltmpp_error(err);
		}
	}

	mpint(const unsigned char * bin, int len)
	{
		int err;
		rstr = NULL;
		if(mp_init(&data) != MP_OKAY){
			throw ltmpp_error(MP_MEM);
			return;
		}
		if((err = mp_read_unsigned_bin(&data, bin, len)) != MP_OKAY){
			throw ltmpp_error(err);
		}
	}

	mpint(const mpint & b)
	{
		rstr = NULL;
		if(mp_init(&data) != MP_OKAY){
			throw ltmpp_error(MP_MEM);
			return;
		}
		if(mp_copy((mp_int *)&b.data, &data) != MP_OKAY){
			throw ltmpp_error(MP_MEM); 
		}
	}

	~mpint()
	{
		mp_clear(&data);
		if(rstr != NULL){
			free(rstr);
		}
	}

	//return binary string of number, call to_bin_size for length
   unsigned char * to_bin()
	{
		int n, err;
		if(rstr){
			free(rstr);
			rstr = NULL;
		}
		n = mp_unsigned_bin_size(&data);
		rstr = (char *)calloc(1, n+1);
		if(rstr == NULL){
			throw ltmpp_error(MP_MEM);
			return NULL;
		}
		if((err = mp_to_unsigned_bin(&data, (unsigned char *)rstr)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return (unsigned char *)rstr;
   }

	//length of binary data
   int to_bin_size()
	{
		return mp_unsigned_bin_size(&data);
	}

	//returns the number in the specified radix (2 .. 64)
	char * to_str(int radix)
	{
		int n, err;
		if(rstr){
			free(rstr);
			rstr = NULL;
		}
		if((err = mp_radix_size(&data, radix, &n)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		rstr = (char *)calloc(1, n+1);
		if(rstr == NULL){
			throw ltmpp_error(MP_MEM);
			return NULL;
		}
		if((err = mp_toradix(&data, rstr, radix)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return rstr;
	}

	void operator = (const mpint &b)
	{
		if(mp_copy((mp_int *)&b.data, &data) != MP_OKAY){
			throw ltmpp_error(MP_MEM);
		}
	}

	mpint operator + (const mpint &b)
	{
		int err;
		mpint tmp;
		if((err = mp_add(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
   }

   mpint operator - (const mpint &b)
	{
		int err;
		mpint tmp;
		if((err = mp_sub(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	mpint operator * (const mpint &b)
	{
		int err;
		mpint tmp;
		if((err = mp_mul(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	mpint operator / (const mpint &b){
		int err;
		mpint tmp;
		if((err = mp_div(&data, (mp_int *)&b.data, &tmp.data, NULL)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	mpint operator % (const mpint &b)
	{
		int err;
		mpint tmp;
		if((err = mp_mod(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

   mpint operator ^ (const mpint &b)
	{
		int err;
		mpint tmp;
		if((err = mp_xor(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	mpint operator | (const mpint &b)
	{
		int err;
		mpint tmp;
		if((err = mp_or(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	mpint operator & (const mpint &b)
	{
		int err;
		mpint tmp;
		if((err = mp_and(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

   mpint operator - ()
	{
		int err;
		mpint tmp;
		if((err = mp_neg(&data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	mpint operator << (int n)
	{
		int err;
		mpint tmp;
		if((err = mp_mul_2d(&data, n, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
   }

   mpint operator >> (int n)
	{
		int err;
		mpint tmp;
		if((err = mp_div_2d(&data, n, &tmp.data, NULL)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	mpint &operator += (const mpint &b)
	{
		int err;
		if((err = mp_add(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator -= (const mpint &b)
	{
		int err;
		if((err = mp_sub(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

   mpint &operator *= (const mpint &b)
	{
		int err;
		if((err = mp_mul(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator /= (const mpint &b)
	{
		int err;
		if((err = mp_div(&data, (mp_int *)&b.data, &data, NULL)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator %= (const mpint &b)
	{
		int err;
		if((err = mp_mod(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator ^= (const mpint &b)
	{
		int err;
		if((err = mp_xor(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator &= (const mpint &b)
	{
		int err;
		if((err = mp_and(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator |= (const mpint &b)
	{
		int err;
		if((err = mp_or(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator <<= (int n)
	{
		int err;
		if((err = mp_mul_2d(&data, n, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator >>= (int n)
	{
		int err;
		if((err = mp_div_2d(&data, n, &data, NULL)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

   mpint &operator ++ ()
	{
		int err;
		if((err = mp_add_d(&data, 1, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator -- ()
	{
		int err;
		if((err = mp_sub_d(&data, 1, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator ++ (int)
	{
		int err;
		if((err = mp_add_d(&data, 1, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator -- (int)
	{
		int err;
		if((err = mp_sub_d(&data, 1, &data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return *this;
	}

	mpint &operator += (const mp_digit b)
	{
		int err;
		mp_digit q;
		if(b > MP_MASK){
			q = MP_MASK - (b & MP_MASK) + 1;
			if((err = mp_sub_d(&data, q, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}else{
			if((err = mp_add_d(&data, b, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}
		return *this;
   }

	mpint &operator -= (const mp_digit b)
	{
		int err;
		mp_digit q;
		if(b > MP_MASK){
			q = MP_MASK - (b & MP_MASK) + 1;
			if((err = mp_add_d(&data, q, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}else{
			if((err = mp_sub_d(&data, b, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
      }
      return *this;
   }

	mpint &operator *= (const mp_digit b)
	{
		int err;
		mp_digit q;
		if(b > MP_MASK){
			q = MP_MASK - (b & MP_MASK) + 1;
			if((err = mp_mul_d(&data, q, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
			if((err = mp_neg(&data, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}else{
			if((err = mp_mul_d(&data, b, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}
		return *this;
	}

	mpint &operator /= (const mp_digit b)
	{
		int err;
		mp_digit q;
		if(b > MP_MASK){
			q = MP_MASK - (b & MP_MASK) + 1;
			if((err = mp_div_d(&data, q, &data, NULL)) != MP_OKAY){
				throw ltmpp_error(err);
			}
			if((err = mp_neg(&data, &data)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}else{
			if((err = mp_div_d(&data, b, &data, NULL)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}
		return *this;
	}

	mpint &operator %= (const mp_digit b)
	{
		int err;
		mp_digit q, r;
		if(b > MP_MASK) {
			q = MP_MASK - (b & MP_MASK) + 1;
			if((err = mp_div_d(&data, q, NULL, &r)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}else{
			if((err = mp_div_d(&data, b, NULL, &r)) != MP_OKAY){
				throw ltmpp_error(err);
			}
		}
		mp_set(&data, r);
		return *this;
	}

   int operator == (const mpint &b){ return mp_cmp(&data, (mp_int *)&b.data) == MP_EQ ? 1 : 0; }
   int operator != (const mpint &b){ return mp_cmp(&data, (mp_int *)&b.data) == MP_EQ ? 0 : 1; }
   int operator > (const mpint &b){ return mp_cmp(&data, (mp_int *)&b.data) == MP_GT ? 1 : 0; }
   int operator < (const mpint &b){ return mp_cmp(&data, (mp_int *)&b.data) == MP_LT ? 1 : 0; }
   int operator >= (const mpint &b){ return mp_cmp(&data, (mp_int *)&b.data) == MP_LT ? 0 : 1; }
   int operator <= (const mpint &b){ return mp_cmp(&data, (mp_int *)&b.data) == MP_GT ? 0 : 1; }

	int operator == (const mp_digit b)
	{
		mp_digit q;
		int r;
		if(b > MP_MASK){
			if(data.sign != MP_NEG){
				return 0;
			}
			q = MP_MASK - (b & MP_MASK) + 1;
			data.sign = MP_ZPOS;
			r = mp_cmp_d(&data, q) == MP_EQ ? 1 : 0;
			data.sign = MP_NEG;
			return r;
		}else{
			return mp_cmp_d(&data, b) == MP_EQ ? 1 : 0;
		}
	}

	int operator != (const mp_digit b)
	{
		mp_digit q;
		int r;
		if(b > MP_MASK){
			if(data.sign != MP_NEG){
				return 1;
			}
			q = MP_MASK - (b & MP_MASK) + 1;
			data.sign = MP_ZPOS;
			r = mp_cmp_d(&data, q) == MP_EQ ? 0 : 1;
			data.sign = MP_NEG;
			return r;
		}else{
			return mp_cmp_d(&data, b) == MP_EQ ? 0 : 1;
		}
	}

	int operator > (const mp_digit b)
	{
		mp_digit q;
		int r;
		if(b > MP_MASK){
			if(data.sign != MP_NEG){
				return 1;
			}
			q = MP_MASK - (b & MP_MASK) + 1;
			data.sign = MP_ZPOS;
			r = mp_cmp_d(&data, q) == MP_LT ? 1 : 0;
			data.sign = MP_NEG;
			return r;
		}else{
			return mp_cmp_d(&data, b) == MP_GT ? 1 : 0;
		}
	}

	int operator >= (const mp_digit b)
	{
		mp_digit q;
		int r;
		if(b > MP_MASK){
			if(data.sign != MP_NEG){
				return 1;
			}
			q = MP_MASK - (b & MP_MASK) + 1;
			data.sign = MP_ZPOS;
			r = mp_cmp_d(&data, q) == MP_GT ? 0 : 1;
			data.sign = MP_NEG;
			return r;
		}else{
			return mp_cmp_d(&data, b) == MP_LT ? 0 : 1;
		}
	}

	int operator < (const mp_digit b)
	{
		mp_digit q;
		int r;
		if(b > MP_MASK){
			if (data.sign != MP_NEG){
				return 0;
			}
			q = MP_MASK - (b & MP_MASK) + 1;
			data.sign = MP_ZPOS;
			r = mp_cmp_d(&data, q) == MP_LT ? 0 : 1;
			data.sign = MP_NEG;
			return r;
		}else{
			return mp_cmp_d(&data, b) == MP_GT ? 0 : 1;
		}
	}

	int operator <= (const mp_digit b)
	{
		mp_digit q;
		int r;
		if(b > MP_MASK){
			if (data.sign != MP_NEG){
				return 0;
			}
			q = MP_MASK - (b & MP_MASK) + 1;
			data.sign = MP_ZPOS;
			r = mp_cmp_d(&data, q) == MP_GT ? 1 : 0;
			data.sign = MP_NEG;
			return r;
		}else{
			return mp_cmp_d(&data, b) == MP_LT ? 1 : 0;
		}
	}

	friend std::ostream & operator << (std::ostream & lval, mpint rval)
	{
		return lval << rval.to_str(10);
	}

	//greatest common denominator
   mpint gcd(const mpint & b)
	{
		int err;
		mpint tmp;
		if((err = mp_gcd(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	//least common multiple
	mpint lcm(const mpint & b)
	{
		int err;
		mpint tmp;
		if((err = mp_lcm(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	//invers modulo
	mpint invmod(const mpint & b)
	{
		int err;
		mpint tmp;
		if((err = mp_invmod(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	int is_prime()
	{
		int err, n;
		if((err = mp_prime_is_prime(&data, 10, &n)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return n;
	}

	//(this number)**y mod(p)
	mpint exptmod(const mpint & y, const mpint & p)
	{
		int err;
		mpint tmp;
		if((err = mp_exptmod(&data, (mp_int *)&y.data, (mp_int *)&p.data, &tmp.data)) != MP_OKAY){
			throw ltmpp_error(err);
		}
		return tmp;
	}

	/*
	Returns a NON-CONST reference to the C mp_int structure that is being wrapped.
	This can be used for the libtommath functions which aren't wrapped. Be careful
	or you'll poke your eye out.
	*/
	mp_int & c_struct()
	{
		return data;
	}

private:
	mp_int data;
	char * rstr;
};
#endif
