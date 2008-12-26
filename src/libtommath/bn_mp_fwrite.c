#include "tommath.h"
#ifdef BN_MP_FWRITE_C

int mp_fwrite(mp_int * a, int radix, FILE * stream)
{
	char *buf;
	int err, len, x;
   
	if((err = mp_radix_size(a, radix, &len)) != MP_OKAY){
		return err;
	}

	buf = OPT_CAST(char) XMALLOC(len);
	if(buf == NULL){
		return MP_MEM;
	}
   
	if((err = mp_toradix(a, buf, radix)) != MP_OKAY){
		XFREE(buf);
		return err;
	}
   
	for(x = 0; x < len; x++){
		if(fputc(buf[x], stream) == EOF){
			XFREE(buf);
			return MP_VAL;
		}
	}
   
	XFREE(buf);
	return MP_OKAY;
}
#endif
