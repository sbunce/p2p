#include "tommath.h"
#ifdef BN_MP_INIT_C

/* init a new mp_int */
int mp_init(mp_int * a)
{
	int i;

	/* allocate memory required and clear it */
	a->dp = OPT_CAST(mp_digit) XMALLOC(sizeof(mp_digit) * MP_PREC);
	if(a->dp == NULL){
		return MP_MEM;
	}

	/* set the digits to zero */
	for(i = 0; i < MP_PREC; i++){
		a->dp[i] = 0;
	}

	/* set the used to zero, allocated digits to the default precision
	 * and sign to positive
	 */
	a->used  = 0;
	a->alloc = MP_PREC;
	a->sign  = MP_ZPOS;

	return MP_OKAY;
}
#endif
