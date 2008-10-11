#include "tommath.h"
#ifdef BN_MP_TO_SIGNED_BIN_C

/* store in signed [big endian] format */
int mp_to_signed_bin(mp_int * a, unsigned char *b)
{
	int res;

	if((res = mp_to_unsigned_bin(a, b + 1)) != MP_OKAY){
		return res;
	}

	b[0] = (unsigned char)((a->sign == MP_ZPOS) ? 0 : 1);
	return MP_OKAY;
}
#endif
