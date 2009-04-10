#include "tommath.h"
#ifdef BN_MP_AND_C

/* AND two ints together */
int mp_and(mp_int * a, mp_int * b, mp_int * c)
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
		t.dp[ix] &= x->dp[ix];
	}

	/* zero digits above the last from the smallest mp_int */
	for(; ix < t.used; ix++){
		t.dp[ix] = 0;
	}

	mp_clamp(&t);
	mp_exch(c, &t);
	mp_clear(&t);
	return MP_OKAY;
}
#endif
