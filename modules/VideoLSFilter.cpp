#define LOG_TAG "FIH/VideoLSFilter"
#define DEBUG_LEVEL                 0

#include "VideoLSFilter.h"
#include "common/PreCalculatedTables.h"
#include "porting/debug.h"

#include <algorithm>    // std::max
#include <omp.h>
#include <arm_neon.h>

#define MY_LOGD(fmt, arg...)        FIH_LOGD("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGI(fmt, arg...)        FIH_LOGI("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGW(fmt, arg...)        FIH_LOGW("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGE(fmt, arg...)        FIH_LOGE("[%s] "fmt, __FUNCTION__, ##arg)
#define MY_LOGT(fmt, arg...)        FIH_LOGD("Temp: [%s] "fmt, __FUNCTION__, ##arg) //temp log

#define MY_LOGD_IF(cond, ...)       do { if (cond) { MY_LOGD(__VA_ARGS__); } }while(0)
#define MY_LOGD_LEVEL(level, ...)   do { if (level >= DEBUG_LEVEL) { MY_LOGD(__VA_ARGS__); } }while(0)

#define USE_NEON

__inline__ void VecAdd_u8x16(u8* pSrc1, u8* pSrc2, u8* pDst, u32 len)
{
    u32 offset;

#ifdef USE_NEON
    uint8x16_t v_src1, v_src2, v_res;
    
    for ( offset=0; offset<len; offset+=16 )
    {
        v_src1 = vld1q_u8( pSrc1+offset );  //Load a single vector from memory1
        v_src2 = vld1q_u8( pSrc2+offset );  //Load a single vector from memory2

        v_res = vaddq_u8( v_src1, v_src2 );
        
        vst1q_u8( pDst+offset, v_res );
    }

    if ( offset == len )
        return;

    for ( offset -= 16; offset<len; offset++) 
    {
        *(pDst+offset) = *(pSrc1+offset) + *(pSrc2+offset);
    }
#else
    for ( offset=0; offset<len; offset++) 
    {
        *(pDst++) = *(pSrc1++) + *(pSrc2++);
    }
#endif
}

__inline__ void VecAdd_u32x4(u32* pSrc1, u32* pSrc2, u32* pDst, u32 len)
{
    u32 offset;

#ifdef USE_NEON
    uint32x4_t v_src1, v_src2, v_res;
    
    for ( offset=0; offset<len; offset+=4 )
    {
        v_src1 = vld1q_u32( pSrc1+offset );  //Load a single vector from memory1
        v_src2 = vld1q_u32( pSrc2+offset );  //Load a single vector from memory2

        v_res = vaddq_u32( v_src1, v_src2 );

        vst1q_u32( pDst+offset, v_res );
    }

    if ( offset == len )
        return;

    for ( offset -= 4; offset<len; offset++) 
    {
        *(pDst+offset) = *(pSrc1+offset) + *(pSrc2+offset);
    }
#else
    for ( offset=0; offset<len; offset++) 
    {
        *(pDst++) = *(pSrc1++) + *(pSrc2++);
    }
#endif
}


VideoLSFilter::
VideoLSFilter(void)
{
    MY_LOGD("VideoLSFilter()");
    pCumulSum    = NULL;      //Accumulated sum
    pCumulSqrSum = NULL;      //Accumulated square sum
    pLSum        = NULL;      //Local sum
    pLSqrSum     = NULL;      //Local square sum
}

VideoLSFilter::
~VideoLSFilter()
{
    MY_LOGD("dawei: ~VideoLSFilter()");
    if( pCumulSum != NULL )    delete[] pCumulSum;
    if( pCumulSqrSum != NULL ) delete[] pCumulSqrSum;
    if( pLSum != NULL )        delete[] pCumulSum;
    if( pLSqrSum != NULL )     delete[] pCumulSqrSum;
}

void
VideoLSFilter::
init(u8 *pSrc, uint h, uint w)
{
    /* 
     * 1. Assign the addrees of source data and its size
     */
    frameW = w;
    frameH = h;
    pData = pSrc;

    /* 
     * 2. Get the the proper radius
     */
    u8 radius = 20;  //maximum radius is 50
    
	u32 sumSizeW = frameW + (radius << 1) + 1;
	u32 sumSizeH = frameH + (radius << 1); 
	u32 sumSize = sumSizeW * sumSizeH; 

    /* 
     * 3. Malloc memory
     */	
    /* Avoid user calling init() twice without uninit correctly */
    //if( pCumulSum )    delete[] pCumulSum;
    //if( pCumulSqrSum ) delete[] pCumulSqrSum;
    //if( pLSum )        delete[] pCumulSum;
    //if( pLSqrSum )     delete[] pCumulSqrSum;
    
    pCumulSum    = new u32 [sumSize]; 
    pCumulSqrSum = new u32 [sumSize]; 
    pLSum        = new u32 [frameW<<1];
    pLSqrSum     = new u32 [frameW<<1];
}

void
VideoLSFilter::
init1(uint h, uint w, u8 cpuNums)
{
    MY_LOGD("dawei: VideoLSFilter::init1()");
    /* 
     * 1. Assign the its size
     */
    frameW = w;
    frameH = h;

    /* 
     * 2. Get the the proper radius
     */
    if ( (frameW <= 600) && (frameH <= 600) ) {
        _smoothRadius = 12;
    }else if ( (frameW <= 1080) && (frameH <= 1080) ) {
        _smoothRadius = 20;
    }else if ( (frameW <= 1920) && (frameH <= 1920) ) {
        _smoothRadius = 30;
    }else{
        _smoothRadius = 40;
        MY_LOGW("Dawei: frame size is invalid, w=%d, h=%d\n", frameW, frameH);
    }

    MY_LOGW("Dawei: frame size is, w=%d, h=%d\n", frameW, frameH);

    u8 radius = _smoothRadius;  //maximum radius is 60
    
	u32 sumSizeW = frameW + (radius << 1) + 1;
	u32 sumSizeH = frameH + (radius << 1); 
	u32 sumSize = sumSizeW * sumSizeH; 

    /* 
     * 3. Malloc memory
     */	
    /* Avoid user calling init() twice without uninit correctly */
    if( pCumulSum )    delete[] pCumulSum;
    if( pCumulSqrSum ) delete[] pCumulSqrSum;
    if( pLSum )        delete[] pCumulSum;
    if( pLSqrSum )     delete[] pCumulSqrSum;

    /* double the buffer size, one for Cumulative sum, another for Cumulative square sum */
    /* The memory stores as below:
       Line1: Sum1, Square Sum1; Sum2, Square Sum2; Sum3, Square Sum3; Sum4, Square Sum4; ...
       Line2: Sum1, Square Sum1; Sum2, Square Sum2; Sum3, Square Sum3; Sum4, Square Sum4; ... 
       Line3: Sum1, Square Sum1; Sum2, Square Sum2; Sum3, Square Sum3; Sum4, Square Sum4; ...
    */
    pCumulSum    = new u32 [sumSize<<1];
    pLSum        = new u32 [((frameW<<1)*cpuNums)<<1];

    pCumulSqrSum = new u32 [sumSize]; 
    pLSqrSum     = new u32 [frameW<<1];
}

void
VideoLSFilter::
uninit(void)
{
    pData = NULL;
    delete[] pCumulSum;
    delete[] pCumulSqrSum;
    delete[] pLSum;
    delete[] pLSqrSum;
}

void
VideoLSFilter::
uninit1(void)
{
    MY_LOGD("dawei: VideoLSFilter::uninit1()");
    pData = NULL;
    delete[] pCumulSum;
    delete[] pLSum;
    
    delete[] pCumulSqrSum;
    delete[] pLSqrSum;

    pCumulSum = NULL;
    pLSum     = NULL;
    pCumulSqrSum = NULL;
    pLSqrSum  = NULL;
}

void
VideoLSFilter::          
GetFrameSize(int* h, int* w)
{
    *h = frameH;
    *w = frameW;
}

/*
-------------------------------
----1----------2----------3----
-------------------------------
-------------------------------
-------------------------------
----4----------5----------6----
-------------------------------
-------------------------------
-------------------------------
----7----------8----------9----
-------------------------------
*/
//Local Statistics Variance Noise Filtering
void
VideoLSFilter::
CalCumulDist_Rows(u8 radius) //One Channel
{	 
    u32 sumW = frameW + (radius << 1) + 1;
    u32 sumH = frameH + (radius << 1);

    //u8* pSrcRow  = pData;
    //u32* pSum    = pCumulSum    + sumW*radius;
    //u32* pSqrSum = pCumulSqrSum + sumW*radius;
        
#pragma omp parallel for    
	for(u32 j=radius; j<sumH-radius; j++) //row
	{  
	    u8* pSrcRow  = pData + frameW*(j-radius);
        u32* pSum    = pCumulSum    + sumW*j;
        u32* pSqrSum = pCumulSqrSum + sumW*j;
    
		// We add one zero colunmn before first column to help calculate local sums which include pixels in first column.
		// i.e., to get the sum of column 1 to column 7, we use cumulative sums (col1 to col7) minus 0.
		// This is because we need to put in a buffer of zero values along x-axis and y-axis in order to make 
		// computation efficient.
		pSum[0]    = 0;
		pSqrSum[0] = 0;

		// Calculate cumulative sums starts from column_1 to column_radius
		for(u32 i=1; i<=radius; i++)
		{
			u8 current_pixel = pSrcRow[radius - i + 1];
			pSum[i] = pSum[i-1] + current_pixel;
			pSqrSum[i] = pSqrSum[i-1] + table_u8Squares[current_pixel];		
		}

	    // Calculate cumulative sums starts from column_(radius+1) to column_(radius+1) last
		for(u32 i=(radius+1); i<=(sumW-radius-1); i++)
		{
			u8 current_pixel = pSrcRow[i - radius - 1];
			pSum[i] = pSum[i-1] + current_pixel;
			pSqrSum[i] = pSqrSum[i-1] + table_u8Squares[current_pixel];
		}

		// Calculate cumulative sums starts from column_radius to end
		for(u32 i=(sumW-radius); i<sumW; i++)
		{
			u8 current_pixel = pSrcRow[frameW - (radius-(sumW-i)) - 1];
			pSum[i] = pSum[i-1] + current_pixel;
			pSqrSum[i] = pSqrSum[i-1] + table_u8Squares[current_pixel];
		}

		//pSrcRow += frameW;
        //pSum    += sumW;
        //pSqrSum += sumW;
	}

	u32 r2 = radius * 2;
	u32 rl = sumH-radius;
//#pragma omp parallel for
	for(u32 j=0; j<radius; j++) //row
	{
		//mCumulSum.row(r2-j).copyTo(mCumulSum.row(j));
		//mCumulSum.row(rl-j-2).copyTo(mCumulSum.row(rl+j));
		//mCumulSquareSum.row(r2-j).copyTo(mCumulSquareSum.row(j)); 
		//mCumulSquareSum.row(rl-j-2).copyTo(mCumulSquareSum.row(rl+j));

		memcpy(&pCumulSum[j*sumW],         &pCumulSum[(r2-j)*sumW],      sumW<<2);
		memcpy(&pCumulSum[(rl+j)*sumW],    &pCumulSum[(rl-j-2)*sumW],    sumW<<2);
        memcpy(&pCumulSqrSum[j*sumW],      &pCumulSqrSum[(r2-j)*sumW],   sumW<<2);
		memcpy(&pCumulSqrSum[(rl+j)*sumW], &pCumulSqrSum[(rl-j-2)*sumW], sumW<<2);
	}
}

/*
Input Parameters:
    [radius] equals 0 means use _smoothRadius;
*/
void
VideoLSFilter::
CalCumulDist_Rows1(u8 radius) //One Channel
{	
    if ( (radius == 0) || (radius > _smoothRadius) )  {
        radius = _smoothRadius;
    }
    u32 sumW = frameW + (radius << 1) + 1;
    u32 sumH = frameH + (radius << 1);
       
#pragma omp parallel for    
	for(u32 j=radius; j<sumH-radius; j++) //row
	{  
	    u8* pSrcRow  = pData + frameW*(j-radius);
        u32* pCSum   = pCumulSum + (sumW<<1)*j;
		// We add one zero colunmn before first column to help calculate local sums which include pixels in first column.
		// i.e., to get the sum of column 1 to column 7, we use cumulative sums (col1 to col7) minus 0.
		// This is because we need to put in a buffer of zero values along x-axis and y-axis in order to make 
		// computation efficient.
		pCSum[0] = 0;
		pCSum[1] = 0;

		// Calculate cumulative sums starts from column_1 to column_radius;
		for(u32 i=1; i<=radius; i++)
		{
		    u32 isum = i << 1;
			u8 current_pixel = pSrcRow[radius - i + 1];
			pCSum[isum]   = pCSum[isum-2] + current_pixel;                  //Cumulative Sum
			pCSum[isum+1] = pCSum[isum-1] + table_u8Squares[current_pixel]; //Cumulative Square Sum
		}

	    // Calculate cumulative sums starts from column_(radius+1) to column_(radius+1) last
		for(u32 i=(radius+1); i<=(sumW-radius-1); i++)
		{
		    u32 isum = i << 1;
			u8 current_pixel = pSrcRow[i - radius - 1];
			pCSum[isum]   = pCSum[isum-2] + current_pixel;                  //Cumulative Sum
			pCSum[isum+1] = pCSum[isum-1] + table_u8Squares[current_pixel];	//Cumulative Square Sum	
		}

		// Calculate cumulative sums starts from column_radius to end
		for(u32 i=(sumW-radius); i<sumW; i++)
		{
		    u32 isum = i << 1;
			u8 current_pixel = pSrcRow[frameW - (radius-(sumW-i)) - 1];
			pCSum[isum]   = pCSum[isum-2] + current_pixel;                  //Cumulative Sum
			pCSum[isum+1] = pCSum[isum-1] + table_u8Squares[current_pixel]; //Cumulative Square Sum
		}
	}

	u32 r2 = radius * 2;
	u32 rl = sumH-radius;
//#pragma omp parallel for
	for(u32 j=0; j<radius; j++) //row
	{
		memcpy(&pCumulSum[j*sumW<<1],      &pCumulSum[(r2-j)*sumW<<1],   sumW<<3);
		memcpy(&pCumulSum[(rl+j)*sumW<<1], &pCumulSum[(rl-j-2)*sumW<<1], sumW<<3);
	}
}

/*
Input Parameters:
    [radius] equals 0 means use _smoothRadius;
*/
int
VideoLSFilter::
CalCumulDist_Rows2(int* pFace) //One Channel
{	
//MY_LOGD("Dawei: +");
    u8 radius = 20;
    int left, top, right, bottom;
    if( pFace == NULL )
    {
        MY_LOGD("Dawei: pFace == NULL");
        return 0;
    }else
    {
        left   = pFace[0];
        top    = pFace[1];
        right  = pFace[2];
        bottom = pFace[3];        
    }


MY_LOGD("Dawei: 1 left %d, top %d, right %d, bottom %d", left, top, right, bottom);
    int w,h;
    w = right - left;
    h = bottom - top;
    left = left - w/2;
    left = (left < (radius+1)) ? (radius+1) : left;
    right = right + w/2;
    right = (right > (int)(frameW - (radius+1))) ? (frameW - (radius+1)) : right;
    top = top - (h/8);
    top = (top < (radius+1)) ? (radius+1) : top;
    bottom = bottom + (h>>3);
    bottom = (bottom > (int)(frameH - (radius+1))) ? (frameH - (radius+1)) : bottom;

#if 1
    /* 
        Check if the extended face is crossed.
    */
    int a = frameW - right;
    int b = frameH - bottom;
    int min = (left < top) ? left : top;
    min = (min < a) ? min : a;
    min = (min < b) ? min : b;

    if( (radius+1) > min )
    {
MY_LOGD("Dawei: (radius+1) > min");
        return 0;
    }
    else
        _smoothRadius = radius;
#endif

    _roiX = left;
    _roiY = top;
    _roiW = right - left;
    _roiH = bottom - top;
    u32 sumW = _roiW + (radius << 1) + 1;
    u32 sumH = _roiH + (radius << 1);

//MY_LOGD("Dawei: _roiX %d,_roiY %d,_roiW %d,_roiH %d", _roiX, _roiY, _roiW, _roiH);
//MY_LOGD("Dawei: 2 left %d, top %d, right %d, bottom %d", left, top, right, bottom);
    u8* pSrcRow  = pData + frameW*_roiY + _roiX - radius;
    u32* pCSum   = pCumulSum;
//#pragma omp parallel for

	for(u32 j=_roiY-radius; j<_roiY+_roiH+radius; j++) //row
	{  
	    u8* pdata = pSrcRow;
	    
		// We add one zero colunmn before first column to help calculate local sums which include pixels in first column.
		// i.e., to get the sum of column 1 to column 7, we use cumulative sums (col1 to col7) minus 0.
		// This is because we need to put in a buffer of zero values along x-axis and y-axis in order to make 
		// computation efficient.
		pCSum[0] = 0;
		pCSum[1] = 0;

		// Calculate cumulative sums starts from column_1 to column_radius;
		for(u32 i=1; i<=_roiW+(radius<<1); i++)
		{
		    u32 isum = i << 1;
			u8 current_pixel = *pdata++;
			pCSum[isum]   = pCSum[isum-2] + current_pixel;                  //Cumulative Sum
			pCSum[isum+1] = pCSum[isum-1] + table_u8Squares[current_pixel]; //Cumulative Square Sum
		}

	    pSrcRow += frameW;
	    pCSum   += sumW<<1;
	}

	return 1;
}

int
VideoLSFilter::
CalCumulDist_Rows3(int* pFaces, int faceNum) //One Channel
{	
    int left_w=0, top_w=0, right_w=0, bottom_w=0;
    int w, h;
    int *pFace = pFaces;

    if( pFace == NULL ){
        MY_LOGE("pFace == NULL");
        return 0;
    }

    /* 
     * 1. Extend boundry of each face, then combine each boundry together
     */
    for ( int i=0; i<faceNum; i++ )
    {
        int left=pFace[0], top=pFace[1], right=pFace[2], bottom=pFace[3];
        //MY_LOGT("1 left %d, top %d, right %d, bottom %d", left, top, right, bottom);

        /* Calculate the width and height */
        w = right - left; /* w = right - left */
        h = pFace[3] - pFace[1]; /* h = bottom - top */

        /* Extend the boundry, left and right are increased by 1/2 percent,
           top and bottom are increased by 1/8 percent */
        left = left - w/2;
        right = right + w/2;
        top = top - (h>>3);
        bottom = bottom + (h>>3);

        /* Calculate combined boundry */
        if ( i == 0){
            left_w = left; top_w = top; right_w = right; bottom_w = bottom;
        }else{
            left_w   = (left_w < left) ? left_w : left;
            top_w    = (top_w < top) ? top_w : top;
            right_w  = (right_w >= right) ? right_w : right;
            bottom_w = (bottom_w >= bottom) ? bottom_w : bottom;
        }

        pFace += 4; //each rect has 4 points
        //MY_LOGT("2 left %d, top %d, right %d, bottom %d", left, top, right, bottom);
    }

    /*
     * 2. Calculate best smooth radius
     */
    w = right_w - left_w;
    h = bottom_w - top_w;
    _smoothRadius = std::max(w,h)/(55*faceNum); 

    MY_LOGD_IF((_smoothRadius <= 0), "left %d, top %d, right %d, bottom %d, w %d, h %d", left_w, top_w, right_w, bottom_w, w, h);

    left_w = (left_w < (_smoothRadius+1)) ? (_smoothRadius+1) : left_w;
    right_w = (right_w > (int)(frameW - (_smoothRadius+1))) ? (frameW - (_smoothRadius+1)) : right_w;
    top_w = (top_w < (_smoothRadius+1)) ? (_smoothRadius+1) : top_w;
    bottom_w = (bottom_w > (int)(frameH - (_smoothRadius+1))) ? (frameH - (_smoothRadius+1)) : bottom_w;

    /*
     * 3. Set the ROI(region of interest)
     */
    _roiX = left_w;
    _roiY = top_w;
    _roiW = right_w - left_w;
    _roiH = bottom_w - top_w;
    u32 sumW = _roiW + (_smoothRadius << 1) + 1;
    u32 sumH = _roiH + (_smoothRadius << 1);

    u8* pSrcRow  = pData + frameW*_roiY + _roiX - _smoothRadius;
    u32* pCSum   = pCumulSum;

//#pragma omp parallel for
	for(u32 j=_roiY-_smoothRadius; j<_roiY+_roiH+_smoothRadius; j++) //row
	{  
	    u8* pdata = pSrcRow;
	    
		// We add one zero colunmn before first column to help us calculate local sums which include first column.
		// i.e., to get the sum of column 1 to column 7, we use cumulative sums (col1 to col7) minus 0.
		// This is because we need to put in a buffer of zero values along x-axis and y-axis in order to make 
		// computation efficient.
		pCSum[0] = 0;
		pCSum[1] = 0;

		// Calculate cumulative sums starts from column_1 to column_radius;
		for(u32 i=1; i<=_roiW+(_smoothRadius<<1); i++)
		{
		    u32 isum = i << 1;
			u8 current_pixel = *pdata++;
			pCSum[isum]   = pCSum[isum-2] + current_pixel;                  //Cumulative Sum
			pCSum[isum+1] = pCSum[isum-1] + table_u8Squares[current_pixel]; //Cumulative Square Sum
		}

	    pSrcRow += frameW;
	    pCSum   += sumW<<1;
	}

	return 1;
}

/*
-------------------------------
----1----------2----------3----
-------------------------------
-------------------------------
-------------------------------
----4----------5----------6----
-------------------------------
-------------------------------
-------------------------------
----7----------8----------9----
-------------------------------
*/
//Local Statistics Variance Noise Filtering
void
VideoLSFilter::
FastLSV_NoiseFiltering_Rough(u8 radius, u8 level)
{
	u32 cols = frameW, rows = frameH;

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) )
	{
	    MY_LOGW("Parameters are invaild in FastLSV_NoiseFiltering_Rough()\n");
		return;
	}

	level = level * level * 5 + 10; //filtering level

    u32 sumW = frameW + (radius << 1) + 1;
    u32 sumH = frameH + (radius << 1);

	u32 rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;
	
    u8* pData_row = pData;

	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
    u32* pLSum_row    = pLSum;            //local sum
    u32* pLSqrSum_row = pLSqrSum;         //local square sum
	memset(pLSum, 0, frameW<<2);
	memset(pLSqrSum, 0, frameW<<2);
#pragma omp parallel for
	for(u32 j=0; j<r2+1; j++) //row
	{
		//pSum = pCumulSum[j]; pSqrSum = mCumulSquareSum.ptr<int>(j);
		u32* pSum    = pCumulSum    + sumW*j; //global sum
        u32* pSqrSum = pCumulSqrSum + sumW*j; //global square sum

		for(u32 i=0; i<cols; i++) //col
		{
			pLSum_row[i]    += (pSum[i+r2+1]    - pSum[i]);			
			pLSqrSum_row[i] += (pSqrSum[i+r2+1] - pSqrSum[i]);
		}
	}
#pragma omp parallel for
	for(u32 i=0; i<cols; i++) //col
	{
		u32 templMean = pLSum_row[i] / rr4;
		u32 templSdv = (pLSqrSum_row[i] - (templMean * pLSum_row[i])) / rr4;
		pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
	}

	/* 2. Calculate the rest rows from second row */
	//u8 previous = 0, current = 1;
//#pragma omp parallel for
	for(u32 j=1; j<rows; j++) //row
	{
        pData_row = pData + frameW*j;
        
		u32* pSum1    = pCumulSum    + sumW*(j-1);    //the line which should be removed
		u32* pSqrSum1 = pCumulSqrSum + sumW*(j-1);    //the line which should be removed
		u32* pSum2    = pCumulSum    + sumW*(j+r2);   //the line which should be added = pSum1 + 2*radius + 1
		u32* pSqrSum2 = pCumulSqrSum + sumW*(j+r2);   //the line which should be added = pSum1 + 2*radius + 1
		
		u32* pLSumP    = pLSum    + frameW*((j+1)&0x01);
		u32* pLSqrSumP = pLSqrSum + frameW*((j+1)&0x01);

		pLSum_row    = pLSum    + frameW*((j)&0x01);
		pLSqrSum_row = pLSqrSum + frameW*((j)&0x01);
		
#pragma omp parallel for
		for(u32 i=0; i<cols; i++) //col
		{
			pLSum_row[i]    = pLSumP[i]    - (pSum1[i+r2+1]    - pSum1[i])    + (pSum2[i+r2+1]    - pSum2[i]);
			pLSqrSum_row[i] = pLSqrSumP[i] - (pSqrSum1[i+r2+1] - pSqrSum1[i]) + (pSqrSum2[i+r2+1] - pSqrSum2[i]);

			u32 templMean = pLSum_row[i] / rr4;
			u32 templSdv = (pLSqrSum_row[i] - (templMean * pLSum_row[i])) / rr4;

			//double kb = templSdv / (templSdv + level);
			pData_row[i] = (uchar)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
		}

		//exchange buffer position
		//current = previous;
		//previous = (previous == 0) ? 1 : 0;
	}
}

void
VideoLSFilter::
FastLSV_NoiseFiltering_Rough1(u8 radius, u8 level)
{
	u32 cols = frameW, rows = frameH;

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) )
	{
	    MY_LOGW("Parameters are invaild in FastLSV_NoiseFiltering_Rough()\n");
		return;
	}

	level = level * level * 5 + 10; //filtering level

    u32 sumW = frameW + (radius << 1) + 1;
    u32 sumH = frameH + (radius << 1);

	u32 rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;
	
    u8* pData_row = pData;

	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
	memset(pLSum, 0, frameW<<2);
	memset(pLSqrSum, 0, frameW<<2);
	
    u32* pLSum_row    = pLSum;            //local sum
    u32* pLSqrSum_row = pLSqrSum;         //local square sum
	//u32* pSum    = pCumulSum;
	//u32* pSqrSum = pCumulSqrSum;
	
#pragma omp parallel for
	for(u32 j=0; j<r2+1; j++) //row
	{
		u32* pSum    = pCumulSum + sumW*j;
	    u32* pSqrSum = pCumulSqrSum + sumW*j;
	    
		for(u32 i=0; i<cols; i++) //col
		{
			pLSum_row[i]    += (pSum[i+r2+1]    - pSum[i]);			
			pLSqrSum_row[i] += (pSqrSum[i+r2+1] - pSqrSum[i]);
		}
		
		//pSum    += sumW; //global sum
        //pSqrSum += sumW; //global square sum
	}
#pragma omp parallel for
	for(u32 i=0; i<cols; i++) //col
	{
		u32 templMean = pLSum_row[i] / rr4;
		u32 templSdv = (pLSqrSum_row[i] - (templMean * pLSum_row[i])) / rr4;
		pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
	}

	/* 2. Calculate the rest rows from second row */

	u32 *pLSumP, *pLSqrSumP;
	u32 *pSum1    = pCumulSum    + sumW;           //the line which should be removed
    u32 *pSqrSum1 = pCumulSqrSum + sumW;           //the line which should be removed
    u32 *pSum2    = pCumulSum    + sumW*(1+r2);    //the line which should be added = pSum1 + 2*radius + 1
    u32 *pSqrSum2 = pCumulSqrSum + sumW*(1+r2);
	pData_row = pData + frameW;
//#pragma omp parallel for
	for(u32 j=1; j<rows; j++) //row
	{
		if( j&0x01 )
		{
            pLSumP       = pLSum;
            pLSqrSumP    = pLSqrSum;
            pLSum_row    = pLSum    + frameW;
		    pLSqrSum_row = pLSqrSum + frameW;
		}else
		{
            pLSumP       = pLSum    + frameW;
            pLSqrSumP    = pLSqrSum + frameW;
            pLSum_row    = pLSum;
		    pLSqrSum_row = pLSqrSum;
		}

		
#pragma omp parallel for
		for(u32 i=0; i<cols; i++) //col
		{
			pLSum_row[i]    = pLSumP[i]    - (pSum1[i+r2+1]    - pSum1[i])    + (pSum2[i+r2+1]    - pSum2[i]);
			pLSqrSum_row[i] = pLSqrSumP[i] - (pSqrSum1[i+r2+1] - pSqrSum1[i]) + (pSqrSum2[i+r2+1] - pSqrSum2[i]);

			u32 templMean = pLSum_row[i] / rr4;
			u32 templSdv = (pLSqrSum_row[i] - (templMean * pLSum_row[i])) / rr4;

			//double kb = templSdv / (templSdv + level);
			pData_row[i] = (uchar)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
		}

		pData_row += frameW;
        
		pSum1    += sumW;    //the line which should be removed
		pSqrSum1 += sumW;    //the line which should be removed
		pSum2    += sumW;    //the line which should be added = pSum1 + 2*radius + 1
		pSqrSum2 += sumW;    //the line which should be added = pSum1 + 2*radius + 1
	}
}

void
VideoLSFilter::
FastLSV_NoiseFiltering_Rough2(u8 radius, u8 level)
{
	u32 cols = frameW, rows = frameH;

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) )
	{
	    MY_LOGW("Parameters are invaild in FastLSV_NoiseFiltering_Rough()\n");
		return;
	}

	level = level * level * 5 + 10; //filtering level

    u32 sumW = frameW + (radius << 1) + 1;
    u32 sumH = frameH + (radius << 1);

	u32 rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;
	
    u32 blkH = frameH >> 3;
    u32 blkSize = frameH * frameW >> 3;
    u32 blkSumSize = sumW * sumH >> 3;
    
#pragma omp parallel for   
    for(u8 index=0; index<8; index++)
    {
        u8* pData_row = pData + blkSize*index;
        //u32 rows = frameH >> 3;
        u32* pLSum    = new u32 [frameW<<1];             //Local sum
        u32* pLSqrSum = new u32 [frameW<<1];          //Local square sum

    	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
    	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
    	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
    	memset(pLSum, 0, frameW<<2);
    	memset(pLSqrSum, 0, frameW<<2);
    	
        u32* pLSum_row    = pLSum;            //local sum
        u32* pLSqrSum_row = pLSqrSum;         //local square sum
    	//u32* pSum    = pCumulSum;
    	//u32* pSqrSum = pCumulSqrSum;
    	
    	for(u32 j=0; j<r2+1; j++) //row
    	{
    		u32* pSum    = pCumulSum    + sumW*j + index*blkSumSize;
    	    u32* pSqrSum = pCumulSqrSum + sumW*j + index*blkSumSize;
    	    
    		for(u32 i=0; i<cols; i++) //col
    		{
    			pLSum_row[i]    += (pSum[i+r2+1]    - pSum[i]);			
    			pLSqrSum_row[i] += (pSqrSum[i+r2+1] - pSqrSum[i]);
    		}
    		
    		//pSum    += sumW; //global sum
            //pSqrSum += sumW; //global square sum
    	}

    	for(u32 i=0; i<cols; i++) //col
    	{
    		u32 templMean = pLSum_row[i] / rr4;
    		u32 templSdv = (pLSqrSum_row[i] - (templMean * pLSum_row[i])) / rr4;
    		pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
    	}

    	/* 2. Calculate the rest rows from second row */
    	for(u32 j=1; j<blkH; j++) //row
    	{
    	    u32 *pLSumP, *pLSqrSumP;
    	    u32 *pLSum_row, *pLSqrSum_row;
    	    
    		u32 *pSum1    = pCumulSum    + sumW*j + index*blkSumSize;           //the line which should be removed
            u32 *pSqrSum1 = pCumulSqrSum + sumW*j + index*blkSumSize;           //the line which should be removed
            u32 *pSum2    = pCumulSum    + sumW*(j+r2) + index*blkSumSize;    //the line which should be added = pSum1 + 2*radius + 1
            u32 *pSqrSum2 = pCumulSqrSum + sumW*(j+r2) + index*blkSumSize;
    	    
            u8* pData_row = pData + frameW*j + index*blkSize;
            
    		if( j&0x01 )
    		{
                pLSumP       = pLSum;
                pLSqrSumP    = pLSqrSum;
                pLSum_row    = pLSum    + frameW;
    		    pLSqrSum_row = pLSqrSum + frameW;
    		}else
    		{
                pLSumP       = pLSum    + frameW;
                pLSqrSumP    = pLSqrSum + frameW;
                pLSum_row    = pLSum;
    		    pLSqrSum_row = pLSqrSum;
    		}

    		for(u32 i=0; i<cols; i++) //col
    		{
    			pLSum_row[i]    = pLSumP[i]    - (pSum1[i+r2+1]    - pSum1[i])    + (pSum2[i+r2+1]    - pSum2[i]);
    			pLSqrSum_row[i] = pLSqrSumP[i] - (pSqrSum1[i+r2+1] - pSqrSum1[i]) + (pSqrSum2[i+r2+1] - pSqrSum2[i]);

    			u32 templMean = pLSum_row[i] / rr4;
    			u32 templSdv = (pLSqrSum_row[i] - (templMean * pLSum_row[i])) / rr4;

    			//double kb = templSdv / (templSdv + level);
    			pData_row[i] = (uchar)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
    		}
    	}

    	delete[] pLSum;
        delete[] pLSqrSum;
    }
}

//culumative values and square culumative values are stored in one memory one by one 
void
VideoLSFilter::
FastLSV_NoiseFiltering_Rough3(u8 radius, u8 level)
{
    if ( (radius == 0) || (radius > _smoothRadius) )  {
        radius = _smoothRadius;
    }
	u32 cols = frameW, rows = frameH;

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) )
	{
	    MY_LOGE("Dawei: Parameters are invaild in FastLSV_NoiseFiltering_Rough()\n");
		return;
	}

	level = level * level * 5 + 10; //filtering level

    u32 sumW = frameW + (radius << 1) + 1; //1321
    u32 sumH = frameH + (radius << 1);     //760

	u32 rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;
	
    u32 blkH = frameH >> 3;              //divide 8(cpu nums)
    u32 blkSize = (frameH * frameW) >> 3;  //divide 8(cpu nums)
    //u32 blkSumSize = (sumW * sumH) >> 3;   //divide 8(cpu nums)  //125495
    u32 blkSumSize = (sumW * frameH) >> 3;   //divide 8(cpu nums)  //125495

   memset(pLSum, 0 , ((frameW<<1)<<3)<<1<<2);
   
   #if 0
  //MY_LOGD("Dawei: blkH,%d; blkSize,%d; blkSumSize,%d; pCumulSum,%x",blkH,blkSize,blkSumSize, pCumulSum);
#pragma omp parallel for   
    for(u8 index=0; index<8; index++)
    {
        u8* pData_row = pData + blkSize*index;
        u32* pLSum_row = pLSum + (u32)((frameW<<2)*index);   //local sum
      
    	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
    	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
    	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
    	u32* pCSum = pCumulSum + (u32)(index*blkSumSize<<1);
  
    	for(u32 j=0; j<r2+1; j++) //row
    	{
    		for(u32 i=0; i<cols; i++) //col
    		{
    		    u32 isum  = (i+r2+1) << 1;
    		    u32 ilsum = i << 1;
    			pLSum_row[ilsum]   += (pCSum[isum]   - pCSum[(i<<1)]);			
    			pLSum_row[ilsum+1] += (pCSum[isum+1] - pCSum[(i<<1)+1]);
    		}

    		pCSum += (sumW << 1);
    	}

    	for(u32 i=0; i<cols; i++) //col
    	{
    	    u32 ilsum = i << 1;
    		u32 templMean = pLSum_row[ilsum] / rr4;
    		u32 templSdv = (pLSum_row[ilsum+1] - (templMean * pLSum_row[ilsum])) / rr4;
    		pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
            //pData_row[i] = 0;
    	}
    	

    	/* 2. Calculate the rest rows from second row */
    	pData_row = pData + frameW + index*blkSize;
        u32 *pSum1 = pCumulSum + (sumW<<1)        + (u32)((index*blkSumSize)<<1);    //the line which should be removed
        u32 *pSum2 = pCumulSum + (sumW<<1)*(r2+1) + (u32)((index*blkSumSize)<<1);    //the line which should be added = pSum1 + 2*radius + 1
    	for(u32 j=1; j<blkH; j++) //row
    	{
    	    u32 *pLSumP, *pLSum_row;
            
    		if( j&0x01 )
    		{
                pLSumP    = pLSum + (u32)((frameW<<2)*index);
                pLSum_row = pLSumP + (u32)(frameW<<1);
    		}else
    		{
    		    pLSum_row = pLSum + (u32)((frameW<<2)*index);
                pLSumP    = pLSum_row + (u32)(frameW<<1);
    		}

    		for(u32 i=0; i<cols; i++) //col
    		{
    		    u32 isum  = (i+r2+1) << 1;
    		    u32 ilsum = i << 1;
    		    //u32 isum  = ilsum + r2 + 1;

    			pLSum_row[ilsum]   = pLSumP[ilsum]   - (pSum1[isum]   - pSum1[ilsum])   + (pSum2[isum]   - pSum2[ilsum]);
    			pLSum_row[ilsum+1] = pLSumP[ilsum+1] - (pSum1[isum+1] - pSum1[ilsum+1]) + (pSum2[isum+1] - pSum2[ilsum+1]);
    			
    			//u32 templMean = pLSum_row[ilsum] / rr4;
    			//u32 templSdv = (pLSum_row[ilsum+1] - (templMean * pLSum_row[ilsum])) / rr4;

    			//double kb = templSdv / (templSdv + level);
    			//pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));

                //u32 templMean = pLSum_row[ilsum];
    			//u32 templSdv = pLSum_row[ilsum+1]*rr4 - (templMean * pLSum_row[ilsum]);
    			//pData_row[i] = (u8)(level*templMean/(templSdv+level*rr4) + templSdv*pData_row[i]/(templSdv+level*rr4));
                //pData_row[i] = 0;
    		}

    		pData_row += frameW;
    	    
    		pSum1 += sumW<<1;  //the line which should be removed
            pSum2 += sumW<<1;  //the line which should be added
    	}

    }

    #endif
}

//culumative values and square culumative values are stored in one memory one by one 
void
VideoLSFilter::
FastLSV_NoiseFiltering_Rough4(u8 level)
{
    u8 radius = _smoothRadius;

	u32 cols = _roiW, rows = _roiH;

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) )
	{
	    MY_LOGE("Parameters are invaild, radius=%d, level=%d, cols=%d, rows=%d\n", radius, level, cols, rows);
		return;
	}
    MY_LOGD_LEVEL(0, "radius=%d, level=%d, cols=%d, rows=%d\n", radius, level, cols, rows);

	level = level * level * 5 + 10; //filtering level

    u32 sumW = _roiW + (radius << 1) + 1;
    u32 sumH = _roiH + (radius << 1);


	u32 rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;
	
    u32 blkH = _roiH >> 3;              //divide 8(cpu nums)
    u32 blkSize = frameW * (_roiH >> 3);  //divide 8(cpu nums)
    //u32 blkSumSize = (sumW * sumH) >> 3;   //divide 8(cpu nums)  //125495
    u32 blkSumSize = sumW * (_roiH >> 3);   //divide 8(cpu nums)  //125495

   memset(pLSum, 0 , ((_roiW<<1)<<3)<<1<<2);
   
   #if 1
  //MY_LOGD("Dawei: blkH,%d; blkSize,%d; blkSumSize,%d; pCumulSum,%x",blkH,blkSize,blkSumSize, pCumulSum);
#pragma omp parallel for   
    for(u8 index=0; index<8; index++)
    {
        u8* pData_row = pData + frameW*(_roiY + blkH*index) + _roiX;        
        u32* pLSum_row = pLSum + (u32)((_roiW<<2)*index);   //local sum
      
    	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
    	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
    	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
    	u32* pCSum = pCumulSum + (u32)(index*blkSumSize<<1);
  
    	for(u32 j=0; j<r2+1; j++) //row
    	{
    		for(u32 i=0; i<cols; i++) //col
    		{
    		    u32 isum  = (i+r2+1) << 1;
    		    u32 ilsum = i << 1;
    			pLSum_row[ilsum]   += (pCSum[isum]   - pCSum[(i<<1)]);			
    			pLSum_row[ilsum+1] += (pCSum[isum+1] - pCSum[(i<<1)+1]);
    		}

    		pCSum += (sumW << 1);
    	}

    	for(u32 i=0; i<cols; i++) //col
    	{
    	    u32 ilsum = i << 1;
    		u32 templMean = pLSum_row[ilsum] / rr4;
    		u32 templSdv = (pLSum_row[ilsum+1] - (templMean * pLSum_row[ilsum])) / rr4;
    		pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
            //pData_row[i] = 0;
    	}
    	

    	/* 2. Calculate the rest rows from second row */
    	pData_row = pData + frameW*(_roiY + blkH*index + 1) + _roiX;
        u32 *pSum1 = pCumulSum + (sumW<<1)        + (u32)((index*blkSumSize)<<1);    //the line which should be removed
        u32 *pSum2 = pCumulSum + (sumW<<1)*(r2+1) + (u32)((index*blkSumSize)<<1);    //the line which should be added = pSum1 + 2*radius + 1
    	for(u32 j=1; j<blkH; j++) //row
    	{
    	    u32 *pLSumP, *pLSum_row;
            
    		if( j&0x01 )
    		{
                pLSumP    = pLSum + (u32)((_roiW<<2)*index);
                pLSum_row = pLSumP + (u32)(_roiW<<1);
    		}else
    		{
    		    pLSum_row = pLSum + (u32)((_roiW<<2)*index);
                pLSumP    = pLSum_row + (u32)(_roiW<<1);
    		}

    		for(u32 i=0; i<cols; i++) //col
    		{
    		    u32 isum  = (i+r2+1) << 1;
    		    u32 ilsum = i << 1;
    		    //u32 isum  = ilsum + r2 + 1;

    			pLSum_row[ilsum]   = pLSumP[ilsum]   - (pSum1[isum]   - pSum1[ilsum])   + (pSum2[isum]   - pSum2[ilsum]);
    			pLSum_row[ilsum+1] = pLSumP[ilsum+1] - (pSum1[isum+1] - pSum1[ilsum+1]) + (pSum2[isum+1] - pSum2[ilsum+1]);
    			
    			u32 templMean = pLSum_row[ilsum] / rr4;
    			u32 templSdv = (pLSum_row[ilsum+1] - (templMean * pLSum_row[ilsum])) / rr4;

    			//double kb = templSdv / (templSdv + level);
    			pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
                //pData_row[i] = 0;
                //u32 templMean = pLSum_row[ilsum];
    			//u32 templSdv = pLSum_row[ilsum+1]*rr4 - (templMean * pLSum_row[ilsum]);
    			//pData_row[i] = (u8)(level*templMean/(templSdv+level*rr4) + templSdv*pData_row[i]/(templSdv+level*rr4));
    		}

    		pData_row += frameW;
    	    
    		pSum1 += sumW<<1;  //the line which should be removed
            pSum2 += sumW<<1;  //the line which should be added
    	}

    }

    #endif
}

#if 0
static void calcul_result(u8 radius, u8 level)
{
    u32 offset;
    u32 cols = frameW, rows = frameH;
    uint32x4_t v_sum1, v_sum1_end, v_sum2, v_sum2_end, v_LSum, v_LSqrSum, v_LSumP, v_LSqrSumP, v_res;

    for ( offset=0; offset<cols; offset+=4 )
    {
        v_LSumP    = vld1q_u32( &pLSumP[ilsum] );
        v_LSqrSumP = vld1q_u32( &pLSqrSumP[ilsum] );

        //pLSum_row[ilsum]   = pLSumP[ilsum]   - (pSum1[isum]   - pSum1[ilsum])   + (pSum2[isum]   - pSum2[ilsum]); 
        v_sum1     = vld1q_u32( pSrc1+offset );      //Load a single vector from memory1
        v_sum1_end = 
        v_sum2     = vld1q_u32( pSrc1+offset );
        v_sum2_end = ;

        v_LSum = vsubq_u32( v_LSumP, vaddq_u32(vsubq_u32(v_sum1_end, v_sum1), vsubq_u32(v_sum2_end, v_sum2)) );

        //pLSum_row[ilsum+1] = pLSumP[ilsum+1] - (pSum1[isum+1] - pSum1[ilsum+1]) + (pSum2[isum+1] - pSum2[ilsum+1]);
        v_sum1     = vld1q_u32( &pSqrSum1[isum] );      //Load a single vector from memory1
        v_sum1_end = vld1q_u32( &pSqrSum1[ilsum] );
        v_sum2     = vld1q_u32( &pSqrSum2[isum] );
        v_sum2_end = vld1q_u32( &pSqrSum2[ilsum] );
        
        v_LSqrSum = vsubq_u32( v_LSqrSumP, vaddq_u32(vsubq_u32(v_sum1_end, v_sum1), vsubq_u32(v_sum2_end, v_sum2)) );

        //u32 templMean = pLSum_row[i] / rr4;
        //u32 templSdv = (pLSqrSum_row[i] - (templMean * pLSum_row[i])) / rr4;
        uint32x4_t templMean = 
        uint32x4_t
uint32x4_t  vrecpeq_u32(uint32x4_t a)
        v_res = vaddq_u8( v_src1, v_src2 );
        vst1q_u8( pDst+offset, v_res );
    }

    if ( offset == len )
        return;

    for ( offset -= 16; offset<len; offset++) 
    {
        *(pDst+offset) = *(pSrc1+offset) + *(pSrc2+offset);
    }

    for(u32 i=0; i<cols; i++) //col
    {
        u32 isum  = (i+r2+1) << 1;
        u32 ilsum = i << 1;

        u32 templMean = pLSum_row[ilsum] / rr4;
        u32 templSdv = (pLSum_row[ilsum+1] - (templMean * pLSum_row[ilsum])) / rr4;

        //double kb = templSdv / (templSdv + level);
        pData_row[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pData_row[i]/(templSdv+level));
        //pData_row[i] = 255;
    }
}
#endif
