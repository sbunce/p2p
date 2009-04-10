#include "tommath.h"
#ifdef BN_MP_LSHD_C

/* shift left a certain amount of digits */
int mp_lshd(mp_int * a, int b)
{
	int x, res;

	/* if its less than zero return */
	if(b <= 0){
		return MP_OKAY;
	}

	/* grow to fit the new digits */
	if(a->alloc < a->used + b){
		if((res = mp_grow(a, a->used + b)) != MP_OKAY){
			return res;
		}
	}

	{
	register mp_digit *top, *bottom;

	/* increment the used by the shift amount then copy upwards */
	a->used += b;

	/* top */
	top = a->dp + a->used - 1;

	/* base */
	bottom = a->dp + a->used - 1 - b;

	/* much like mp_rshd this is implemented using a sliding window
	 * except the window goes the otherway around.  Copying from
	 * the bottom to the top.  see bn_mp_rshd.c for more info.
	 */
	for(x = a->used - 1; x >= b; x--){
		*top-- = *bottom--;
	}

	/* zero the lower digits */
	top = a->dp;
	for(x = 0; x < b; x++){
		*top++ = 0;
	}
	}

	return MP_OKAY;
}
#endif
