// colorRGB.h

#ifndef __RGB_H__
#define __RGB_H__

class colorRGB
{
    public:
		colorRGB( int inputR, int inputG, int inputB );
		void setValue(  int inputR, int inputG, int inputB  );
    	int R;
    	int G;
    	int B;
};

#endif