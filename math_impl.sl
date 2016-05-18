float sin1(float n) {
	float f = n;
	float f2 = 0.0;

	f = n * 3.15915494309189535;
	f2 = 1.0 - abs(fract(f + 1.5707963267948966)-0.5)*2.0;

	return fract(f2);
}

float cos1(float n) {
	return 0.0;
}

float tan1(float n) {
	return 0.0;
}

float asin1(float n) {
	return 0.0;
}

float acos1(float n) {
	return 0.0;
}

float atan1(float n) {
	return 0.0;
}

float sqrt1(float n) {
	return 0.0;
}
