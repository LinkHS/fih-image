#define LOG_TAG "FIH/MTK_FDManager"

#include "porting/MTK_FDManager.h"
#include "debug.h"

#define DEBUG_LEVEL                 0

#define MY_LOGD(fmt, arg...)        FIH_LOGD("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        FIH_LOGI("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        FIH_LOGW("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        FIH_LOGE("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGT(fmt, arg...) 

#define MY_LOGD_IF(level, ...)      do { if (level >= DEBUG_LEVEL) { MY_LOGD(__VA_ARGS__); } }while(0)


MTK_FDManager::
MTK_FDManager(void)
{
    MY_LOGD_IF(1, "+");
    _faceNum = 0;
    _readlock = 0;
}

MTK_FDManager::
~MTK_FDManager()
{
    MY_LOGD_IF(1, "-");
}

/*
    Note: Currently Only support 3 faces
*/
void
MTK_FDManager::
UpdateFDResult(int* faceRects, int faceNum)
{
    if ( (faceNum > _maxNum) || (faceNum < 0) ){
        MY_LOGE("Error, unspported face number\n");
    }
    else if ( faceNum == 0 ) {
        _faceNum = 0;
        return;
    }

    //if ( _faceNum != 0 ) {
    //    MY_LOGE("Dawei: Error, new FD Result is dropped\n");
    //    return;
    //}

    memcpy(_faceRects, faceRects, faceNum * sizeof(int) * 4);
    _faceNum = faceNum;
    MY_LOGD_IF(0, "_faceNum = %d", _faceNum);
}

int
MTK_FDManager::
GetFDResult(int* faceRects)
{
    //lock buffer
    //android_atomic_inc(&_readlock);

    int faceNum = _faceNum;
    if ( _faceNum > 0 ) {
        memcpy(faceRects, _faceRects, faceNum * sizeof(int) * 4);
    }

    //clear _faceNum to make _faceRects buffer avaliable
    //_faceNum = 0;

    //release buffer
    //android_atomic_dec(&_readlock);

    return faceNum;
}

int
MTK_FDManager::
GetFirstFaceRect(int faceRect[4])
{
    int faceNum = _faceNum;

    if (_faceNum > 0) {
        for (int i=0; i<4; i++) {
            faceRect[i] = _faceRects[0][i];
        }
    }

    //clear _faceNum to make _faceRects buffer avaliable
    //_faceNum = 0;

    return faceNum;
}

/* Calibrate Face Coordinate, from absolute coordinate to relative */
void
MTK_FDManager::
CalibFaceCoord(int *pFaceRects, int num, int h, int w) 
{
    int length = _MaxAbsCoord - _MinAbsCoord;
    int *pFace = pFaceRects;

    for (int i=0; i<num; i++) 
    {
        pFace[0] = (pFace[0] - _MinAbsCoord)*w/length;
        pFace[1] = (pFace[1] - _MinAbsCoord)*h/length;
        pFace[2] = (pFace[2] - _MinAbsCoord)*w/length;
        pFace[3] = (pFace[3] - _MinAbsCoord)*h/length;

        pFace += 4;
    }
}