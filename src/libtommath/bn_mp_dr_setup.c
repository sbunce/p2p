#include "tommath.h"
#ifdef BN_MP_DR_SETUP_C

/* determines the setup value */
void mp_dr_setup(mp_int *a, mp_digit *d)
{
	/* the casts are required if DIGIT_BIT is one less than
	 * the number of bits in a mp_digit [e.g. DIGIT_BIT==31]
	 */
	*d = (mp_digit)((((mp_word)1) << ((mp_word)DIGIT_BIT)) - ((mp_word)a->dp[0]));
}
#endif
