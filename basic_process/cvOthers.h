#ifndef __CV_OTHERS_H
#define __CV_OTHERS_H

#include "opencv.hpp"

#define cvStartTimer(t)       do{ t = (double)getTickCount(); } while(0)
/* Times passed in seconds */
#define cvStopTimer(t)        do{ t = ((double)getTickCount() - t)/getTickFrequency(); } while(0)
#define cvGetFunCostTime(fun, funName)                           \
        do{                                                      \
            double t = (double)getTickCount();                   \
            fun;                                                 \
            t = ((double)getTickCount() - t)/getTickFrequency(); \
            MY_LOGD("%s() cost %lfs", funName, t);               \
        } while(0)

void cvCorrectGamma( cv::Mat& mSrc, cv::Mat& mDst, double gamma );
void cvConvertAndSplit(cv::Mat& mSrc, cv::Mat* pChannels, int code=CV_BGR2YCrCb);
void cvMergeAndConvert(cv::Mat* pChannels, cv::Mat& mDst, int code=CV_YCrCb2BGR);
void cvSaturation(cv::Mat& mSrc, cv::Mat &mDst, int increment);

void cvDisplayMat(cv::Mat& mSrc, int rect_size = 1024, const char* pWindowName = NULL);
void cvResizeDown(cv::Mat& mSrc, cv::Mat& mDst, int rect_size = 1024);
void cvResizeUp(cv::Mat& mSrc, cv::Mat& mDst, int rect_size = 1024);

void ColorCvt_BGR2YUYV(cv::Mat& mBGR, unsigned char *pYUYV);
void ColorCvt_YUYV2BGR(unsigned char *pYUYV, cv::Mat& mBGR, int width, int height);

void cvCalcHist( cv::Mat &mSrc, cv::Mat &mHist );

#endif
