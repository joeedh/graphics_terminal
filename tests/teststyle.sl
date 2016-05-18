float tent(float f) {
	return 1.0 - abs(fract(f)-0.5)*2.0;
}

r = x*u;
g = y*v;
b = tent((u*u + v*v)*11.2);
