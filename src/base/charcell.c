#include "base/charcell.h"

int
color_eq (struct color a, struct color b)
{
	if(a.type != b.type) {
		return 0;
	}
	switch(a.type) {
	case CLR_USR:
		return a.data.usr == b.data.usr;
	case CLR_256:
		return a.data.c256 == b.data.c256;
	case CLR_RGB:
		return a.data.rgb.r == b.data.rgb.r &&
		       a.data.rgb.g == b.data.rgb.g &&
		       a.data.rgb.b == b.data.rgb.b;
	default:
		return 1;
	}
}

