#include "tommath.h"
#ifdef BN_MP_COPY_C

/* copy, b = a */
int mp_copy(mp_int * a, mp_int * b)
{
	int res, n;

	/* if dst == src do nothing */
	if(a == b){
		return MP_OKAY;
	}

	/* grow dest */
	if(b->alloc < a->used){
		if((res = mp_grow(b, a->used)) != MP_OKAY){
			return res;
		}
	}

	/* zero b and copy the parameters over */
	{
	register mp_digit *tmpa, *tmpb;

	/* pointer aliases */

	/* source */
	tmpa = a->dp;

	/* destination */
	tmpb = b->dp;

	/* copy all the digits */
	for(n = 0; n < a->used; n++){
		*tmpb++ = *tmpa++;
	}

	/* clear high digits */
	for(; n < b->used; n++){
		*tmpb++ = 0;
	}
	}

	/* copy used count and sign */
	b->used = a->used;
	b->sign = a->sign;
	return MP_OKAY;
}
#endif
