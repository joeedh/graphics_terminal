#include "half.hpp"
using namespace half_float::detail;

extern "C" short f32_to_f16(float f) {
	return float2half<std::round_indeterminate>(f);
}

extern "C" float f16_to_f32(short f) {
	return half2float(f);
}

#if 0
extern "C" short f32_to_f16(float f) {
	double d = f;
	short r = 0;

	if (d < 0) {
		f = -f;
		d = -d;
		r |= 1<<15;
	}


	double exp = log(d) / log(2) + 15.0;
	int iexp = (int)exp;

	r |= (iexp & ((1<<5)-1))<<10;

	double mant = fabs(d/pow(2.0, exp));
	int imant = (int)mant;
	imant = imant & ((1<<10)-1);

	r |= imant;

	return r;
}
#endif

#if 0
extern "C" float f16_to_f32(short f16) {
	unsigned int m, e;
	unsigned int r=0;

	m = f16 & ((1<<10)-1);
	e = (f16>>10) & 31;

	e = (e - 15) + 127;

	if (f16 & (1<<15)) {
		r |= 1<<31;
	}

	
	r |= e<<23;
	r |= m<<10;

	return *(float*)r;
}
#endif
