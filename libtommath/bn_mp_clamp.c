#include "tommath.h"
#ifdef BN_MP_CLAMP_C

/* trim unused digits 
 *
 * This is used to ensure that leading zero digits are
 * trimed and the leading "used" digit will be non-zero
 * Typically very fast.  Also fixes the sign if there
 * are no more leading digits
 */
void mp_clamp(mp_int * a)
{
	/* decrease used while the most significant digit is
	 * zero.
	 */
	while(a->used > 0 && a->dp[a->used - 1] == 0){
		--(a->used);
	}

	/* reset the sign flag if used == 0 */
	if(a->used == 0){
		a->sign = MP_ZPOS;
	}
}
#endif
