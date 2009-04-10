#include "tommath.h"
#ifdef BN_MP_SQR_C

/* computes b = a*a */
int mp_sqr(mp_int * a, mp_int * b)
{
	int res;

#ifdef BN_MP_TOOM_SQR_C
	/* use Toom-Cook? */
	if(a->used >= TOOM_SQR_CUTOFF){
		res = mp_toom_sqr(a, b);
		/* Karatsuba? */
	}else 
#endif
#ifdef BN_MP_KARATSUBA_SQR_C
	if(a->used >= KARATSUBA_SQR_CUTOFF){
		res = mp_karatsuba_sqr (a, b);
	}else 
#endif
	{
#ifdef BN_FAST_S_MP_SQR_C
	/* can we use the fast comba multiplier? */
	if((a->used * 2 + 1) < MP_WARRAY && a->used <
		(1 << (sizeof(mp_word) * CHAR_BIT - 2*DIGIT_BIT - 1))){
		res = fast_s_mp_sqr (a, b);
	}else
#endif
#ifdef BN_S_MP_SQR_C
		res = s_mp_sqr(a, b);
#else
		res = MP_VAL;
#endif
	}

	b->sign = MP_ZPOS;
	return res;
}
#endif
