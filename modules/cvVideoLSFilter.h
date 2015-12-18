#ifndef __CV_VIDEO_LOCAL_STATISTIC_FILTERING_H
#define __CV_VIDEO_LOCAL_STATISTIC_FILTERING_H

class cvVideoLSFilter
{
public: ////                    Instantiation.
        virtual                 ~cvVideoLSFilter();
                                cvVideoLSFilter(void);

public:
        u8* 				    pData;

private:		
        uint                    _frameH;            //Frame Height
        uint                    _frameW;            //Frame Width


        u32*                    pCumulSum;         //Accumulated sum
        u32*                    pCumulSqrSum;      //Accumulated square sum
        u32*                    pLSum;             //Local sum
        u32*                    pLSqrSum;          //Local square sum

        u8                      _smoothRadius;     //Smooth radium
        u32                     _roiX;             //Start x position of ROI(region of interest)
        u32                     _roiY;             //Start y position of ROI(region of interest)
        u32                     _roiW;             //width of Region of interest
        u32                     _roiH;             //height of Region of interest 

        cv::Mat                 _mCumulSum;         //Cumulative Sum, 2 Channels - Sum and Square sum
        cv::Mat                 _mLocalSum;         //Local Sum, 2 Channels - Sum and Square sum
public:
        void                    init(u8 *pSrc, uint h, uint w);
        void                    init1(uint h, uint w, u8 cpuNums);
		void                    init2(uint h, uint w, u8 cpuNums);
        void                    uninit(void);
        void                    uninit1(void);

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
