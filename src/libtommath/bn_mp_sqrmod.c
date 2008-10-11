#include "tommath.h"
#ifdef BN_MP_SQRMOD_C

/* c = a * a (mod b) */
int mp_sqrmod(mp_int * a, mp_int * b, mp_int * c)
{
	int res;
	mp_int t;

	if((res = mp_init(&t)) != MP_OKAY){
		return res;
	}

	if((res = mp_sqr (a, &t)) != MP_OKAY){
		mp_clear(&t);
		return res;
	}

	res = mp_mod(&t, b, c);
	mp_clear(&t);
	return res;
}
#endif
