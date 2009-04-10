#include "tommath.h"
#ifdef BN_MP_SET_INT_C

/* set a 32-bit const */
int mp_set_int(mp_int * a, unsigned long b)
{
	int x, res;

	mp_zero(a);
  
	/* set four bits at a time */
	for(x = 0; x < 8; x++){
		/* shift the number up four bits */
		if((res = mp_mul_2d(a, 4, a)) != MP_OKAY){
			return res;
		}

		/* OR in the top four bits of the source */
		a->dp[0] |= (b >> 28) & 15;

		/* shift the source up to the next four bits */
		b <<= 4;

		/* ensure that digits are not clamped off */
		a->used += 1;
	}

	mp_clamp(a);
	return MP_OKAY;
}
#endif
