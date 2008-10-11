#include "tommath.h"
#ifdef BN_MP_MOD_D_C

int mp_mod_d(mp_int * a, mp_digit b, mp_digit * c)
{
	return mp_div_d(a, b, NULL, c);
}
#endif
