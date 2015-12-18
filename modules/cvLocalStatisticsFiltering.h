#ifndef __CV_LOCAL_STATISTICS_FILTERING_H
#define __CV_LOCAL_STATISTICS_FILTERING_H

#include "opencv.hpp"

void cvLocalStatisticsFiltering_YChannel(cv::Mat& mSrc, cv::Mat& mDst, int radius, int level);
void cvLocalStatisticsFiltering_YChannel_Rough(cv::Mat& mSrc, cv::Mat& mDst, int radius, int level);

#endif
