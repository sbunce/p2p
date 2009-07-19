#include <tommath/mpint.hpp>

mpint::mpint()
{
	rstr = NULL;
	if(mp_init(&data) != MP_OKAY){
		throw ltmpp_error(MP_MEM);
		return;
	}
}

mpint::mpint(const char * str, int radix)
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

mpint::mpint(const unsigned char * bin, int len)
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

mpint::mpint(const mpint & b)
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

mpint::~mpint()
{
	mp_clear(&data);
	if(rstr != NULL){
		free(rstr);
	}
}

unsigned char * mpint::to_bin()
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

int mpint::to_bin_size()
{
	return mp_unsigned_bin_size(&data);
}

char * mpint::to_str(int radix)
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

void mpint::operator = (const mpint &b)
{
	if(mp_copy((mp_int *)&b.data, &data) != MP_OKAY){
		throw ltmpp_error(MP_MEM);
	}
}

mpint mpint::operator + (const mpint &b)
{
	int err;
	mpint tmp;
	if((err = mp_add(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator - (const mpint &b)
{
	int err;
	mpint tmp;
	if((err = mp_sub(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator * (const mpint &b)
{
	int err;
	mpint tmp;
	if((err = mp_mul(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator / (const mpint &b){
	int err;
	mpint tmp;
	if((err = mp_div(&data, (mp_int *)&b.data, &tmp.data, NULL)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator % (const mpint &b)
{
	int err;
	mpint tmp;
	if((err = mp_mod(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator ^ (const mpint &b)
{
	int err;
	mpint tmp;
	if((err = mp_xor(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator | (const mpint &b)
{
	int err;
	mpint tmp;
	if((err = mp_or(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator & (const mpint &b)
{
	int err;
	mpint tmp;
	if((err = mp_and(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator - ()
{
	int err;
	mpint tmp;
	if((err = mp_neg(&data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator << (int n)
{
	int err;
	mpint tmp;
	if((err = mp_mul_2d(&data, n, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::operator >> (int n)
{
	int err;
	mpint tmp;
	if((err = mp_div_2d(&data, n, &tmp.data, NULL)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint & mpint::operator += (const mpint &b)
{
	int err;
	if((err = mp_add(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator -= (const mpint & b)
{
	int err;
	if((err = mp_sub(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator *= (const mpint &b)
{
	int err;
	if((err = mp_mul(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator /= (const mpint &b)
{
	int err;
	if((err = mp_div(&data, (mp_int *)&b.data, &data, NULL)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator %= (const mpint &b)
{
	int err;
	if((err = mp_mod(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator ^= (const mpint &b)
{
	int err;
	if((err = mp_xor(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator &= (const mpint &b)
{
	int err;
	if((err = mp_and(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator |= (const mpint &b)
{
	int err;
	if((err = mp_or(&data, (mp_int *)&b.data, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator <<= (int n)
{
	int err;
	if((err = mp_mul_2d(&data, n, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator >>= (int n)
{
	int err;
	if((err = mp_div_2d(&data, n, &data, NULL)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator ++ ()
{
	int err;
	if((err = mp_add_d(&data, 1, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator -- ()
{
	int err;
	if((err = mp_sub_d(&data, 1, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator ++ (int)
{
	int err;
	if((err = mp_add_d(&data, 1, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator -- (int)
{
	int err;
	if((err = mp_sub_d(&data, 1, &data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return *this;
}

mpint & mpint::operator += (const mp_digit b)
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

mpint & mpint::operator -= (const mp_digit b)
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

mpint & mpint::operator *= (const mp_digit b)
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

mpint & mpint::operator /= (const mp_digit b)
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

mpint & mpint::operator %= (const mp_digit b)
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

bool mpint::operator == (const mpint &b)
{
	return mp_cmp(&data, (mp_int *)&b.data) == MP_EQ ? true : false;
}

bool mpint::operator != (const mpint &b)
{
	return mp_cmp(&data, (mp_int *)&b.data) == MP_EQ ? false : true;
}

bool mpint::operator > (const mpint &b)
{
	return mp_cmp(&data, (mp_int *)&b.data) == MP_GT ? true : false;
}

bool mpint::operator < (const mpint &b)
{
	return mp_cmp(&data, (mp_int *)&b.data) == MP_LT ? true : false;
}

bool mpint::operator >= (const mpint &b)
{
	return mp_cmp(&data, (mp_int *)&b.data) == MP_LT ? false : true;
}

bool mpint::operator <= (const mpint &b)
{
	return mp_cmp(&data, (mp_int *)&b.data) == MP_GT ? false : true;
}

bool mpint::operator == (const mp_digit b)
{
	mp_digit q;
	int r;
	if(b > MP_MASK){
		if(data.sign != MP_NEG){
			return false;
		}
		q = MP_MASK - (b & MP_MASK) + 1;
		data.sign = MP_ZPOS;
		r = mp_cmp_d(&data, q) == MP_EQ ? true : false;
		data.sign = MP_NEG;
		return r;
	}else{
		return mp_cmp_d(&data, b) == MP_EQ ? true : false;
	}
}

bool mpint::operator != (const mp_digit b)
{
	mp_digit q;
	int r;
	if(b > MP_MASK){
		if(data.sign != MP_NEG){
			return true;
		}
		q = MP_MASK - (b & MP_MASK) + 1;
		data.sign = MP_ZPOS;
		r = mp_cmp_d(&data, q) == MP_EQ ? false : true;
		data.sign = MP_NEG;
		return r;
	}else{
		return mp_cmp_d(&data, b) == MP_EQ ? false : true;
	}
}

bool mpint::operator > (const mp_digit b)
{
	mp_digit q;
	int r;
	if(b > MP_MASK){
		if(data.sign != MP_NEG){
			return true;
		}
		q = MP_MASK - (b & MP_MASK) + 1;
		data.sign = MP_ZPOS;
		r = mp_cmp_d(&data, q) == MP_LT ? true : false;
		data.sign = MP_NEG;
		return r;
	}else{
		return mp_cmp_d(&data, b) == MP_GT ? true : false;
	}
}

bool mpint::operator >= (const mp_digit b)
{
	mp_digit q;
	int r;
	if(b > MP_MASK){
		if(data.sign != MP_NEG){
			return true;
		}
		q = MP_MASK - (b & MP_MASK) + 1;
		data.sign = MP_ZPOS;
		r = mp_cmp_d(&data, q) == MP_GT ? false : true;
		data.sign = MP_NEG;
		return r;
	}else{
		return mp_cmp_d(&data, b) == MP_LT ? false : true;
	}
}

bool mpint::operator < (const mp_digit b)
{
	mp_digit q;
	int r;
	if(b > MP_MASK){
		if (data.sign != MP_NEG){
			return false;
		}
		q = MP_MASK - (b & MP_MASK) + 1;
		data.sign = MP_ZPOS;
		r = mp_cmp_d(&data, q) == MP_LT ? false : true;
		data.sign = MP_NEG;
		return r;
	}else{
		return mp_cmp_d(&data, b) == MP_GT ? false : true;
	}
}

bool mpint::operator <= (const mp_digit b)
{
	mp_digit q;
	int r;
	if(b > MP_MASK){
		if (data.sign != MP_NEG){
			return false;
		}
		q = MP_MASK - (b & MP_MASK) + 1;
		data.sign = MP_ZPOS;
		r = mp_cmp_d(&data, q) == MP_GT ? true : false;
		data.sign = MP_NEG;
		return r;
	}else{
		return mp_cmp_d(&data, b) == MP_LT ? true : false;
	}
}

mpint mpint::gcd(const mpint & b)
{
	int err;
	mpint tmp;
	if((err = mp_gcd(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::lcm(const mpint & b)
{
	int err;
	mpint tmp;
	if((err = mp_lcm(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

mpint mpint::invmod(const mpint & b)
{
	int err;
	mpint tmp;
	if((err = mp_invmod(&data, (mp_int *)&b.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}

bool mpint::is_prime()
{
	int err, n;
	if((err = mp_prime_is_prime(&data, 10, &n)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return n;
}

mpint mpint::random(const int bytes)
{
	unsigned char * buff = static_cast<unsigned char *>(std::malloc(bytes));
	portable_urandom(buff, bytes, NULL);
	mpint temp(buff, bytes);
	std::free(buff);
	return temp;
}

mpint mpint::random_prime(const int bytes)
{
	mpint temp;
	mp_prime_random_ex(
		&temp.data,
		mp_prime_rabin_miller_trials(bytes * 8),
		bytes * 8,         //size (bits) of prime to generate
		0,                 //optional flags
		&portable_urandom, //random byte source
		NULL               //optional void * passed to PRNG
	);
	return temp;
}

mpint mpint::exptmod(const mpint & y, const mpint & p)
{
	int err;
	mpint tmp;
	if((err = mp_exptmod(&data, (mp_int *)&y.data, (mp_int *)&p.data, &tmp.data)) != MP_OKAY){
		throw ltmpp_error(err);
	}
	return tmp;
}
