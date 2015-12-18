#define LOG_TAG "FIH/FIHImgProc"
#define DEBUG_LEVEL                 0

#include "includes.h"
#include "opencv.hpp"
#include <opencv2/core/ocl.hpp>

#include "bspApiForPlatform.h"
#include "basic_process/cvOthers.h"
#include "basic_process/cvSharpen.h"
#include "modules/cvFaceDetect.h"
#include "modules/cvLocalStatisticsFiltering.h"
#include "modules/VideoLSFilter.h"
#include "porting/debug.h"
#include "porting/MTK_FDManager.h"

#include "dehazing/dehazing.h"

#include <sys/times.h>
#include <cutils/atomic.h>

#include "FihFaceInfo.h"

using namespace cv;

extern void WYMCAdjustment_RedPixels(Mat& mSrc, Mat& mDst, const int WYMC[4]);
extern void WYMCAdjustment_BlackPixels(Mat& mSrc, Mat& mDst, struct WYMCParams* pWYMCParams);

#define MY_LOGD(fmt, arg...)        FIH_LOGD("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        FIH_LOGI("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        FIH_LOGW("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        FIH_LOGE("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGT(fmt, arg...)        FIH_LOGD("Temp: [%s] "fmt, __FUNCTION__, ##arg) //temp log

#define MY_LOGD_IF(cond, ...)       do { if (cond) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGD_LEVEL(level, ...)   do { if (level >= DEBUG_LEVEL) { MY_LOGD(__VA_ARGS__); } }while(0)

int g_level = 0;
int g_enable = 0;

MTK_FDManager *g_FDManager = NULL;

void CaptureProcess_FaceBeauty(uchar* pYUYV, int width, int height)
{
    int faceRect[4];
    int smoothSize=0, fw, fh;
    int smoothLevel = g_level;

    if ( g_FDManager != NULL )
    {
        if( g_FDManager->GetFirstFaceRect((int *)faceRect) )
        {
            g_FDManager->CalibFaceCoord( faceRect, 1, height, width);

            /* Calculate the width and height */
            fw = faceRect[2] - faceRect[0];  /* w = right - left */
            fh = faceRect[3] - faceRect[1];  /* h = bottom - top */

            smoothSize = std::max(fw,fh)/55;
        }
    }

    if ( smoothSize == 0)
    {
        smoothSize = std::max(width, height)/80;
    }


    int fb_gamma = 1, fb_sharpen = 50;

    /* White, Yellow, Magenta, Cyan */
    const int WYMC1[4] = {30, 0, 0, -40};
    const int WYMC2[4] = {30, 0, 0, -40};
    const int WYMC3[4] = {50, 0, 0, -50};
    const int WYMC4[4] = {50, 0, 0, -70};
    int const *fb_pWYMC_Red = WYMC1;

    /* [0, 10] to [0,4] */
    switch (smoothLevel/2) {
        case 0:
            return;
        case 1:
            fb_gamma = 1.1;
            fb_pWYMC_Red = WYMC1;
            fb_sharpen = 50;
            smoothLevel += 2;
            break;
        case 2:
            fb_gamma = 1.1;
            fb_pWYMC_Red = WYMC2;
            fb_sharpen = 60;
            break;
        case 3:
            fb_gamma = 1.1;
            fb_pWYMC_Red = WYMC3;
            fb_sharpen = 60;
            break;
        case 4:
            fb_gamma = 1.2;
            fb_pWYMC_Red = WYMC3;
            fb_sharpen = 70;
            break;
        case 5:
            fb_gamma = 1.2;
            fb_pWYMC_Red = WYMC4;
            fb_sharpen = 70;
            break;
    }

    /* 1. YUY2 convert to BGR(Mat)*/
    Mat mBGR;
    Mat mSrcYUYV( height, width, CV_8UC2, pYUYV );
    cvtColor( mSrcYUYV, mBGR, CV_YUV2BGR_YUY2 );

    /*
     * 2. Image Porcessing for Color Processing 
     * 2.1 Color Whitening 
     */
    Mat mDstBGR;
    WYMCAdjustment_RedPixels(mBGR, mDstBGR, fb_pWYMC_Red);   mBGR = mDstBGR;
    //if (smoothLevel > 6) {
    //    WYMCAdjustment_BlackPixels(mBGR, mDstBGR, NULL); mBGR = mDstBGR;
    //}
    //mBGR.convertTo(mDstBGR, -1, 1.2, 0); 

    /* 2.2 Get Y channel data */
    Mat mYCrCb, mChannels[3];
    cvtColor( mBGR, mYCrCb, CV_BGR2YCrCb );
    split( mYCrCb, mChannels );
    Mat& mY = mChannels[0];

    Mat mDstY;
    cvCorrectGamma( mY, mDstY, fb_gamma ); mY = mDstY;
    //mDstY.convertTo(mDstY, -1, 1.2, 0);

    cvLocalStatisticsFiltering_YChannel(mY, mDstY, smoothSize, smoothLevel);
    mY = mDstY;

    SharpenParams parms = {fb_sharpen, NULL};
    cvSharpen(mY, mDstY, &parms);
    mY = mDstY;

    merge(mChannels, 3, mYCrCb);
    cvtColor( mYCrCb, mBGR , CV_YCrCb2BGR );

    /*
     * 3. BGR convert to YUY2 and copy back
     */
    ColorCvt_BGR2YUYV(mBGR, pYUYV);
}

void CaptureProcess_Dehazing(uchar* pYUYV, int width, int height)
{
    Mat mBGR;
    Mat mSrcYUYV( height>>1, width>>1, CV_8UC2, pYUYV );
    cvtColor( mSrcYUYV, mBGR, CV_YUV2BGR_YUY2 );

    //MY_LOGD("mBGR.cols %d, mBGR.rows %d", mBGR.cols, mBGR.rows);
    dehazing dehazingImg(mBGR.cols, mBGR.rows, 16, false, false, 5.0f, 1.0f, 40);
    
    Mat mResult;
	dehazingImg.ImageHazeRemoval(mBGR, mResult);

    ColorCvt_BGR2YUYV(mResult, pYUYV);
}

void CaptureProcess_Denoising1(uchar* pYUYV, int width, int height)
{
   const int levels[] = {1,1,3,3,3,5,5,5,7,7,7};
    int level = levels[g_level];

    Mat mDst, mChannels[2];
    Mat mSrcYUYV( height, width, CV_8UC2, pYUYV );

    split( mSrcYUYV, mChannels );
    Mat& mY = mChannels[0];

  #if 1
    //bilateralFilter(mY, mDst, level, level<<1, level>>1);
    //fastNlMeansDenoising(mY, mDst, level);

#pragma omp parallel for    
	for(int i=0; i<8; i++) //row
	{
        Mat mSubY(mY, Rect(0, (height >> 4)*i, width, (height >> 4)));
        Mat mSubDY;
        fastNlMeansDenoising(mSubY, mSubDY, level);
        mSubDY.copyTo(mSubY);
    }
    mDst = mY;

  #else
    UMat umSrc;
    mY.copyTo(umSrc);
    UMat umDst;

    ocl::setUseOpenCL(true);
    if(  ocl::haveOpenCL() )
      MY_LOGD("OCL");
    else
      MY_LOGD("OCL Failed");

    //bilateralFilter(umSrc, umDst, level, level<<1, level>>1);
    fastNlMeansDenoising(umSrc, umDst, level, 7, 15);
    umDst.copyTo(mDst);
  #endif

    int channels = mDst.channels();
    int nRows = mDst.rows;
    int nCols = mDst.cols * channels;
    if( mDst.isContinuous() )
    {
        nCols *= nRows;
        nRows = 1;
    }

    for( int i = 0; i < nRows; ++i)
    {
        uchar* pY = mDst.ptr<uchar>(i);
        for( int j = 0; j < nCols; j++)
        {
            *pYUYV++ = *pY++;
            *pYUYV++;
        }
    }
}

void CaptureProcess_Denoising2(uchar* pYUYV, int width, int height)
{
    const int levels[] = {1,1,3,3,3,5,5,5,7,7,7};
    int level = levels[g_level];

    Mat mDst, mChannels[2];
    Mat mSrcYUYV( height, width, CV_8UC2, pYUYV );

    split( mSrcYUYV, mChannels );
    Mat& mY = mChannels[0];

    //bilateralFilter(mY, mDst, level, level<<1, level>>1);
  #if 0
    fastNlMeansDenoising(mY, mDst, level);
  #else
    UMat umSrc;
    mY.copyTo(umSrc);
    UMat umDst;

    ocl::setUseOpenCL(true);
    if(  ocl::haveOpenCL() )
      MY_LOGD("OCL");
    else
      MY_LOGD("OCL Failed");

    fastNlMeansDenoising(umSrc, umDst, level);

    //mDst = umDst;
    umDst.copyTo(mDst);
  #endif

    int channels = mDst.channels();
    int nRows = mDst.rows;
    int nCols = mDst.cols * channels;
    if( mDst.isContinuous() )
    {
        nCols *= nRows;
        nRows = 1;
    }

    for( int i = 0; i < nRows; ++i)
    {        
        uchar* pY = mDst.ptr<uchar>(i);
        for( int j = 0; j < nCols; j++)
        {
            *pYUYV++ = *pY++;
            *pYUYV++;
        }
    }
}

void CaptureProcess_LocalHistEqual_noSaturation(uchar* pYUYV, int width, int height)
{
#define PRINT_COST_TIME    1
#define USE_OPENCL         1

#if PRINT_COST_TIME
    double t;
    cvStartTimer(t);
#endif

    Mat mDst, mChannels[2];
    Mat mSrcYUYV( height, width, CV_8UC2, pYUYV );

    split( mSrcYUYV, mChannels );
    Mat& mY = mChannels[0];

#if USE_OPENCL
    ocl::setUseOpenCL(true);
    if(  ocl::haveOpenCL() )
      MY_LOGD("OCL");
    else
      MY_LOGD("OCL Failed");

    UMat umY;
    mY.copyTo(umY);
#endif

	Ptr<CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(1);
	clahe->apply(umY,mDst);

    /*
     * Convert to BGR
     */
    int channels = mDst.channels();
    int nRows = mDst.rows;
    int nCols = mDst.cols * channels;
    if( mDst.isContinuous() )
    {
        nCols *= nRows;
        nRows = 1;
        uchar* pY = mDst.ptr<uchar>(0);
  #pragma omp parallel for 
        for( int j = 0; j < nCols; j++)
        {
            *pYUYV++ = *pY++;
            *pYUYV++;
        }
    }
    else{
  #pragma omp parallel for 
        for( int i = 0; i < nRows; ++i)
        {
            uchar* pY = mDst.ptr<uchar>(i);
            for( int j = 0; j < nCols; j++)
            {
                *pYUYV++ = *pY++;
                *pYUYV++;
            }
        }
    }
  
#if PRINT_COST_TIME  
    cvStopTimer(t);
    MY_LOGT("Local Hist Equal, %d x %d, cost %lfs",height, width, t);
#endif

#undef PRINT_COST_TIME
#undef USE_OPENCL
}

void CaptureProcess_LocalHistEqual(uchar* pYUYV, int width, int height)
{
#define PRINT_COST_TIME    1
#define USE_OPENCL         0

#if PRINT_COST_TIME  
    double t;
    cvStartTimer(t);
#endif

    /*
     * Get Y channel
     */
    Mat mDst, mChannels[2];
    Mat mSrcYUYV( height, width, CV_8UC2, pYUYV );

    split( mSrcYUYV, mChannels );
    Mat& mY = mChannels[0];

    /*
     * Local Histogram Equalization in Y Channel
     */
	Ptr<CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(1);
	clahe->apply(mY,mDst);

    /*
     * Convert to BGR
     */
    Mat mBGR;
    mChannels[0] = mDst;
    merge(mChannels, 2, mSrcYUYV);
    cvtColor( mSrcYUYV, mBGR, CV_YUV2BGR_YUY2 );

    /*
     * Increase Saturation
     */
    //const int levels[] = {20,20,30,30,40,40,50,50,60,60,70};
    //int level = levels[g_level];
    int level = 30;

    cvSaturation(mBGR, mDst, level);
    ColorCvt_BGR2YUYV(mBGR, pYUYV);


#if PRINT_COST_TIME  
    cvStopTimer(t);
    MY_LOGD("Local Hist Equal, %d x %d, cost %lfs",height, width, t);
#endif

#undef PRINT_COST_TIME
#undef USE_OPENCL
}


void FIH_FB_GetParams_Callback(void* pFBParams)
{
    /* convert [-12, 12] tp [0 - 12] */
    g_level = (*(int*)pFBParams + 12) >> 1;
    g_level = (g_level > 10 ) ? 10 : g_level;

    MY_LOGD_LEVEL(1, "FB Level = %d -> %d", *(int *)pFBParams, g_level);
}

// FIH Face Beauty in preview, get Detected Faces information from MTK
void FIH_FBPreview_GetFDInfo(void* pFDInfo)
{
#define MAX_FACE_NUM  3
    MY_LOGD_LEVEL(0, "pFDInfo = %x", pFDInfo);

    int fd_rect[MAX_FACE_NUM][4];

    if( pFDInfo == NULL ){
        g_FDManager->UpdateFDResult(NULL, 0);
        return;
    }

    CameraFaceMetadata* pMtkDetectedFaces = (CameraFaceMetadata*)pFDInfo;
    CameraFace* pFaces = pMtkDetectedFaces->faces;
    
    int number_of_faces = pMtkDetectedFaces->number_of_faces;
    if (number_of_faces > MAX_FACE_NUM) 
        number_of_faces = MAX_FACE_NUM;
    MY_LOGD_LEVEL(0, "number_of_faces = %d", number_of_faces);

    for (int i=0; i<number_of_faces; i++)
    {
        fd_rect[i][0] = pFaces->rect[0];
        fd_rect[i][1] = pFaces->rect[1];
        fd_rect[i][2] = pFaces->rect[2];
        fd_rect[i][3] = pFaces->rect[3];

        pFaces++;
    }

    g_FDManager->UpdateFDResult((int *)fd_rect, number_of_faces);
}

VideoLSFilter g_videoLSFilter;

void FIH_PreviewProcess_Init(int height, int width, int mode)
{
    MY_LOGI("+ h=%d, w=%d, m=%d", height, width, mode);

    g_FDManager = new MTK_FDManager;
    
    g_videoLSFilter.init1(height, width, 8);
}

void FIH_PreviewProcess_Release(void)
{
    MY_LOGI("+");
    delete g_FDManager;
    g_FDManager = NULL;

    g_videoLSFilter.uninit1();
}

void FIH_PreviewProcess_Handler(FIH_Frame* pFrameInfo, FIH_ProcessParams* pParams)
{
    MY_LOGI("+");

    int faceRects[3][4];
    int frameH, frameW;

    //if ( (g_FDManager == NULL) || (g_videoLSFilter == NULL) ) {
    if ( (g_FDManager == NULL) ) {
        MY_LOGE("Dawei: Error, g_FDManager(%d) or g_videoLSFilter is not initialized", g_FDManager);
        return;
    }

    g_videoLSFilter.pData = pYUV;
    
    int faceNum = g_FDManager->GetFDResult((int *)faceRects);
    if ( faceNum > 0 ) 
    {
        g_videoLSFilter.GetFrameSize(&frameH, &frameW);
        g_FDManager->CalibFaceCoord((int *)faceRects, faceNum, frameH, frameW);
        if( g_videoLSFilter.CalCumulDist_Rows3((int *)faceRects, faceNum) )
        {
            int smoothLevel = (*(int *)pUserData + 12) >> 1; //[-12, 12] to [0, 12]
            g_videoLSFilter.FastLSV_NoiseFiltering_Rough4(smoothLevel);
        }
    }

    MY_LOGD_LEVEL(0, "faceNum=%d, smoothLevel=%d", faceNum, *(int *)pUserData);
}

void FIH_CaptureProcess_Handler(FIH_Frame* pFrameInfo, FIH_ProcessParams* pParams)
{
    MY_LOGI("+, p1 %x, p2 %x", pFrameInfo, pParams);

    switch( pParams->mode )
    {
    case 1: //SHDR
        CaptureProcess_LocalHistEqual(pFrameInfo->pAddr, pFrameInfo->width, pFrameInfo->height);
        break;
    case 2:
        CaptureProcess_FaceBeauty(pFrameInfo->pAddr, pFrameInfo->width, pFrameInfo->height);
        break;
    default:
        MY_LOGE("Unspported FIH_CaptureProcess Mode %d", pParams->mode);
        break;
    }
}

void FIH_PreviewProcess_Handler(FIH_Frame &frameInfo, FIH_ProcessParams &params)
{
    MY_LOGI("+");
}

void FIH_CaptureProcess_Handler(FIH_Frame &frameInfo, FIH_ProcessParams &params)
{
    MY_LOGI("+");

    switch( params.mode )
    {
    case 1: //SHDR
        CaptureProcess_LocalHistEqual(frameInfo.pAddr, frameInfo.width, frameInfo.height);
        break;
    case 2:
        CaptureProcess_FaceBeauty(frameInfo.pAddr, frameInfo.width, frameInfo.height);
        break;
    default:
        MY_LOGE("Unspported FIH_CaptureProcess Mode %d", params.mode);
        break;
    }
}


