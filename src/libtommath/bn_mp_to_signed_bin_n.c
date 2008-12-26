#include "tommath.h"
#ifdef BN_MP_TO_SIGNED_BIN_N_C

/* store in signed [big endian] format */
int mp_to_signed_bin_n(mp_int * a, unsigned char *b, unsigned long *outlen)
{
	if(*outlen < (unsigned long)mp_signed_bin_size(a)){
		return MP_VAL;
	}
	*outlen = mp_signed_bin_size(a);
	return mp_to_signed_bin(a, b);
}
#endif
