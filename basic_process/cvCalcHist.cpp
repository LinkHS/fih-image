/**
 * @function calcHist_Demo.cpp
 * @brief Demo code to use the function calcHist
 * @author
 */
#include "opencv2/highgui/highgui.hpp"
//#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>

using namespace std;
using namespace cv;

int calcHist_main2( char *pName )
{
	Mat src, dst, hsv;

	/// Load image
	src = imread( pName );

	if( src.empty() )
	{ return -1; }

	cvtColor(src, hsv, CV_BGR2HSV);

	/// Separate the image in 3 places ( B, G and R )
	vector<Mat> bgr_planes;
	split( hsv, bgr_planes );

	/// Establish the number of bins
	int histSize = 240;

	/// Set the ranges ( for B,G,R) )
	float range[] = { 0, 240 } ;
	const float* histRange = { range };

	bool uniform = true; bool accumulate = false;

	Mat h_hist, s_hist, v_hist;

	/// Compute the histograms:
	calcHist( &bgr_planes[0], 1, 0, Mat(), h_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &bgr_planes[1], 1, 0, Mat(), s_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &bgr_planes[2], 1, 0, Mat(), v_hist, 1, &histSize, &histRange, uniform, accumulate );

	// Draw the histograms for B, G and R
	int hist_w = 512; int hist_h = 256;
	int bin_w = cvRound( (double) hist_w/histSize );

	Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 255,255,255) );

	/// Normalize the result to [ 0, histImage.rows ]
	normalize(h_hist, h_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(s_hist, s_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(v_hist, v_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );

	/// Draw for each channel
	for( int i = 1; i < histSize; i++ )
	{
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(h_hist.at<float>(i-1)) ) ,
						Point( bin_w*(i), hist_h - cvRound(h_hist.at<float>(i)) ),
						Scalar( 255, 0, 0), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(s_hist.at<float>(i-1)) ) ,
						Point( bin_w*(i), hist_h - cvRound(s_hist.at<float>(i)) ),
						Scalar( 0, 255, 0), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(v_hist.at<float>(i-1)) ) ,
						Point( bin_w*(i), hist_h - cvRound(v_hist.at<float>(i)) ),
						Scalar( 0, 0, 255), 2, 8, 0  );
	}

	/// Display
	namedWindow(pName, WINDOW_AUTOSIZE );
	imshow(pName, histImage );
	//waitKey(0);

	return 0;
}

int calcHist_main1( char *pName )
{
	Mat src, hsv;
	if( !(src=imread(pName)).data )
		return -1;
	
	cvtColor(src, hsv, CV_BGR2HSV);
	
	// Quantize the hue to 30 levels
	// and the saturation to 32 levels
	int hbins = 30, sbins = 32;
	int histSize[] = {hbins, sbins};
	// hue varies from 0 to 179, see cvtColor

	float hranges[] = { 0, 180 };
	// saturation varies from 0 (black-gray-white) to
	// 255 (pure spectrum color)
	float sranges[] = { 0, 256 };
	const float* ranges[] = { hranges, sranges };
	MatND hist;
	// we compute the histogram from the 0-th and 1-st channels
	int channels[] = {0, 1};
	
	calcHist( &hsv, 1, channels, Mat(), // do not use mask
			  hist, 2, histSize, ranges,
              true, // the histogram is uniform
	          false );
	double maxVal=0;
	minMaxLoc(hist, 0, &maxVal, 0, 0);
	
	int scale = 10;
	Mat histImg = Mat::zeros(sbins*scale, hbins*10, CV_8UC3);
	
	for( int h = 0; h < hbins; h++ )
		for( int s = 0; s < sbins; s++ )
		{
			float binVal = hist.at<float>(h, s);
			int intensity = cvRound(binVal*255/maxVal);
			rectangle( histImg, Point(h*scale, s*scale),
			           Point( (h+1)*scale - 1, (s+1)*scale - 1),
					   Scalar::all(intensity),
						CV_FILLED );
		}

	namedWindow( "Source", 1 );
	imshow( "Source", src );

	namedWindow( "H-S Histogram", 1 );
	imshow( "H-S Histogram", histImg );
	waitKey();

	return 0;
}

/**
 * @function main
 */
int calcHist_main( char *pName )
{
	Mat src, dst;

	/// Load image
	src = imread( pName );

	if( src.empty() )
	{ return -1; }

	/// Separate the image in 3 places ( B, G and R )
	vector<Mat> bgr_planes;
	split( src, bgr_planes );

	/// Establish the number of bins
	int histSize = 256;

	/// Set the ranges ( for B,G,R) )
	float range[] = { 0, 256 } ;
	const float* histRange = { range };

	bool uniform = true; bool accumulate = false;

	Mat b_hist, g_hist, r_hist;

	/// Compute the histograms:
	calcHist( &bgr_planes[0], 1, 0, Mat(), b_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &bgr_planes[1], 1, 0, Mat(), g_hist, 1, &histSize, &histRange, uniform, accumulate );
	calcHist( &bgr_planes[2], 1, 0, Mat(), r_hist, 1, &histSize, &histRange, uniform, accumulate );

	// Draw the histograms for B, G and R
	int hist_w = 512; int hist_h = 400;
	int bin_w = cvRound( (double) hist_w/histSize );

	Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );

	/// Normalize the result to [ 0, histImage.rows ]
	normalize(b_hist, b_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(g_hist, g_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );
	normalize(r_hist, r_hist, 0, histImage.rows, NORM_MINMAX, -1, Mat() );

	/// Draw for each channel
	for( int i = 1; i < histSize; i++ )
	{
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(b_hist.at<float>(i-1)) ) ,
						Point( bin_w*(i), hist_h - cvRound(b_hist.at<float>(i)) ),
						Scalar( 255, 0, 0), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(g_hist.at<float>(i-1)) ) ,
						Point( bin_w*(i), hist_h - cvRound(g_hist.at<float>(i)) ),
						Scalar( 0, 255, 0), 2, 8, 0  );
		line( histImage, Point( bin_w*(i-1), hist_h - cvRound(r_hist.at<float>(i-1)) ) ,
						Point( bin_w*(i), hist_h - cvRound(r_hist.at<float>(i)) ),
						Scalar( 0, 0, 255), 2, 8, 0  );
	}

	/// Display
	namedWindow("calcHist Demo", WINDOW_AUTOSIZE );
	imshow("calcHist Demo", histImage );

	waitKey(0);

	return 0;
}

