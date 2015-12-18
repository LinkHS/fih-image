#ifndef __VIDEO_LOCAL_STATISTIC_FILTERING_H
#define __VIDEO_LOCAL_STATISTIC_FILTERING_H

#include "TypeDefines.h"

/******************************************************************************
*  The information of a face from camera face detection.
******************************************************************************/
struct FIHCameraFace{
		/**
		* Bounds of the face [left, top, right, bottom]. (-1000, -1000) represents
		* the top-left of the camera field of view, and (1000, 1000) represents the
		* bottom-right of the field of view. The width and height cannot be 0 or
		* negative. This is supported by both hardware and software face detection.
		*
		* The direction is relative to the sensor orientation, that is, what the
		* sensor sees. The direction is not affected by the rotation or mirroring
		* of CAMERA_CMD_SET_DISPLAY_ORIENTATION.
		*/
		int32 rect[4];

		/**
		* The confidence level of the face. The range is 1 to 100. 100 is the
		* highest confidence. This is supported by both hardware and software
		* face detection.
		*/
		int32 score;

		/**
		* An unique id per face while the face is visible to the tracker. If
		* the face leaves the field-of-view and comes back, it will get a new
		* id. If the value is 0, id is not supported.
		*/
		int32 id;

		/**
		* The coordinates of the center of the left eye. The range is -1000 to
		* 1000. -2000, -2000 if this is not supported.
		*/
		int32 left_eye[2];

		/**
		* The coordinates of the center of the right eye. The range is -1000 to
		* 1000. -2000, -2000 if this is not supported.
		*/
		int32 right_eye[2];

		/**
		* The coordinates of the center of the mouth. The range is -1000 to 1000.
		* -2000, -2000 if this is not supported.
		*/
		int32 mouth[2];

};


/******************************************************************************
 *   FD Pose Information: ROP & RIP
 *****************************************************************************/
struct FIHFaceInfo {

    int32 rop_dir;
    int32 rip_dir;
};


/******************************************************************************
 *  The metadata of the frame data.
 *****************************************************************************/
struct FIHCameraFaceMetadata {
    /**
     * The number of detected faces in the frame.
     */
    int32 number_of_faces;

    /**
     * An array of the detected faces. The length is number_of_faces.
     */
    FIHCameraFace *faces;
    FIHFaceInfo   *posInfo;
};


class VideoLSFilter
{
public: ////                    Instantiation.
        virtual                 ~VideoLSFilter();
                                VideoLSFilter(void);

public:
        u8* 			pData;

private:
        uint                    frameH;            //Frame Height
        uint                    frameW;            //Frame Width		
        u32*                    pCumulSum;         //Accumulated sum
        u32*                    pCumulSqrSum;      //Accumulated square sum
        u32*                    pLSum;             //Local sum
        u32*                    pLSqrSum;          //Local square sum

        u8                      _smoothRadius;     //Smooth radium
        u32                     _roiX;             //Start x position of ROI(region of interest)
        u32                     _roiY;             //Start y position of ROI(region of interest)
        u32                     _roiW;             //width of Region of interest
        u32                     _roiH;             //height of Region of interest 

public:
        void                    init(u8 *pSrc, uint h, uint w);
        void                    init1(uint h, uint w, u8 cpuNums);
        void                    init2(uint h, uint w, u8 cpuNums);
        void                    uninit(void);
        void                    uninit1(void);

        void                    GetFrameSize(int* h, int* w);

        void                    CalCumulDist_Rows(u8 radius);
        void                    CalCumulDist_Rows1(u8 radius);
        int                     CalCumulDist_Rows2(int* face);
        int                     CalCumulDist_Rows3(int* faces, int faceNum);
        void                    FastLSV_NoiseFiltering_Rough(u8 radius, u8 level);
        void                    FastLSV_NoiseFiltering_Rough1(u8 radius, u8 level);
        void                    FastLSV_NoiseFiltering_Rough2(u8 radius, u8 level);
        void                    FastLSV_NoiseFiltering_Rough3(u8 radius, u8 level);
        void                    FastLSV_NoiseFiltering_Rough4(u8 level);
};

#endif
