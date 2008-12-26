#include "tommath.h"
#ifdef BN_MP_INIT_SIZE_C

/* init an mp_init for a given size */
int mp_init_size(mp_int * a, int size)
{
	int x;

	/* pad size so there are always extra digits */
	size += (MP_PREC * 2) - (size % MP_PREC);	
  
	/* alloc mem */
	a->dp = OPT_CAST(mp_digit) XMALLOC(sizeof(mp_digit) * size);
	if(a->dp == NULL){
		return MP_MEM;
	}

	/* set the members */
	a->used  = 0;
	a->alloc = size;
	a->sign  = MP_ZPOS;

	/* zero the digits */
	for(x = 0; x < size; x++){
		a->dp[x] = 0;
	}

	return MP_OKAY;
}
#endif
