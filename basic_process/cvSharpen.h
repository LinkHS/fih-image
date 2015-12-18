#ifndef __CV_SHARPEN_H__
#define __CV_SHARPEN_H__

#include "opencv.hpp"
#include "hopePixelRgn.h"

typedef struct
{
  int  sharpen_percent;
  HopePixelRgn* roi;          /* Region of interest */
} SharpenParams;

void cvSharpen (cv::Mat& mSrc, cv::Mat& mDst, SharpenParams* params=NULL);

#endif

