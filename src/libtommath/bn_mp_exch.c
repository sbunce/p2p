#include "tommath.h"
#ifdef BN_MP_EXCH_C

/* swap the elements of two integers, for cases where you can't simply swap the 
 * mp_int pointers around
 */
void mp_exch(mp_int * a, mp_int * b)
{
	mp_int  t;

	t  = *a;
	*a = *b;
	*b = t;
}
#endif
