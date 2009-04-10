#include "tommath.h"
#ifdef BN_MP_SIGNED_BIN_SIZE_C

/* get the size for an signed equivalent */
int mp_signed_bin_size(mp_int * a)
{
	return 1 + mp_unsigned_bin_size(a);
}
#endif
