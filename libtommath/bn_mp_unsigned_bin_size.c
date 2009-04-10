#include "tommath.h"
#ifdef BN_MP_UNSIGNED_BIN_SIZE_C

/* get the size for an unsigned equivalent */
int mp_unsigned_bin_size(mp_int * a)
{
	int size = mp_count_bits (a);
	return (size / 8 + ((size & 7) != 0 ? 1 : 0));
}
#endif
