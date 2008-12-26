#include "tommath.h"
#ifdef BN_MP_INIT_SET_INT_C

/* initialize and set a digit */
int mp_init_set_int(mp_int * a, unsigned long b)
{
	int err;
	if((err = mp_init(a)) != MP_OKAY){
		return err;
	}
	return mp_set_int(a, b);
}
#endif
