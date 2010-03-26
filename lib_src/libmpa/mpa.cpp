#include "tommath.hpp"
#include <mpa.hpp>
#include <random.hpp>

#ifdef max
	#undef max
#endif

std::exception exception(const int code)
{
	switch(code){
		case MP_MEM:
			return mpa::out_of_memory();
		case MP_VAL:
			return mpa::invalid_value();
		default:
			std::cout << "boost::mpa unhandled error";
			exit(1);
	}
}

mpa::mpint::mpint():
	data(new mp_int())
{
	if(::mp_init(data.get()) != MP_OKAY){
		throw ::exception(MP_MEM);
	}
}

mpa::mpint::mpint(const mpint & M):
	data(new mp_int())
{
	if(::mp_init(data.get()) != MP_OKAY){
		throw ::exception(MP_MEM);
	}
	if(::mp_copy(M.data.get(), data.get()) != MP_OKAY){
		throw ::exception(MP_MEM);
	}
}

mpa::mpint::mpint(const char * str, const int radix):
	data(new mp_int())
{
	int err;
	if(::mp_init(data.get()) != MP_OKAY){
		throw ::exception(MP_MEM);
	}
	if((err = ::mp_read_radix(data.get(), str, radix)) != MP_OKAY){
		throw ::exception(err);
	}
}

mpa::mpint::mpint(const unsigned char * bin, const int len):
	data(new mp_int())
{
	int err;
	if(::mp_init(data.get()) != MP_OKAY){
		throw ::exception(MP_MEM);
	}
	if((err = ::mp_read_unsigned_bin(data.get(), bin, len)) != MP_OKAY){
		throw ::exception(err);
	}
}

mpa::mpint::~mpint()
{
	::mp_clear(data.get());
}

std::string mpa::mpint::bin(std::size_t bytes) const
{
	assert(bytes <= std::numeric_limits<int>::max());
	int n = ::mp_unsigned_bin_size(data.get());
	if(bytes != 0){
		assert(n <= bytes);
	}
	unsigned char * tmp;
	std::size_t offset;
	if(bytes == 0){
		//no padding
		tmp = new unsigned char[n];
		offset = 0;
	}else{
		//pad
		tmp = new unsigned char[bytes];
		offset = bytes - n;
		std::memset(tmp, '\0', offset);
	}
	if(tmp == NULL){
		throw ::exception(MP_MEM);
	}
	int err;
	if((err = ::mp_to_unsigned_bin(data.get(), tmp + offset)) != MP_OKAY){
		throw ::exception(err);
	}
	std::string result(reinterpret_cast<char *>(tmp), bytes == 0 ? n : bytes);
	delete[] tmp;
	return result;
}

std::string mpa::mpint::str(const int radix) const
{
	int n, err;
	if((err = ::mp_radix_size(data.get(), radix, &n)) != MP_OKAY){
		throw ::exception(err);
	}
	char * tmp = new char[n+1];
	if(tmp == NULL){
		throw ::exception(MP_MEM);
	}
	if((err = ::mp_toradix(data.get(), tmp, radix)) != MP_OKAY){
		throw ::exception(err);
	}
	std::string result(tmp, n);
	delete[] tmp;
	return result;
}

mpa::mpint & mpa::mpint::operator = (const mpint & b)
{
	if(::mp_copy(b.data.get(), data.get()) != MP_OKAY){
		throw ::exception(MP_MEM);
	}
	return *this;
}

mpa::mpint mpa::mpint::operator + (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_add(data.get(), b.data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator - (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_sub(data.get(), b.data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator * (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_mul(data.get(), b.data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator / (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_div(data.get(), b.data.get(), tmp.data.get(), NULL)) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator & (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_and(data.get(), b.data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator | (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_or(data.get(), b.data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator ^ (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_xor(data.get(), b.data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator % (const mpint & b) const
{
	int err;
	mpint tmp;
	if((err = ::mp_mod(data.get(), b.data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator - () const
{
	int err;
	mpint tmp;
	if((err = ::mp_neg(data.get(), tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint & mpa::mpint::operator += (const mpint & b)
{
	int err;
	if((err = ::mp_add(data.get(), b.data.get(), data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator -= (const mpint & b)
{
	int err;
	if((err = ::mp_sub(data.get(), b.data.get(), data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator *= (const mpint & b)
{
	int err;
	if((err = ::mp_mul(data.get(), b.data.get(), data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator /= (const mpint & b)
{
	int err;
	if((err = ::mp_div(data.get(), b.data.get(), data.get(), NULL)) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator &= (const mpint & b)
{
	int err;
	if((err = ::mp_and(data.get(), b.data.get(), data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator |= (const mpint & b)
{
	int err;
	if((err = ::mp_or(data.get(), b.data.get(), data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator ^= (const mpint & b)
{
	int err;
	if((err = ::mp_xor(data.get(), b.data.get(), data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator %= (const mpint & b)
{
	int err;
	if((err = ::mp_mod(data.get(), b.data.get(), data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator <<= (const int n)
{
	int err;
	if((err = ::mp_mul_2d(data.get(), n, data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator >>= (const int n)
{
	int err;
	if((err = ::mp_div_2d(data.get(), n, data.get(), NULL)) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator ++ ()
{
	int err;
	if((err = ::mp_add_d(data.get(), 1, data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint & mpa::mpint::operator -- ()
{
	int err;
	if((err = ::mp_sub_d(data.get(), 1, data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return *this;
}

mpa::mpint mpa::mpint::operator ++ (int)
{
	int err;
	mpint tmp = *this;
	if((err = ::mp_add_d(data.get(), 1, data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator -- (int)
{
	int err;
	mpint tmp = *this;
	if((err = ::mp_sub_d(data.get(), 1, data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

bool mpa::mpint::operator == (const mpint & b) const
{
	return ::mp_cmp(data.get(), b.data.get()) == MP_EQ;
}

bool mpa::mpint::operator != (const mpint & b) const
{
	return ::mp_cmp(data.get(), b.data.get()) != MP_EQ;
}

bool mpa::mpint::operator > (const mpint & b) const
{
	return ::mp_cmp(data.get(), b.data.get()) == MP_GT;
}

bool mpa::mpint::operator < (const mpint & b) const
{
	return ::mp_cmp(data.get(), b.data.get()) == MP_LT;
}

bool mpa::mpint::operator >= (const mpint & b) const
{
	int ret = ::mp_cmp(data.get(), b.data.get());
	return ret == MP_GT || ret == MP_EQ;
}

bool mpa::mpint::operator <= (const mpint & b) const
{
	int ret = ::mp_cmp(data.get(), b.data.get());
	return ret == MP_LT || ret == MP_EQ;
}

mpa::mpint mpa::mpint::operator << (const int n) const
{
	int err;
	mpint tmp;
	if((err = ::mp_mul_2d(data.get(), n, tmp.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::mpint::operator >> (const int n) const
{
	int err;
	mpint tmp;
	if((err = ::mp_div_2d(data.get(), n, tmp.data.get(), NULL)) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::sqr(const mpint & a)
{
	int err;
	mpint b;
	if((err = ::mp_sqr(a.data.get(), b.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return b;
}

mpa::mpint mpa::sqrt(const mpint & a)
{
	int err;
	mpint b;
	if((err = ::mp_sqrt(a.data.get(), b.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return b;
}

mpa::mpint mpa::n_root(const mpint & a, const boost::uint32_t & b)
{
	int err;
	mpint c;
	if((err = ::mp_n_root(a.data.get(), b, c.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return c;
}

mpa::mpint mpa::mul_2(const mpint & a)
{
	int err;
	mpint b;
	if((err = ::mp_mul_2(a.data.get(), b.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return b;
}

mpa::mpint mpa::div_2(const mpint & a)
{
	int err;
	mpint b;
	if((err = ::mp_div_2(a.data.get(), b.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return b;
}

mpa::mpint mpa::mul_2d(const mpint & a, const boost::uint32_t b)
{
	int err;
	mpint c;
	if((err = ::mp_mul_2d(a.data.get(), b, c.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return c;
}

void mpa::div_2d(const mpint & a, const boost::uint32_t b, mpint & c, mpint & d)
{
	int err;
	if((err = ::mp_div_2d(a.data.get(), b, c.data.get(), d.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
}

mpa::mpint mpa::mod_2d(const mpint & a, const boost::uint32_t b)
{
	int err;
	mpint c;
	if((err = ::mp_mod_2d(a.data.get(), b, c.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return c;
}

mpa::mpint mpa::addmod(const mpint & a, const mpint & b, const mpint & c)
{
	int err;
	mpint d;
	if((err = ::mp_addmod(a.data.get(), b.data.get(), c.data.get(), d.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return d;
}

mpa::mpint mpa::submod(const mpint & a, const mpint & b, const mpint & c)
{
	int err;
	mpint d;
	if((err = ::mp_submod(a.data.get(), b.data.get(), c.data.get(), d.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return d;
}

mpa::mpint mpa::mulmod(const mpint & a, const mpint & b, const mpint & c)
{
	int err;
	mpint d;
	if((err = ::mp_mulmod(a.data.get(), b.data.get(), c.data.get(), d.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return d;
}

mpa::mpint mpa::sqrmod(const mpint & a, const mpint & b)
{
	int err;
	mpint c;
	if((err = ::mp_sqrmod(a.data.get(), b.data.get(), c.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return c;
}

mpa::mpint mpa::invmod(const mpint & a, const mpint & b)
{
	int err;
	mpint c;
	if((err = ::mp_invmod(a.data.get(), b.data.get(), c.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return c;
}

mpa::mpint mpa::exptmod(const mpint & a, const mpint & b, const mpint & c)
{
	int err;
	mpint d;
	if((err = ::mp_exptmod(a.data.get(), b.data.get(), c.data.get(), d.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return d;
}

bool mpa::is_prime(const mpint & a, const int t)
{
	assert(t > 0);
	int err, n;
	if((err = ::mp_prime_is_prime(a.data.get(), t, &n)) != MP_OKAY){
		throw ::exception(err);
	}
	return n;
}

mpa::mpint mpa::next_prime(const mpint & a, const int t, const bool bbs_style)
{
	assert(t > 0);
	int err, bbs_style_int = bbs_style ? 1 : 0;
	mpint tmp = a;
	if((err = ::mp_prime_next_prime(tmp.data.get(), t, bbs_style_int)) != MP_OKAY){
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::gcd(const mpint & a, const mpint & b)
{
	int err;
	mpint c;
	if((err = ::mp_gcd(a.data.get(), b.data.get(), c.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return c;
}

mpa::mpint mpa::lcm(const mpint & a, const mpint & b)
{
	int err;
	mpint c;
	if((err = ::mp_lcm(a.data.get(), b.data.get(), c.data.get())) != MP_OKAY){
		throw ::exception(err);
	}
	return c;
}

mpa::mpint mpa::random_prime(const std::size_t bytes)
{
	assert(bytes * 8 < std::numeric_limits<int>::max());
	int err;
	mpint tmp;
	if((err = mp_prime_random_ex(
		tmp.data.get(),
		mp_prime_rabin_miller_trials(bytes * 8),
		bytes * 8,         //size (bits) of prime to generate
		0,                 //optional flags
		&portable_urandom, //random byte source
		NULL               //optional void * passed to PRNG
	)) != MP_OKAY)
	{
		throw ::exception(err);
	}
	return tmp;
}

mpa::mpint mpa::random(const int bytes)
{
	unsigned char * buf = new unsigned char[bytes];
	if(buf == NULL){
		throw ::exception(MP_MEM);
	}
	portable_urandom(buf, bytes);
	mpint tmp(buf, bytes);
	delete[] buf;
	return tmp;
}
