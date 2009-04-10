#include "tommath.h"
#ifdef BN_MP_CMP_MAG_C

/* compare maginitude of two ints (unsigned) */
int mp_cmp_mag(mp_int * a, mp_int * b)
{
	int n;
	mp_digit *tmpa, *tmpb;

	/* compare based on # of non-zero digits */
	if(a->used > b->used){
		return MP_GT;
	}
  
	if(a->used < b->used){
		return MP_LT;
	}

	/* alias for a */
	tmpa = a->dp + (a->used - 1);

	/* alias for b */
	tmpb = b->dp + (a->used - 1);

	/* compare based on digits  */
	for(n = 0; n < a->used; ++n, --tmpa, --tmpb){
		if(*tmpa > *tmpb){
			return MP_GT;
		}

		if(*tmpa < *tmpb){
			return MP_LT;
		}
	}
	return MP_EQ;
}
#endif
