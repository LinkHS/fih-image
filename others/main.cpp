#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/photo/photo.hpp>
#include <iostream>
using namespace cv;
using namespace std;

extern int OBJ_main( char *pName );
extern void FaceDectect(Mat mSrc, std::vector<Rect> &faces);
extern void SkinPixelsDecect(Mat& mSrc, Mat& mDst);
extern void WYMCAdjustment_RedPixels(Mat& mSrc, Mat& mDst, struct WYMCParams* pWYMCParams);
extern void WYMCAdjustment_BlackPixels(Mat& mSrc, Mat& mDst, struct WYMCParams* pWYMCParams);

extern Mat correctGamma( Mat& img, double gamma );

void test(Mat& mSrc, Mat& mDst)
{
	//Mat mMask;
	//SkinPixelsDecect(mSrc, mMask);
	//bilateralFilter(mSrc, mDst, 4, 32, 4); mSrc = mDst;
	
	mSrc.convertTo(mDst, -1, 1.2, 0);mSrc = mDst;
	//mSrc = correctGamma(mSrc, 1.2);

	//LSV_NoiseFiltering_main(mSrc, mDst); mSrc = mDst;

	WYMCAdjustment_RedPixels(mSrc, mDst, NULL); mSrc = mDst;
	WYMCAdjustment_BlackPixels(mSrc, mDst, NULL); mSrc = mDst;
}

void MFC_test(const char* pName)
{
	Mat mSrc;
	printf("OpenCV1 %s\r\n", pName);
	if( !(mSrc = imread(pName)).data )
		return;
	printf("OpenCV2 %s\r\n", pName);
	imshow("111", mSrc);
	waitKey(0);
}

int main( int argc, char** argv )
{
    Mat mSrc, mDst;

	char* pName = "others/woman1.jpg";
	if( !(mSrc = imread(pName)).data )
		return -1;

	//OBJ_main("woman4.jpg");
	//LaplaceDemo(mSrc, mDst);
	//RemovePoint(mSrc);
	//LSV_NoiseFiltering_Test(mSrc, mDst);
	test(mSrc, mDst);
	//imshow("Dst", mDst);

	vector<uchar> buf;
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);
	imwrite("test.jpg", mDst, compression_params);

	waitKey(0);
	return 0;
}