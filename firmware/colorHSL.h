// colorHSL.h

#ifndef __HSL_H__
#define __HSL_H__

#include "colorRGB.h"

class HSL
{
    private:
        float HueToRGB(float v1, float v2, float vH);
    public:
    	HSL(int h, float s, float l);
    	int H;
        float S;
    	float L;
    	bool Equals(HSL hsl);
    	colorRGB* toRGB();
};

#endif