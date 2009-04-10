#include "tommath.h"
#ifdef BN_MP_INIT_SET_C

/* initialize and set a digit */
int mp_init_set(mp_int * a, mp_digit b)
{
	int err;
	if((err = mp_init(a)) != MP_OKAY){
		return err;
	}
	mp_set(a, b);
	return err;
}
#endif
