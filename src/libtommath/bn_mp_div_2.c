#include "tommath.h"
#ifdef BN_MP_DIV_2_C

/* b = a/2 */
int mp_div_2(mp_int * a, mp_int * b)
{
	int x, res, oldused;

	/* copy */
	if(b->alloc < a->used){
		if((res = mp_grow(b, a->used)) != MP_OKAY){
			return res;
		}
	}

	oldused = b->used;
	b->used = a->used;
	{
	register mp_digit r, rr, *tmpa, *tmpb;

	/* source alias */
	tmpa = a->dp + b->used - 1;

	/* dest alias */
	tmpb = b->dp + b->used - 1;

	/* carry */
	r = 0;
	for(x = b->used - 1; x >= 0; x--){
		/* get the carry for the next iteration */
		rr = *tmpa & 1;

		/* shift the current digit, add in carry and store */
		*tmpb-- = (*tmpa-- >> 1) | (r << (DIGIT_BIT - 1));

		/* forward carry to next iteration */
		r = rr;
	}

	/* zero excess digits */
	tmpb = b->dp + b->used;
	for(x = b->used; x < oldused; x++){
		*tmpb++ = 0;
	}
	}
	b->sign = a->sign;
	mp_clamp(b);
	return MP_OKAY;
}
#endif
