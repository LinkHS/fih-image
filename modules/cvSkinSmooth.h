#ifndef __CV_SKIN_SMOOTH_H
#define __CV_SKIN_SMOOTH_H

#include "opencv.hpp"

void cvSkinSmooth_YChannel(cv::Mat& mSrc, cv::Mat& mDst, cvInputType type, int radius, int level);
void cvSkinSmooth_YChannel(cv::Mat& mSrc, cv::Mat& mDst, std::vector<cv::Rect> &faces, cvInputType type);

#endif

