#include "tommath.h"
#ifdef BN_MP_SHRINK_C

/* shrink a bignum */
int mp_shrink(mp_int * a)
{
	mp_digit *tmp;
	if(a->alloc != a->used && a->used > 0){
		if ((tmp = OPT_CAST(mp_digit) XREALLOC (a->dp, sizeof (mp_digit) * a->used)) == NULL){
			return MP_MEM;
		}
		a->dp    = tmp;
		a->alloc = a->used;
	}
	return MP_OKAY;
}
#endif
