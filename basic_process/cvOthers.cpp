#include "basic_process/cvOthers.h"
#include "ImageUtils.h"

using namespace cv;

void cvSaturation(Mat& mSrc, Mat &mDst, int increment)
{
	// increment (-100.0, 100.0)  

    int channels = mSrc.channels();
    int nRows = mSrc.rows;
    int nCols = mSrc.cols * channels;
    if( mSrc.isContinuous() )
    {
        nCols *= nRows;
        nRows = 1;
    }

  //#pragma omp parallel for 
    for( int i=0; i<nRows; ++i )
	{  
        uchar* pSrc = mSrc.ptr<uchar>(i);
  #pragma omp parallel for 
        for( int j=0; j<nCols; j+=3)
        {

    		int t1 = pSrc[j+0];
    		int t2 = pSrc[j+1];
    		int t3 = pSrc[j+2];
    		int minVal = std::min(std::min(t1,t2), t3);  
    		int maxVal = std::max(std::max(t1,t2), t3);

            //float delta = maxVal - minVal;  
            //float L = 0.5*(maxVal + minVal);  
            //float S = max(0.5*delta/L, 0.5*delta/(255-L)) * 100;
    		float delta = (float)(maxVal-minVal);  
    		float L = (float)(maxVal+minVal);  
    		float S = std::max(delta/L, delta/(510-L));  

            if (increment > 0)
    		{  
    			float alpha = max(S, 1-(float)increment/100);  
    			alpha = 1.0/alpha-1;  
    			pSrc[j+0] = (t1 + (t1 - L/2.0) * alpha);  
    			pSrc[j+1] = (t2 + (t2 - L/2.0) * alpha);  
    			pSrc[j+2] = (t3 + (t3 - L/2.0) * alpha);  
    		}  
			else  
    		{  
    			//alpha = increment;  
    			//pSrc[j+0] = (L + (t1 - L) * (1+alpha));  
    			//pSrc[j+1] = (L + (t2 - L) * (1+alpha));  
    			//pSrc[j+2] = (L + (t3 - L) * (1+alpha));  
    		} 
        } 
	}  
}

void cvSaturation_float(Mat& mSrc, Mat &mDst, float increment)
{
	// increment (-100.0, 100.0)  
    increment = increment/100.0;  

    int channels = mSrc.channels();
    int nRows = mSrc.rows;
    int nCols = mSrc.cols * channels;
    if( mSrc.isContinuous() )
    {
        nCols *= nRows;
        nRows = 1;
    }

    for( int i=0; i<nRows; ++i )
	{  
        uchar* pSrc = mSrc.ptr<uchar>(i);
  #pragma omp parallel for 
        for( int j=0; j<nCols; j+=3)
        {
        	float alpha; 

    		float t1 = pSrc[j];
    		float t2 = pSrc[j+1];
    		float t3 = pSrc[j+2];

    		float minVal = std::min(std::min(t1,t2), t3);  
    		float maxVal = std::max(std::max(t1,t2), t3);  
    		float delta = (maxVal-minVal)/255.0;  
    		float L = 0.5*(maxVal+minVal)/255.0;  
    		float S = std::max(0.5*delta/L, 0.5*delta/(1-L));  

            if (increment > 0)
    		{  
    			alpha = max(S, 1-increment);  
    			alpha = 1.0/alpha-1;  
    			pSrc[j+0] = (t1 + (t1 - L*255.0) * alpha);  
    			pSrc[j+1] = (t2 + (t2 - L*255.0) * alpha);  
    			pSrc[j+2] = (t3 + (t3 - L*255.0) * alpha);  
    		}  
    		else  
    		{  
    			alpha = increment;  
    			pSrc[j+0] = (L*255.0 + (t1 - L*255.0) * (1+alpha));  
    			pSrc[j+1] = (L*255.0 + (t2 - L*255.0) * (1+alpha));  
    			pSrc[j+2] = (L*255.0 + (t3 - L*255.0) * (1+alpha));  
    		} 
        } 
	}  
}

void cvCorrectGamma( Mat& mSrc, Mat &mDst, double gamma )
{
    if ( gamma == 1 ) {
        mDst = mSrc;
        return;
    }

    double inverse_gamma = 1.0 / gamma;

	Mat lut_matrix(1, 256, CV_8UC1 );
	uchar * ptr = lut_matrix.ptr();
	for( int i = 0; i < 256; i++ )
		ptr[i] = (int)( pow( (double) i / 255.0, inverse_gamma ) * 255.0 );

	LUT( mSrc, lut_matrix, mDst );
}

void GenerateRGB(void)
{
	Mat M(500,300, CV_8UC3, Scalar(50,60,183));
	imwrite("R183_G60_B50.jpg",M);
}

void GenerateGrayMulti(void)
{
	Mat mDst = Mat::zeros( 480, 480, CV_8UC1 );

	int channels = mDst.channels();
	int nRows = mDst.rows;
	int nCols = mDst.cols * channels;

	for( int i=0; i<nRows; ++i)
	{
		uchar* p = mDst.ptr<uchar>(i);
		for( int j=0; j<nCols; j++)
		{
			p[j] = j/30 + (i/30)*16;
		}
	}

	imwrite("255LevelGray.jpg", mDst);
}

void Sobel(Mat& mSrc)
{
	Mat mSrc_gray;
	cvtColor( mSrc, mSrc_gray, CV_RGB2GRAY );

	/// Generate grad_x and grad_y
	Mat grad, grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;

	Sobel( mSrc_gray, grad_x, CV_16S, 1, 0, 3, 1, 0, BORDER_DEFAULT );
	convertScaleAbs( grad_x, abs_grad_x );

	Sobel( mSrc_gray, grad_y, CV_16S, 0, 1, 3, 1, 0, BORDER_DEFAULT );
	convertScaleAbs( grad_y, abs_grad_y );

	/// Total Gradient (approximate)
    addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );

	imwrite("sobelx.jpg",abs_grad_x);
	imwrite("sobely.jpg",abs_grad_y);
	imwrite("sobel.jpg",grad);
}

void LaplaceDemo(Mat& mSrc, Mat& mDst)
{
	Mat src, src_gray, dst;
	int kernel_size = 3;
	int scale = 1;
	int delta = 0;
	int ddepth = CV_16S;
	const char* window_name = "Laplace Demo";

	/// Load an image
	src = mSrc;

	if( !src.data )
	{ return; }

	/// Remove noise by blurring with a Gaussian filter
	GaussianBlur( src, src, Size(3,3), 0, 0, BORDER_DEFAULT );

	/// Convert the image to grayscale
	cvtColor( src, src_gray, COLOR_RGB2GRAY );

	/// Create window
	namedWindow( window_name, WINDOW_AUTOSIZE );

	/// Apply Laplace function
	Mat abs_dst;

	Laplacian( src_gray, dst, ddepth, kernel_size, scale, delta, BORDER_DEFAULT );
	convertScaleAbs( dst, abs_dst );

	/// Show what you got
	imshow( window_name, abs_dst );

	mDst = abs_dst;
}

Mat img, inpaintMask;
Point prevPt(-1,-1);

void onMouse( int event, int x, int y, int flags, void* )
{
    if( event == CV_EVENT_LBUTTONUP || !(flags & CV_EVENT_FLAG_LBUTTON) )
        prevPt = Point(-1,-1);
    else if( event == CV_EVENT_LBUTTONDOWN )
        prevPt = Point(x,y);
    else if( event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON) )
    {
        Point pt(x,y);
        if( prevPt.x < 0 )
            prevPt = pt;
        line( inpaintMask, prevPt, pt, Scalar::all(255), 5, 8, 0 );
        line( img, prevPt, pt, Scalar::all(255), 5, 8, 0 );
        prevPt = pt;
        imshow("image", img);
    }
}

int RemovePoint( Mat& mSrc )
{
    Mat img0 = mSrc;
    if(img0.empty())
    {
        return 0;
    }

    namedWindow( "image", 1 );

    img = img0.clone();
    inpaintMask = Mat::zeros(img.size(), CV_8U);

    imshow("image", img);
    setMouseCallback( "image", onMouse, 0 );

    for(;;)
    {
        char c = (char)waitKey();

        if( c == 27 )
            break;

        if( c == 'r' )
        {
            inpaintMask = Scalar::all(0);
            img0.copyTo(img);
            imshow("image", img);
        }

        if( c == 'i' || c == ' ' )
        {
            Mat inpainted;
            inpaint(img, inpaintMask, inpainted, 3, CV_INPAINT_TELEA);
            imshow("inpainted image", inpainted);
        }
    }

    return 0;
}

/*
 * Extract Y(light) channel from RGB channels.
 * Currently this function only supports for CV_8UC3 type.
 */
void cvConvertAndSplit(Mat& mSrc, Mat* pChannels, int code)
{
    Mat mXYZ;

    cvtColor(mSrc, mXYZ, code);
    split(mXYZ, pChannels);
}

/*
 * Parmameter - code: CV_BGR2GRAY, CV_BGR2YCrCb(default)
 */
void cvMergeAndConvert(Mat* pChannels, Mat& mDst, int code)
{
    Mat mXYZ;

    merge(pChannels, 3, mXYZ);
    cvtColor(mXYZ, mDst, code);
}

/*
 * Replace Y(light) channel in original image with processed Y channel.
 * It assumes that only Y channel in original image has been changed!!!
 * Currently this function only supports for CV_8UC3 && BGR type.
 */
void cvReplaceYChannel(Mat& mSrc, Mat& mDst, Mat& mY)
{
    Mat mYab, mTemp;
    Mat channels[3];

    CV_Assert( mSrc.type() == CV_8UC3 );

    cvtColor(mSrc, mYab, CV_BGR2YCrCb);
    split(mYab, channels);

    channels[0] = mY;
    merge(channels, 3, mTemp);
    cvtColor(mTemp, mDst, CV_YCrCb2BGR);
}

/*------------------------------------------------------------------------------
 * Timer functions
 * Record the execution time of some code, in seconds.
 * eg:
 *  cvStartTimer;
 *    ...
 *  cvStopTimerAndPrint("xxx function");
 */
 /*
static double time;
void cvStartTimer(void)
{
    time = (double)getTickCount();
}

void cvStopTimerAndPrint(const char* pMethodName)
{
    time = (double)getTickCount() - time;
    time = time /getTickFrequency();
    LOGI("%s execution time is %f s:\r\n", pMethodName, time);
}
*/
/*
 * Display an image by using imshow.
   If the rect_size == 0, display it without chaning size.
 */
void cvDisplayMat(Mat &mSrc, int rect_size, const char* pWindowName)
{
    //Maximum 10 windows, declare it as static to avoid overriding previous window.
    static uchar window_index = 0x30;
	Mat mResize;

    if( rect_size > 0)
    { 
    	cvResizeDown(mSrc, mResize, rect_size);
	}
	
    if( pWindowName == NULL )
    {
        imshow((const char *)&window_index, mResize); //00110001
        if( ++window_index == 0x39 )
            window_index = 0x30;
    }else
    {
        imshow(pWindowName, mResize);
    }
}

/*
 * Resize an image to a smaller one without changing the ratio. 
 * If the original size is smaller than the parameter(rect_size), just return.
 */
void cvResizeDown(Mat &mSrc, Mat &mDst, int rect_size)
{
    int width  = mSrc.cols;
    int height = mSrc.rows;
    int max_dim = ( width >= height ) ? width : height;
    
    if( (max_dim > rect_size) && (rect_size > 0))
    {
        double scale = max_dim / rect_size;
        width  = width / scale;
        height = height / scale;
	    resize(mSrc, mDst, Size(width, height));
    }else
    {
        mDst = mSrc;
    }
}

/*
 * Resize an image to a bigger one without changing the ratio. 
 * If the original size is bigger than the parameter(rect_size), just return.
 */
void cvResizeUp(Mat &mSrc, Mat &mDst, int rect_size)
{
    int width  = mSrc.cols;
    int height = mSrc.rows;
    int min_dim = ( width <= height ) ? width : height;
    
    if( (min_dim < rect_size) && (rect_size > 0) )
    {
        double scale = rect_size / min_dim;
        width  = width * scale;
        height = height * scale;
	    resize(mSrc, mDst, Size(width, height));
    }else
    {
        mDst = mSrc;
    }
}

/*
    Color Convert
*/
void ColorCvt_YUYV2BGR(uchar* pYUYV, uchar* pBGR, int length)
{
    for( int j = 0; j < length; )
    {
        uchar Y1 = *pYUYV++; 
        uchar U  = *pYUYV++; 
        uchar Y2 = *pYUYV++; 
        uchar V  = *pYUYV++; 
        
        int C1 = Y1 - 16;
        int C2 = Y2 - 16;
        int D  = U - 128;
        int E  = V - 128;

        pBGR[j++] = saturate_cast<uchar>(( 298 * C1 + 516 * D + 128) >> 8);           //B1
        pBGR[j++] = saturate_cast<uchar>(( 298 * C1 - 100 * D - 208 * E + 128) >> 8); //G1
        pBGR[j++] = saturate_cast<uchar>(( 298 * C1 + 409 * E + 128) >> 8);           //R1

        pBGR[j++] = saturate_cast<uchar>(( 298 * C2 + 516 * D + 128) >> 8);           //B2
        pBGR[j++] = saturate_cast<uchar>(( 298 * C2 - 100 * D - 208 * E + 128) >> 8); //G2
        pBGR[j++] = saturate_cast<uchar>(( 298 * C2 + 409 * E + 128) >> 8);           //R2
    }
}

void ColorCvt_YUYV2BGR(uchar* pYUYV, Mat& mBGR, int width, int height)
{
    Mat mSrcYUYV( height, width, CV_8UC2, pYUYV );

    cvtColor( mSrcYUYV, mBGR, CV_YUV2BGR_YUY2 );
}

void ColorCvt_BGR2YUYV(Mat& mBGR, uchar* pYUYV)
{
    int channels = mBGR.channels();
    int nRows = mBGR.rows;
    int nCols = mBGR.cols * channels;
    if( mBGR.isContinuous() )
    {
        nCols *= nRows;
        nRows = 1;
    }

    for( int i = 0; i < nRows; ++i)
    {
        uchar* pBGR = mBGR.ptr<uchar>(i);
        for( int j = 0; j < nCols; )
        {
            int B = pBGR[j++];
            int G = pBGR[j++];
            int R = pBGR[j++];
                        
            *pYUYV++ = ((66*R + 129*G + 25*B + 128) >> 8) + 16;
            *pYUYV++ = (j&0x01) ? (((-38*R - 74*G + 112*B + 128) >> 8) + 128) : (((112 * R - 94 * G - 18 * B + 128) >> 8) + 128);
        }
    }
}

void cvCalcHist( Mat &mSrc, Mat &mHist )
{
	/// Establish the number of bins
	int histSize = 256;

	/// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 } ;
	const float* histRange = { range };

	bool uniform = true; 
    bool accumulate = false;

	Mat y_hist;

	/// Compute the histograms:
	calcHist( &mSrc, 1, 0, Mat(), y_hist, 1, &histSize, &histRange, uniform, accumulate );

	// Draw the histograms for B, G and R
	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound( (double) hist_w/histSize );

	Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );
    normalize(y_hist, y_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );

	/// Draw for each channel
	for( int i = 1; i < histSize; i++ )
	{
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(y_hist.at<float>(i-1)) ) ,
						 Point( bin_w*(i),   hist_h - cvRound(y_hist.at<float>(i)) ),
						 Scalar( 255, 0, 0), 2, 8, 0  );
	}

    mHist = histImage;
}

