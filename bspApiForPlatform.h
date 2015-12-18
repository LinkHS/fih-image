#ifndef __BSP_API_FOR_PLATFORM_H__
#define __BSP_API_FOR_PLATFORM_H__

#define  FIH_FACEBEAUTY_ENABLED    1

typedef struct
{
    unsigned char * pAddr;
    int             frame_type;
    int             width;
    int             height;
}FIH_Frame;

typedef struct
{
    int             mode;
    int             level1;
    int             level2;
    int             level3;
}FIH_ProcessParams;


void YUYVSkinSmooth(unsigned char* pYUV, int width, int height);
void YUVProcess_Preview(unsigned char* pYUV, int width, int height);

void VideoLSFilter_Init(unsigned int height, unsigned int width);
void VideoLSFilter_Uninit(void);
void VideoLSFilter_Run(unsigned char* pYUV, void *pUserData);

void FIH_FB_GetParams_Callback(void* pFBParams);
void FIH_FBPreview_GetFDInfo_Callback(void* pFDInfo);

void FIH_PreviewProcess_Init(int height, int width, int mode);
void FIH_PreviewProcess_Release(void);
void FIH_PreviewProcess_Handler(FIH_Frame *, FIH_ProcessParams *);
void FIH_CaptureProcess_Handler(FIH_Frame *, FIH_ProcessParams *);
void FIH_PreviewProcess_Handler(FIH_Frame &frameInfo, FIH_ProcessParams &params);
void FIH_CaptureProcess_Handler(FIH_Frame &frameInfo, FIH_ProcessParams &params);
#endif