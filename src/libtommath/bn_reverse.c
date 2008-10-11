#include "tommath.h"
#ifdef BN_REVERSE_C

/* reverse an array, used for radix code */
void bn_reverse(unsigned char *s, int len)
{
	int ix, iy;
	unsigned char t;

	ix = 0;
	iy = len - 1;
	while(ix < iy){
		t     = s[ix];
		s[ix] = s[iy];
		s[iy] = t;
		++ix;
		--iy;
	}
}
#endif
