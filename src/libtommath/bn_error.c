#include "tommath.h"
#ifdef BN_ERROR_C

static const struct {
	int code;
	char *msg;
} msgs[] = {
	{ MP_OKAY, "Successful" },
	{ MP_MEM,  "Out of heap" },
	{ MP_VAL,  "Value out of range" }
};

/* return a char * string for a given code */
char * mp_error_to_string(int code)
{
	int x;

	/* scan the lookup table for the given message */
	for(x = 0; x < (int)(sizeof(msgs) / sizeof(msgs[0])); x++){
		if(msgs[x].code == code){
			return msgs[x].msg;
		}
	}

	/* generic reply for invalid code */
	return "Invalid error code";
}
#endif
