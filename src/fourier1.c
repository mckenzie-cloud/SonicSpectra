
#include <stdio.h>
#include <math.h>

/***************************************************************************************************************************************
 * Reference: Press, William H., et al. Numerical Recipes: The Art of Scientific Computing. 3rd ed., Cambridge University Press, 2007. * 
 ***************************************************************************************************************************************/

#define TWO_PI 6.28318530717959

void swap(float *a, float *b)
{
    float temp = *a;
    *a = *b;
    *b = temp;
    return;
}

/***************************************************
 * Chapter:  12.2 - Fast Fourier Transform         *
 * Page: 608 - 614                                 *
 * *************************************************/
void four1(float *data, const int n, const int isign)
{
    //----------------------------------------------
    int nn, mmax, m, j, istep, i;
    float wtemp, wr, wpr, wpi, wi, theta, tempr, tempi;

    //----------------------------------------------
    nn = n << 1;;
    j = 1;
    for (i = 1; i < nn; i += 2)          // Bit-reversal section of the routine.
    {
        //----------------------------------------------
        if (j > i)
        {
            swap(&data[j-1], &data[i-1]); // exchange the two complex numbers.
            swap(&data[j], &data[i]);
        }

        //----------------------------------------------
        m = n;
        while (m >= 2 && j > m)
        {
            /* code */
            j -= m;
            m >>= 1;
        }
        j += m;
        //----------------------------------------------
    }
    //----------------------------------------------

    /*############################################################
     # Here begins the Danielson-Lanczos section of the routine. #
     ############################################################*/
    mmax = 2;
    while (nn > mmax)                           // Outer loop executed log2*n times.
    {
        //----------------------------------------------
        istep = mmax << 1;
        theta = isign * (TWO_PI / mmax);       // Initialize the trigonometric recurrence.
        wtemp = sinf(0.5f * theta);
        wpr   = -2.0f * wtemp * wtemp;
        wpi   = sinf(theta);
        wr    = 1.0f;
        wi    = 0.0f;

        //----------------------------------------------
        for (m = 1; m < mmax; m += 2)
        {
            //----------------------------------------------
            for (i = m; i <= nn; i+=istep)
            {
                /* This is the Danielson-Lanczos formula. */
                j = i + mmax;                   
                tempr = wr*data[j-1]-wi*data[j];
                tempi = wr*data[j]+wi*data[j-1];
                data[j-1] = data[i-1]-tempr;
                data[j]   = data[i]-tempi;
                data[i-1] += tempr;
                data[i]   += tempi;
            }
            /* Trigonometric recurrence. */
            wr = (wtemp=wr)*wpr-wi*wpi+wr;
            wi = wi*wpr+wtemp*wpi+wi;
            //----------------------------------------------
        }
        mmax = istep;
        //----------------------------------------------
    }
    //----------------------------------------------
    return;
}
