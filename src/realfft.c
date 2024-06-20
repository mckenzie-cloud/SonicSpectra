
#include <stdio.h>
#include <math.h>
#include "../include/fourier.h"

/***************************************************************************************************************************************
 * Reference: Press, William H., et al. Numerical Recipes: The Art of Scientific Computing. 3rd ed., Cambridge University Press, 2007. * 
 ***************************************************************************************************************************************/

#define PI 3.141592653589793238

/***************************************************
 * Chapter: 12.3.2 - FFT of a Single Real Function *
 * Page: 618 - 620                                 *
 * *************************************************/
void realft(float data[], size_t n, const int isign)
{
    //----------------------------------------------
    if (n < 2 || n & (n-1))
    {
        printf("Error: n must be a power of 2!\n");
        return;
    }

    //----------------------------------------------
    int i1, i2, i3, i4;
    float c1=0.5, c2, h1r, h1i, h2r, h2i, wr, wi, wpr, wpi, wtemp;
    float theta = PI/(float)(n>>1);

    //----------------------------------------------
    if (isign == 1) {
        c2 = -0.5;
        four1(data, n>>1, 1);
    } else {
        c2 = 0.5;
        theta = -theta;
    }

    //----------------------------------------------
    wtemp = sin(0.5*theta);
    wpr   = -2.0*wtemp*wtemp;
    wpi   = sinf(theta);
    wr    = 1.0+wpr;
    wi    = wpi;

    //----------------------------------------------
    for (size_t i=1;i<(n>>2);i++)
    {
        i2  =  1+(i1=i+i);
        i4  =  1+(i3=n-i1);
        h1r =  c1*(data[i1]+data[i3]);
        h1i =  c1*(data[i2]-data[i4]);
        h2r = -c2*(data[i2]+data[i4]);
        h2i =  c2*(data[i1]-data[i3]);
        data[i1]  =  h1r+wr*h2r-wi*h2i;
        data[i2]  =  h1i+wr*h2i+wi*h2r;
        data[i3]  =  h1r-wr*h2r+wi*h2i;
        data[i4]  = -h1i+wr*h2i+wi*h2r;
        wr = (wtemp=wr)*wpr-wi*wpi+wr;
        wi = wi*wpr+wtemp*wpi+wi;
    }
    //----------------------------------------------
    if (isign == 1)
    {
        data[0] = (h1r=data[0])+data[1];
        data[1] =  h1r-data[1];
    } else {
        data[0] = c1*((h1r=data[0])+data[1]);
        data[1] = c1*(h1r-data[1]);
        four1(data, n>>1, -1);
    }
}
