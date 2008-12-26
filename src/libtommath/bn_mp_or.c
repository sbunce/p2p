#include "tommath.h"
#ifdef BN_MP_OR_C

/* OR two ints together */
int mp_or(mp_int * a, mp_int * b, mp_int * c)
{
	int res, ix, px;
	mp_int t, *x;

	if(a->used > b->used){
		if((res = mp_init_copy(&t, a)) != MP_OKAY){
			return res;
		}
		px = b->used;
		x = b;
	}else{
		if((res = mp_init_copy(&t, b)) != MP_OKAY){
			return res;
		}
		px = a->used;
		x = a;
	}

	for(ix = 0; ix < px; ix++){
		t.dp[ix] |= x->dp[ix];
	}
	mp_clamp(&t);
	mp_exch(c, &t);
	mp_clear(&t);
	return MP_OKAY;
}
#endif
