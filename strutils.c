#include "ctype.h"

static const char *hexline = "0123456789ABCDEF";
static const char *decline = "0123456789";

int myatoi(const char *str, int base, int *out) {
	int len = strlen(str), i=0;
	int ret=0, isneg=1;
	char *line = base==16 ? hexline : decline;
	
	*out = -1;

	if (base != 10 && base != 16) {
		return 0;
	}

	while (i < len && (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')) {
		i++;
	}

	if (str[i] == '-' && i < len-1) {
		isneg = -1;
		i++;
	}

	if (base == 16 && i < len-2 && (str[i+1] == 'x' || str[i+1] == 'X')) {
		i += 2;
	}

	for (; i<len; i++) {
		char ch = str[i];
		int j;

		for (j=0; j<base; j++) {
			if (toupper(ch) == line[j]) {
				break;
			}
		}

		if (j == base) { //out of range character
			break;
		}

		ret = ret*base + j;
	}

	*out = ret*isneg;
	return 1;
}
