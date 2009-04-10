#include "tommath.h"
#ifdef BN_MP_COUNT_BITS_C

/* returns the number of bits in an int */
int mp_count_bits(mp_int * a)
{
	int r;
	mp_digit q;

	/* shortcut */
	if(a->used == 0){
		return 0;
	}

	/* get number of digits and add that */
	r = (a->used - 1) * DIGIT_BIT;
  
	/* take the last digit and count the bits in it */
	q = a->dp[a->used - 1];
	while(q > ((mp_digit)0)){
		++r;
		q >>= ((mp_digit)1);
	}
	return r;
}
#endif
