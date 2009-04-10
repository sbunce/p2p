#include "tommath.h"
#ifdef BN_MP_SET_C

/* set to a digit */
void mp_set(mp_int * a, mp_digit b)
{
	mp_zero(a);
	a->dp[0] = b & MP_MASK;
	a->used = (a->dp[0] != 0) ? 1 : 0;
}
#endif
