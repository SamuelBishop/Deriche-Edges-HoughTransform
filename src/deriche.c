//
// Created by Arjun on 4/30/18.
//

#include "deriche.h"

/**
 * @brief Applies the Deriche Filter to the source image by running the following passes: L-to-R horizontal, R-to-L
 * horizontal, T-to-B vertical, B-to-T vertical. Output is stored inplace in the source matrix.
 * @param image Source Mat, Output Mat
 * @param coeffs Deriche coefficients pre-generated with known alpha
 */
void applyDericheFilter(Mat *image, const Coeffs *coeffs) {

    const unsigned long height = image->height;
    const unsigned long width  = image->width;

    Mat *out = Mat_generate(width, height, 0);

    long i = 0, j = 0;

    // horizontal pass
    for(i = 0; i < height; i++)
    {
        float X_0=0, X_1=0, X_2=0, Y_0=0, Y_1=0, Y_2=0;

        // left to right pass
        for(j = 0; j < width; j++)
        {
            unsigned long idx = (i * width) + j;
            X_1 = X_0;
            X_0 = image->data[idx];
            Y_2 = Y_1;
            Y_1 = Y_0;
            Y_0 = (coeffs->a1 * X_0) + (coeffs->a2 * X_1) + (coeffs->b1 * Y_1) + (coeffs->b2 * Y_2);
            out->data[idx] = Y_0;
        }

        X_2 = 0;
        X_1 = 0;
        X_0 = 0;

        Y_2 = 0;
        Y_1 = 0;
        Y_0 = 0;

        // right to left pass
        for(j = (width - 1); j>= 0; j--)
        {
            unsigned long idx = (i * width) + j;
            X_2 = X_1;
            X_1 = X_0;
            X_0 = image->data[idx];

            Y_2 = Y_1;
            Y_1 = Y_0;
            Y_0 = (coeffs->a3 * X_1) + (coeffs->a4 * X_2) + (coeffs->b1 * Y_1) + (coeffs->b2 * Y_2);
            out->data[idx] = coeffs->c1 * (Y_0 + out->data[idx]);
        }
    }


    // vertical pass
    for(j = 0; j < width; j++)
    {

        float X_0 = 0, X_1 = 0, X_2 = 0, Y_0 = 0, Y_1 = 0, Y_2 = 0;

        // top to bottom
        for(i = 0; i < height ; i++)
        {
            unsigned long idx = (i * width) + j;
            X_1 = X_0;
            X_0 = out->data[idx];
            Y_2 = Y_1;
            Y_1 = Y_0;
            Y_0 = (coeffs->a5 * X_0) + (coeffs->a6 * X_1) + (coeffs->b1 * Y_1) + (coeffs->b2 * Y_2);
            image->data[idx] = Y_0;
        }

        X_2 = 0;
        X_1 = 0;
        X_0 = 0;
        Y_2 = 0;
        Y_1 = 0;
        Y_0 = 0;

        // bottom to top
        for(i = (height - 1); i >= 0; i--)
        {
            unsigned long idx = (i * width) + j;
            X_2 = X_1;
            X_1 = X_0;
            X_0 = out->data[idx];
            Y_2 = Y_1;
            Y_1 = Y_0;
            Y_0 = (coeffs->a7 * X_1) + (coeffs->a8 * X_2) + (coeffs->b1 * Y_1) + (coeffs->b2 * Y_2);
            image->data[idx] = coeffs->c2 * (Y_0 + image->data[idx]);
        }
    }
    Mat_destroy(out);
}

void hysteresisConnect(Mat *image, const size_t x, const size_t y, const int low) {

    const size_t width = image->width;
    const size_t height = image->height;

    long xx, yy;

    for (xx = x - 1; xx <= x + 1; xx++)
    {
        for (yy = y - 1; yy <= y + 1; yy++)
        {
            if ((xx < width) && (yy < height) && (xx >= 0) && (yy >= 0) && (xx != x) && (yy != y))
            {
                const size_t idx = (yy * width) + xx;
                const float pxl_val = image->data[idx];

                if (pxl_val != 255.0)
                {
                    if (pxl_val >= low)
                    {
                        image->data[idx] = 255.0;
                        hysteresisConnect(image, xx, yy, low);
                    }
                    else
                    {
                        image->data[idx] = 0.0;
                    }
                }
            }
        }
    }
}

unsigned int hysteresisThreshold(Mat *image, const int low, const int high) {

    const size_t width  = image->width;
    const size_t height = image->height;

    unsigned int numFound = 0;
    size_t x,y;
    for(x = 1; x < width; x++)
    {
        for (y = 1; y < height; y++)
        {
            const size_t idx = (y * width) + x;
            const float pxl = image->data[idx];

            if(pxl >= high)
            {
                numFound++;
                image->data[idx] = 255.0;
                hysteresisConnect(image, x, y, low);
            }
        }
    }

    for(x = 0; x < width; x++)
    {
        for (y = 0; y < height; y++)
        {
            const size_t idx = (y * width) + x;
            const float pxl = image->data[idx];

            if(pxl != 255.0)
            {
                image->data[idx] = 0.0;
            }
        }
    }
    return numFound;
}

float fillBlurCoeffs(Coeffs *blurCoeffs, const float ALPHA) {

    if(blurCoeffs == NULL)
    {
        exit(1);
    }

    const float k = (float)((cosh(ALPHA) - 1.0f)/(ALPHA + sinh(ALPHA)));
    const float expNegOneAlpha = expf(-1.0f * ALPHA);
    const float expNegTwoAlpha = expf(-2.0f * ALPHA);

    const float k_expNegOneAlpha = k * expNegOneAlpha;
    const float k_expNegOneAlpha_A = k_expNegOneAlpha * ALPHA;


    /** Prepare co-efficients for blurring pass **/
    blurCoeffs->a1 = k;
    blurCoeffs->a2 = k_expNegOneAlpha_A - k_expNegOneAlpha;
    blurCoeffs->a3 = k_expNegOneAlpha_A + k_expNegOneAlpha;
    blurCoeffs->a4 = -(k * expNegTwoAlpha);
    blurCoeffs->a5 = blurCoeffs->a1;
    blurCoeffs->a6 = blurCoeffs->a2;
    blurCoeffs->a7 = blurCoeffs->a3;
    blurCoeffs->a8 = blurCoeffs->a4;
    blurCoeffs->b1 = 2.0f * expNegOneAlpha;
    blurCoeffs->b2 = -(expNegTwoAlpha);
    blurCoeffs->c1 = 1.0f;
    blurCoeffs->c2 = 1.0f;

    // return this for Xgrad_c1 calculation
    return (1.0f - expNegOneAlpha);
}
/**
 *
 * @param ALPHA
 * @return
 */
DericheCoeffs *DericheCoeffs_generate(const float ALPHA) {

    Coeffs *blurCoeffs = (Coeffs *) malloc(sizeof(Coeffs));
    Coeffs *xDerivativeCoeffs = (Coeffs *) malloc(sizeof(Coeffs));
    Coeffs *yDerivativeCoeffs = (Coeffs *) malloc(sizeof(Coeffs));

    DericheCoeffs *dericheCoeffs = (DericheCoeffs *) malloc(sizeof(DericheCoeffs));

    if(blurCoeffs == NULL || xDerivativeCoeffs == NULL || yDerivativeCoeffs == NULL || dericheCoeffs == NULL)
    {
        perror("unable to allocate memory for deriche coefficients. exiting.");
        exit(1);
    }

    /** generate the blur coefficients **/
    const float oneMinusExpNegOneAlpha = fillBlurCoeffs(blurCoeffs, ALPHA);

    /** Prepare co-efficients for x-derivative pass **/
    xDerivativeCoeffs->a1 =  0.0f;
    xDerivativeCoeffs->a2 =  1.0f;
    xDerivativeCoeffs->a3 = -1.0f;
    xDerivativeCoeffs->a4 =  0.0f;
    xDerivativeCoeffs->a5 = blurCoeffs->a1;
    xDerivativeCoeffs->a6 = blurCoeffs->a2;
    xDerivativeCoeffs->a7 = blurCoeffs->a3;
    xDerivativeCoeffs->a8 = blurCoeffs->a4;
    xDerivativeCoeffs->b1 = blurCoeffs->b1;
    xDerivativeCoeffs->b2 = blurCoeffs->b2;
    xDerivativeCoeffs->c1 = -(oneMinusExpNegOneAlpha * oneMinusExpNegOneAlpha);
    xDerivativeCoeffs->c2 =  1.0f;

    /** Prepare co-efficients for y-derivative pass **/
    yDerivativeCoeffs->a1 = xDerivativeCoeffs->a5;
    yDerivativeCoeffs->a2 = xDerivativeCoeffs->a6;
    yDerivativeCoeffs->a3 = xDerivativeCoeffs->a7;
    yDerivativeCoeffs->a4 = xDerivativeCoeffs->a8;
    yDerivativeCoeffs->a5 = xDerivativeCoeffs->a1;
    yDerivativeCoeffs->a6 = xDerivativeCoeffs->a2;
    yDerivativeCoeffs->a7 = xDerivativeCoeffs->a3;
    yDerivativeCoeffs->a8 = xDerivativeCoeffs->a4;
    yDerivativeCoeffs->b1 = xDerivativeCoeffs->b1;
    yDerivativeCoeffs->b2 = xDerivativeCoeffs->b2;
    yDerivativeCoeffs->c1 = xDerivativeCoeffs->c2;
    yDerivativeCoeffs->c2 = xDerivativeCoeffs->c1;

    dericheCoeffs->blur = blurCoeffs;
    dericheCoeffs->xGradient = xDerivativeCoeffs;
    dericheCoeffs->yGradient = yDerivativeCoeffs;

    return dericheCoeffs;
}

void DericheCoeffs_destroy(DericheCoeffs *dericheCoeffs) {

    if(dericheCoeffs == NULL || dericheCoeffs->blur == NULL || dericheCoeffs->xGradient == NULL || dericheCoeffs->yGradient == NULL)
    {
        return;
    }
    free(dericheCoeffs->blur);
    free(dericheCoeffs->xGradient);
    free(dericheCoeffs->yGradient);
    free(dericheCoeffs);
}


void calculateGradientIntensities(Mat *xGrad, Mat* yGrad) {

    if(xGrad == NULL || yGrad == NULL || xGrad->width != yGrad->width || xGrad->height != yGrad->height || xGrad->data==NULL || yGrad->data==NULL)
        return;

    const size_t pixel_count = yGrad->width * yGrad->height;

    size_t i,j;

    float *directions = malloc(pixel_count* sizeof(float));

    if(directions == NULL)
        return;

    // set location of gradient magnitudes
    float *gradients = xGrad->data;

    // compute magnitude using hypotf
    for(i = 0; i < pixel_count; i++)
    {
        const float gradient = hypotf(xGrad->data[i], yGrad->data[i]);
        //const float direction = (fmod(atan2f(xGrad->data[i], yGrad->data[i]) + M_PI, M_PI) / M_PI) * 8.0f;
        if(gradient > 0)
        {
            const float CC_DIR = (float) (fmod(atan2f(xGrad->data[i], yGrad->data[i]) + M_PI, M_PI) / M_PI) * 8.0f;
            if(CC_DIR <=1 || CC_DIR > 7)  directions[i] = 0; // 0 degrees is 0
            if(CC_DIR > 1 && CC_DIR <= 3) directions[i] = 2; // 45 degrees is 2
            if(CC_DIR > 3 && CC_DIR <= 5) directions[i] = 4; // 90 degrees is 4
            if(CC_DIR > 5 && CC_DIR <= 7) directions[i] = 6; // 135 degrees is 6
            //printf("direction: %f, dir: %f\n", directions[i], CC_DIR);
        }
        gradients[i] = gradient;
    }

    const size_t height = yGrad->height;
    const size_t width = yGrad->width;

    // with padding = 1 on left, right , up , down
    float NW_MAG, NN_MAG, NE_MAG, WW_MAG, CC_MAG, EE_MAG, SW_MAG, SS_MAG, SE_MAG;

    size_t EE_POS, NE_POS, SE_POS;

    for(i = 1; i < height - 1; i++)
    {

        NW_MAG = gradients[width * (i-1) + 0];
        NN_MAG = gradients[width * (i-1) + 1];
        NE_MAG = gradients[width * (i-1) + 2];

        WW_MAG = gradients[width * (i+0) + 0];
        CC_MAG = gradients[width * (i+0) + 1];
        EE_MAG = gradients[width * (i+0) + 2];

        SW_MAG = gradients[width * (i+1) + 0];
        SS_MAG = gradients[width * (i+1) + 1];
        SE_MAG = gradients[width * (i+1) + 2];

        // left to right
        for(j = 1; j < width - 1; j++)
        {
            const size_t idx = (i*width) + j;

            WW_MAG = CC_MAG;
            CC_MAG = EE_MAG;

            SW_MAG = SS_MAG;
            SS_MAG = SE_MAG;

            NW_MAG = NN_MAG;
            NN_MAG = NE_MAG;

            EE_POS = idx + 1;
            NE_POS = EE_POS - width;
            SE_POS = EE_POS + width;

            // fetch or compute NE_MAG
            NE_MAG = gradients[NE_POS];
            EE_MAG = gradients[EE_POS];
            SE_MAG = gradients[SE_POS];


            if(CC_MAG > 0)
            {
                const float CC_DIR = directions[idx];
                if(CC_DIR == 0 && (CC_MAG <= EE_MAG || CC_MAG <= WW_MAG)) CC_MAG = 0; // 0 degrees
                if(CC_DIR == 2 && (CC_MAG <= NE_MAG || CC_MAG <= SW_MAG)) CC_MAG = 0; // 45 degrees
                if(CC_DIR == 4 && (CC_MAG <= NN_MAG || CC_MAG <= SS_MAG)) CC_MAG = 0; // 90 degrees
                if(CC_DIR == 6 && (CC_MAG <= NW_MAG || CC_MAG <= SE_MAG)) CC_MAG = 0; // 135 degrees
            }

            yGrad->data[idx] = CC_MAG ;//> 0 ? C : 0;
        }
    }

    free(directions);
}

void performMagnitudeSupression(Mat *xGrad, Mat* yGrad) {

    if(xGrad == NULL || yGrad == NULL || xGrad->width != yGrad->width || xGrad->height != yGrad->height || xGrad->data==NULL || yGrad->data==NULL)
        return;


    const size_t height = xGrad->height;
    const size_t width = xGrad->width;

    const size_t pixel_count = width * height;

    float *directions = malloc(pixel_count* sizeof(float));

    if(directions == NULL)
        return;

    // set location of gradient magnitudes
    float *gradients = xGrad->data;

    unsigned int i, j;
    // compute magnitude using hypotf
    for(i = 0; i < pixel_count; i++)
    {
        gradients[i] = hypotf(xGrad->data[i], yGrad->data[i]);
        directions[i] = (float)(fmod(atan2f(xGrad->data[i], yGrad->data[i]) + M_PI, M_PI) / M_PI) * 8.0f;
    }

    for (i = 1; i < width - 1; i++)
    {
        for (j = 1; j < height - 1; j++)
        {
            const size_t cc = i + width * j;
            const float     G_cc = gradients[cc];

            /** Nothing to do here if G_cc is not 255.0! **/
//            if(G_cc < MAGNITUDE_LIMIT)
//            {
//                yGrad->data[cc] = 0.0f;
//                continue;
//            }

            /** Set indices for 8-neighbors **/
            const size_t nn = cc - width;
            const size_t ss = cc + width;
            const size_t ww = cc + 1;
            const size_t ee = cc - 1;
            const size_t nw = nn + 1;
            const size_t ne = nn - 1;
            const size_t sw = ss + 1;
            const size_t se = ss - 1;

            /** get gradients 8-neighbors **/
            const float G_ee = (i > 0)                                 ? gradients[ee] : 0.0f;
            const float G_ww = (i < (width - 1))                       ? gradients[ww] : 0.0f;
            const float G_nw = (j > 0)            && (i < (width - 1)) ? gradients[nw] : 0.0f;
            const float G_nn = (j > 0)                                 ? gradients[nn] : 0.0f;
            const float G_ne = (j > 0)            && (i > 0)           ? gradients[ne] : 0.0f;
            const float G_se = (j < (height - 1)) && (i > 0)           ? gradients[se] : 0.0f;
            const float G_ss = (j < (height - 1))                      ? gradients[ss] : 0.0f;
            const float G_sw = (j < (height - 1)) && (i < (width - 1)) ? gradients[sw] : 0.0f;

            const float dir = gradients[cc];

            if (((dir <= 1 || dir > 7) && G_cc > G_ee && G_cc > G_ww) || // 0 deg
                ((dir > 1 && dir <= 3) && G_cc > G_nw && G_cc > G_se) || // 45 deg
                ((dir > 3 && dir <= 5) && G_cc > G_nn && G_cc > G_ss) || // 90 deg
                ((dir > 5 && dir <= 7) && G_cc > G_ne && G_cc > G_sw))   // 135 deg

                yGrad->data[cc] = MAGNITUDE_LIMIT;
            else
                yGrad->data[cc] = 0.0;
        }
    }

    free(directions);
}

void writeCoeffs(Coeffs *coeffs, char *name) {
    printf("%s\n", name);
    printf("a1: %f\n", coeffs->a1);
    printf("a2: %f\n", coeffs->a2);
    printf("a3: %f\n", coeffs->a3);
    printf("a4: %f\n", coeffs->a4);
    printf("a5: %f\n", coeffs->a5);
    printf("a6: %f\n", coeffs->a6);
    printf("a7: %f\n", coeffs->a7);
    printf("a8: %f\n", coeffs->a8);
    printf("b1: %f\n", coeffs->b1);
    printf("b2: %f\n", coeffs->b2);
    printf("c1: %f\n", coeffs->c1);
    printf("c2: %f\n", coeffs->c2);
}

void writeDericheCoeffs(DericheCoeffs *coeffs) {
    if(coeffs == NULL)
        return;

    if(coeffs->blur != NULL)      writeCoeffs(coeffs->blur, "Blur Coeffs");
    if(coeffs->xGradient != NULL) writeCoeffs(coeffs->xGradient, "X-Gradient Coeffs");
    if(coeffs->yGradient != NULL) writeCoeffs(coeffs->yGradient, "Y-Gradient Coeffs");
}
