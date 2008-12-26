#include "tommath.h"
#ifdef BN_MP_MOD_C

/* c = a mod b, 0 <= c < b */
int mp_mod(mp_int * a, mp_int * b, mp_int * c)
{
	mp_int t;
	int res;

	if((res = mp_init(&t)) != MP_OKAY){
		return res;
	}

	if((res = mp_div(a, b, NULL, &t)) != MP_OKAY){
		mp_clear(&t);
		return res;
	}

	if(t.sign != b->sign){
		res = mp_add(b, &t, c);
	}else{
		res = MP_OKAY;
		mp_exch(&t, c);
	}

	mp_clear(&t);
	return res;
}
#endif
