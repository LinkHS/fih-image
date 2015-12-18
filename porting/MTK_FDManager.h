#ifndef __MTK_FDMANAGER_H
#define __MTK_FDMANAGER_H

class MTK_FDManager
{
public: ////                    Instantiation.
        virtual                 ~MTK_FDManager();
                                MTK_FDManager(void);
public:
        bool 				    isEnable(void);
        void                    Enable(void);
        void                    Disable(void);
        void                    UpdateFDResult(int* faceRects, int faceNum);
        int                     GetFDResult(int* faceRects);
        int                     GetFirstFaceRect(int faceRect[4]);
        void                    CalibFaceCoord(int *pFaceRects, int num, int h, int w);
public:

private:
       /**
        * Bounds of the face [left, top, right, bottom]. (-1000, -1000) represents
        * the top-left of the camera field of view, and (1000, 1000) represents the
        * bottom-right of the field of view. The width and height cannot be 0 or
        * negative. This is supported by both hardware and software face detection.
        *
        * The direction is relative to the sensor orientation, that is, what the
        * sensor sees. The direction is not affected by the rotation or mirroring
        * of CAMERA_CMD_SET_DISPLAY_ORIENTATION.
        * */ 
        static const int        _MinAbsCoord = -1000; //minimum absolute coordinate
        static const int        _MaxAbsCoord = 1000;  //maximum absolute coordinate

        static const int        _maxNum = 3;
        int                     _faceRects[_maxNum][4];
        int                     _faceNum;
        unsigned char           _status;
        int                     _readlock;
};

#endif

