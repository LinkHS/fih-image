#ifndef __CV_FACE_DETECT_H
#define __CV_FACE_DETECT_H

#include "opencv.hpp"

#ifndef __cplusplus
  #ifndef bool
    #define bool unsigned char
  #endif
  #ifndef true
    #define true 1
  #endif
  #ifndef false
    #define false 0
  #endif
#endif

//Face Dectect Parameters
typedef struct 
{
    double scaleFactor;       //1.1
    int minNeighbors;         //3
    //cv::CASCADE_DO_CANNY_PRUNING; CASCADE_SCALE_IMAGE; CASCADE_FIND_BIGGEST_OBJECT; CASCADE_DO_ROUGH_SEARCH
    int flags;           
    int minSizeW;
    int minSizeH;
    //int maxSizeW;
    //int maxSizeH;
		
		bool overlay;
}FIH_FaceDectParms;

void cvFaceDectect(cv::Mat& mSrc, std::vector<cv::Rect> &faces, bool overlay = true);
void cvFaceDectect(cv::Mat& mSrc, std::vector<cv::Rect> &faces, FIH_FaceDectParms& faceDectParms);

void cvFaceFeatureDectect(cv::Mat& mSrc, std::vector<cv::Rect> &faces);


#endif

