// colorHSL.cpp

#include "colorHSL.h"
#include "colorRGB.h"
#include <math.h>

HSL::HSL(int h, float s, float l){
    // Ensure we are not using full integers here
	if( s > 1 ){
	    s = s / 100;
	}
	
	if( l > 1 ){
	    l = l / 100;
	}
	// Ensure we are not using full integers here

	H = h;
	S = s;
	L = l;
};

// See if a value is equal to another value
bool HSL::Equals(HSL hsl){
	return (H == hsl.H) && (S == hsl.S) && (L == hsl.L);
};

// Convert the HSL to RGB
colorRGB* HSL::toRGB( ){
	int r = 0;
	int g = 0;
	int b = 0;

	if (S == 0){
		r = g = b = round(L * 255);
	}
	else{
		float v1, v2;
		float hue = (float)H / 360;

		v2 = (L < 0.5) ? (L * (1 + S)) : ((L + S) - (L * S));
		v1 = 2 * L - v2;

		r = (int)(255 * HSL::HueToRGB(v1, v2, hue + (1.0f / 3)));
		g = (int)(255 * HSL::HueToRGB(v1, v2, hue));
		b = (int)(255 * HSL::HueToRGB(v1, v2, hue - (1.0f / 3)));
	}

    return new colorRGB( r, g, b );
};


// Change a hue into RGB
float HSL::HueToRGB(float v1, float v2, float vHue) {
	if (vHue < 0)
		vHue += 1;

	if (vHue > 1)
		vHue -= 1;

	if ((6 * vHue) < 1)
		return (v1 + (v2 - v1) * 6 * vHue);

	if ((2 * vHue) < 1)
		return v2;

	if ((3 * vHue) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vHue) * 6);

	return v1;
};