#include "includes.h"
#include "cvSkinSmooth.h"

using namespace cv; 

void cvSkinSmooth_YChannel(Mat& mSrc, Mat& mDst, cvInputType type, int radius, int level)
{
	Mat mYab;
	Mat channels[3];
	Mat &mY = channels[0];

    switch( type )
    {
      case CV_INPUT_Y:
	  case CV_INPUT_GRAY:
	    mY = mSrc;
	    break;
      case CV_INPUT_BGR:
        cvtColor(mSrc, mYab, CV_BGR2YCrCb);
	  case CV_INPUT_YUV:
	    split(mYab, channels);
	    break;
    }

	cvLocalStatisticsFiltering_YChannel(mY, mY, radius, level);

	cvMergeAndConvert(channels, mDst);
}

void cvSkinSmooth_YChannel(Mat& mSrc, Mat& mDst, std::vector<Rect> &faces, cvInputType type)
{
	Mat mYab;
	Mat channels[3];
	Mat &mY = channels[0];

    switch( type )
    {
      case CV_INPUT_Y:
	  case CV_INPUT_GRAY:
	  
	    break;
      case CV_INPUT_BGR:
        cvtColor(mSrc, mYab, CV_BGR2YCrCb);
	  case CV_INPUT_YUV:
	    split(mYab, channels);
	    break;
    }

	cvLocalStatisticsFiltering_YChannel(mY, mDst, 10, 10);
}