#define LOG_TAG "FIH/cvLSFilter"
#define DEBUG_LEVEL                 0

#include "includes.h"
#include "cvLocalStatisticsFiltering.h"
#include "common/PreCalculatedTables.h"
#include "basic_process/cvOthers.h"
#include "porting/debug.h"

using namespace cv; 

#if DEBUG_PLATFORM != DEBUG_IN_WINDOWS
  #define MY_LOGD(fmt, arg...)        FIH_LOGD("[%s] "fmt, __FUNCTION__, ##arg)
  #define MY_LOGI(fmt, arg...)        FIH_LOGI("[%s] "fmt, __FUNCTION__, ##arg)
  #define MY_LOGW(fmt, arg...)        FIH_LOGW("[%s] "fmt, __FUNCTION__, ##arg)
  #define MY_LOGE(fmt, arg...)        FIH_LOGE("[%s] "fmt, __FUNCTION__, ##arg)
  #define MY_LOGT(fmt, arg...)        FIH_LOGD("Temp: [%s] "fmt, __FUNCTION__, ##arg) //temp log

  #define MY_LOGD_IF(cond, ...)       do { if (cond) { MY_LOGD(__VA_ARGS__); } }while(0)
  #define MY_LOGD_LEVEL(level, ...)   do { if (level >= DEBUG_LEVEL) { MY_LOGD(__VA_ARGS__); } }while(0)

#else
  #define MY_LOGD(fmt, ...)    do { FIH_LOGD("[%s] ", __FUNCTION__); FIH_LOGD(fmt, __VA_ARGS__); FIH_LOGD("\n"); } while (0)
#endif

//Local Statistics Variance Noise Filtering
void FastLSV_NoiseFilteringOld(Mat& mSrc, Mat& mDst, Mat& mCumulSum, Mat& mCumulSquareSum, int radius, int level)
{
	int rm1=radius-1, rp1=radius+1, rr=4*radius*radius; //rm1,radius minus 1; rp1, radius plus 1;
	mDst.create(mSrc.size(), mSrc.type());
	
	for(int j = radius; j < (mSrc.rows)-rp1; j++)
	{
		uchar* current = mSrc.ptr<uchar>(j);
		uchar* pDst = mDst.ptr<uchar>(j);
	    int* pSum; int* pSquareSum;

		for(int i = rp1; i < (mSrc.cols - rp1); i++)
		{
			double templMean, templSdv;
			int lSum = 0, lSquareSum = 0;
			for(int m = -radius; m < radius; m++)
			{
				pSum = mCumulSum.ptr<int>(j+m);
				pSquareSum = mCumulSquareSum.ptr<int>(j+m); 
				lSum += pSum[i+rm1] - pSum[i-rp1];
				lSquareSum += pSquareSum[i+rm1] - pSquareSum[i-rp1];
			}
			templMean = (double)lSum / rr;
			templSdv = (lSquareSum - (((double)lSum/(2*radius)) * ((double)lSum/(2*radius)))) / rr;

			double kb = templSdv / (templSdv + 10 + level * level * 5);
			pDst[i] = (uchar)((1-kb)*(double)templMean + kb*(double)current[i]);
		}	
	}
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
static void FastLSV_NoiseFiltering(Mat& mSrc, Mat& mDst, Mat& mCumulSum, Mat& mCumulSquareSum, int radius, int level)
{
	int cols = mSrc.cols, rows = mSrc.rows;
    const int CPUs =  8;
    //const int sumW = cols + (radius << 1) + 1;
    //const int sumH = rows + (radius << 1);
    const int blkH = rows >> 3; 

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) || (mSrc.type() != CV_8UC1) )
	{
	    mDst = mSrc;
		return;
	}

	level = level * level * 5 + 10; //filtering level

	Mat mLSums( 2*CPUs, cols, CV_32S, Scalar(0));
	Mat mLSqrSums( 2*CPUs, cols, CV_32S, Scalar(0));

	mDst.create(mSrc.size(), CV_8UC1);

	const int rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;

#pragma omp parallel for	
    for(u8 index=0; index<CPUs; index++)
    {
    	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
    	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
    	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
    	int* plSum = mLSums.ptr<int>(index<<1);
        int* plSqrSum = mLSqrSums.ptr<int>(index<<1);
    	uchar* pSrc = mSrc.ptr<uchar>(blkH * index);
    	uchar* pDst = mDst.ptr<uchar>(blkH * index);

    	for(int j=0; j<r2+1; j++) //row
    	{
            int* pSum = mCumulSum.ptr<int>(index*blkH + j);
            int* pSqrSum = mCumulSquareSum.ptr<int>(index*blkH + j);

    		for(int i=0; i<cols; i++) //col
    		{
    			plSum[i] += (pSum[i+r2+1] - pSum[i]);			
    			plSqrSum[i] += (pSqrSum[i+r2+1] - pSqrSum[i]);
    		}
    	}
    	for(int i=0; i<cols; i++) //col
    	{
    		int templMean = plSum[i] / rr4;
    		int templSdv = (plSqrSum[i] - (templMean * plSum[i])) / rr4;

    		/* double kb = templSdv / (templSdv + level);
    		   pDst[i] = (uchar)((1-kb)*templMean + kb*pSrc[i]); */
            pDst[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pSrc[i]/(templSdv+level));
    	}

    	/* 2. Compute the rest rows from second row */
    	//for(int j=1; j<rows; j++) //row
        for(int j=index*blkH+1; j<(index+1)*blkH; j++) //row
    	{
            int* plSumP;
            int* plSqrSumP;

    		pSrc = mSrc.ptr<uchar>(j);
    		pDst = mDst.ptr<uchar>(j);

    		int* pSum1 = mCumulSum.ptr<int>(j-1);     //the line which should be removed
    		int* pSqrSum1 = mCumulSquareSum.ptr<int>(j-1);  //the line which should be removed
    		int* pSum2 = mCumulSum.ptr<int>(j+r2);     //the line which should be added = pSum1 + 2*radius + 1
    		int* pSqrSum2 = mCumulSquareSum.ptr<int>(j+r2);  //the line which should be added = pSum1 + 2*radius + 1
    		
    		if( j&0x01 )
    		{
                plSumP = mLSums.ptr<int>( index<<1 );
                plSqrSumP = mLSqrSums.ptr<int>( index<<1 );
                plSum = mLSums.ptr<int>( (index<<1) + 1 ); 
                plSqrSum = mLSqrSums.ptr<int>( (index<<1) + 1 );
    		}else
    		{
                plSum = mLSums.ptr<int>( index<<1 );
                plSqrSum = mLSqrSums.ptr<int>( index<<1 );
                plSumP = mLSums.ptr<int>( (index<<1) + 1 ); 
                plSqrSumP = mLSqrSums.ptr<int>( (index<<1) + 1 );
    		}

    		for( int i=0; i<cols; i++ ) //col
    		{
    			plSum[i] = plSumP[i] - (pSum1[i+r2+1] - pSum1[i]) + (pSum2[i+r2+1] - pSum2[i]);
    			plSqrSum[i] = plSqrSumP[i] - (pSqrSum1[i+r2+1] - pSqrSum1[i]) + (pSqrSum2[i+r2+1] - pSqrSum2[i]);

    			int templMean = plSum[i] / rr4;
    			int templSdv = (plSqrSum[i] - (templMean * plSum[i])) / rr4;

                /* double kb = templSdv / (templSdv + level);
    		       pDst[i] = (uchar)((1-kb)*templMean + kb*pSrc[i]); */
                pDst[i] = (u8)(level*templMean/(templSdv+level) + templSdv*pSrc[i]/(templSdv+level));
    		}
    	}
    }
}

static void FastLSV_NoiseFiltering_backup(Mat& mSrc, Mat& mDst, Mat& mCumulSum, Mat& mCumulSquareSum, int radius, int level)
{
	int cols = mSrc.cols, rows = mSrc.rows;

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) || (mSrc.type() != CV_8UC1) )
	{
	    mDst = mSrc;
		return;
	}

	level = level * level * 5 + 10; //filtering level

	Mat mLSums( 2, cols, CV_32S, Scalar(0));
	Mat mLSqrSums( 2, cols, CV_32S, Scalar(0));

	mDst.create(mSrc.size(), CV_8UC1);

	int rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;
	
	int* plSum; int* plSqrSum; //Local Sum, Local Square Sum
	int* pSum; int* pSqrSum; 
	uchar* pSrc = mSrc.ptr<uchar>(0);
	uchar* pDst = mDst.ptr<uchar>(0);

	//pDst = mDst.ptr<uchar>(0);

	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
	plSum = mLSums.ptr<int>(0); plSqrSum = mLSqrSums.ptr<int>(0);
	for(int j=0; j<r2+1; j++) //row
	{
		pSum = mCumulSum.ptr<int>(j); pSqrSum = mCumulSquareSum.ptr<int>(j);
		
		for(int i=0; i<cols; i++) //col
		{
			plSum[i] += (pSum[i+r2+1] - pSum[i]);			
			plSqrSum[i] += (pSqrSum[i+r2+1] - pSqrSum[i]);
		}
	}
	for(int i=0; i<cols; i++) //col
	{
		double templMean = plSum[i] / rr4;
		double templSdv = (plSqrSum[i] - (templMean * plSum[i])) / rr4;

		double kb = templSdv / (templSdv + level);
		pDst[i] = (uchar)((1-kb)*templMean + kb*pSrc[i]);
	}

	/* 2. Compute the rest rows from second row */
	int previous = 0, current = 1, temp = 0;
	for(int j=1; j<rows; j++) //row
	{
		pSrc = mSrc.ptr<uchar>(j);
		pDst = mDst.ptr<uchar>(j);

		int* pSum1 = mCumulSum.ptr<int>(j-1);            //the line which should be removed
		int* pSqrSum1 = mCumulSquareSum.ptr<int>(j-1);   //the line which should be removed
		int* pSum2 = mCumulSum.ptr<int>(j+r2);           //the line which should be added = pSum1 + 2*radius + 1
		int* pSqrSum2 = mCumulSquareSum.ptr<int>(j+r2);  //the line which should be added = pSum1 + 2*radius + 1
		
		int* plSumP = mLSums.ptr<int>(previous);
		int* plSqrSumP =mLSqrSums.ptr<int>(previous);

		plSum = mLSums.ptr<int>(current); plSqrSum = mLSqrSums.ptr<int>(current);

		for(int i=0; i<cols; i++) //col
		{
			plSum[i] = plSumP[i] - (pSum1[i+r2+1] - pSum1[i]) + (pSum2[i+r2+1] - pSum2[i]);
			plSqrSum[i] = plSqrSumP[i] - (pSqrSum1[i+r2+1] - pSqrSum1[i]) + (pSqrSum2[i+r2+1] - pSqrSum2[i]);

			double templMean = plSum[i] / rr4;
			double templSdv = (plSqrSum[i] - (templMean * plSum[i])) / rr4;

			double kb = templSdv / (templSdv + level);
			pDst[i] = (uchar)((1-kb)*templMean + kb*pSrc[i]);
		}

		//exchange buffer position
		temp = current; current = previous; previous = temp;
	}
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
static void FastLSV_NoiseFiltering_Rough(Mat& mSrc, Mat& mDst, Mat& mCumulSum, Mat& mCumulSquareSum, int radius, int level)
{
	int cols = mSrc.cols, rows = mSrc.rows;

	if( radius == 0 || level == 0 || (cols < radius) || (rows < radius) || (mSrc.type() != CV_8UC1) )
	{
	    mDst = mSrc;
		return;
	}

	level = level * level * 5 + 10; //filtering level

	Mat mLSums( 2, cols, CV_32S, Scalar(0));
	Mat mLSqrSums( 2, cols, CV_32S, Scalar(0));

	mDst.create(mSrc.size(), CV_8UC1);

	int rr4=(2*radius+1)*(2*radius+1), r2=radius*2; //rm1,radius minus 1; rp1, radius plus 1;
	
	int* plSum; int* plSqrSum; //Local Sum, Local Square Sum
	int* pSum; int* pSqrSum; 
	uchar* pSrc = mSrc.ptr<uchar>(0);
	uchar* pDst = mDst.ptr<uchar>(0);

	//pDst = mDst.ptr<uchar>(0);

	/* 1. Calculate local sums and loacl square sums of each pixel in first row. */
	/* This step makes it much easier to calculate local sums and local square sums of each pixels in the rest rows.
	   Beacuse they can be calculated by adding sums of pixels of new row and removing old ones. */
	plSum = mLSums.ptr<int>(0); plSqrSum = mLSqrSums.ptr<int>(0);
	for(int j=0; j<r2+1; j++) //row
	{
		pSum = mCumulSum.ptr<int>(j); pSqrSum = mCumulSquareSum.ptr<int>(j);
		
		for(int i=0; i<cols; i++) //col
		{
			plSum[i] += (pSum[i+r2+1] - pSum[i]);			
			plSqrSum[i] += (pSqrSum[i+r2+1] - pSqrSum[i]);
		}
	}
	for(int i=0; i<cols; i++) //col
	{
		int templMean = plSum[i] / rr4;
		int templSdv = (plSqrSum[i] - (templMean * plSum[i])) / rr4;
		//int kb = templSdv / (templSdv + level);
		pDst[i] = (uchar)(level*templMean/(templSdv+level) + templSdv*pSrc[i]/(templSdv+level));

	}

	/* 2. Calculate the rest rows from second row */
	int previous = 0, current = 1, temp = 0;
	for(int j=1; j<rows; j++) //row
	{
		pSrc = mSrc.ptr<uchar>(j);
		pDst = mDst.ptr<uchar>(j);

		int* pSum1 = mCumulSum.ptr<int>(j-1);            //the line which should be removed
		int* pSqrSum1 = mCumulSquareSum.ptr<int>(j-1);   //the line which should be removed
		int* pSum2 = mCumulSum.ptr<int>(j+r2);           //the line which should be added = pSum1 + 2*radius + 1
		int* pSqrSum2 = mCumulSquareSum.ptr<int>(j+r2);  //the line which should be added = pSum1 + 2*radius + 1
		
		int* plSumP = mLSums.ptr<int>(previous);
		int* plSqrSumP =mLSqrSums.ptr<int>(previous);

		plSum = mLSums.ptr<int>(current); plSqrSum = mLSqrSums.ptr<int>(current);

		for(int i=0; i<cols; i++) //col
		{
			plSum[i] = plSumP[i] - (pSum1[i+r2+1] - pSum1[i]) + (pSum2[i+r2+1] - pSum2[i]);
			plSqrSum[i] = plSqrSumP[i] - (pSqrSum1[i+r2+1] - pSqrSum1[i]) + (pSqrSum2[i+r2+1] - pSqrSum2[i]);

			int templMean = plSum[i] / rr4;
			int templSdv = (plSqrSum[i] - (templMean * plSum[i])) / rr4;

			//double kb = templSdv / (templSdv + level);
			pDst[i] = (uchar)(level*templMean/(templSdv+level) + templSdv*pSrc[i]/(templSdv+level));
		}

		//exchange buffer position
		temp = current; current = previous; previous = temp;
	}
}


void CalCumulDist_Rows(Mat& mOneChn, Mat& mCumulSum, Mat& mCumulSquareSum) //One Channel
{	
	mCumulSum.create(mOneChn.size(), CV_32S);
	mCumulSquareSum.create(mOneChn.size(), CV_32S);

	for(int j = 0; j < mOneChn.rows; j++)
	{
		uchar* current = mOneChn.ptr<uchar>(j);
		int* pSum = mCumulSum.ptr<int>(j);
		int* pSquareSum = mCumulSquareSum.ptr<int>(j);

		pSum[0] = current[0];
		pSquareSum[0] = table_u8Squares[current[0]];
		for(int i = 1; i < mOneChn.cols; i++)
		{
			pSum[i] = pSum[i-1] + current[i];
			pSquareSum[i] = pSquareSum[i-1] + table_u8Squares[current[i]];
		}	
	}
}

void CalCumulDist_Rows(Mat& mOneChn, Mat& mCumulSum, Mat& mCumulSquareSum, int radius) //One Channel
{	 
	Size sumSize;
	sumSize.width = mOneChn.cols + (radius << 1) + 1;
	sumSize.height = mOneChn.rows + (radius << 1);
	mCumulSum.create(sumSize, CV_32S);
	mCumulSquareSum.create(sumSize, CV_32S);

#pragma omp parallel for
	for(int j=radius; j<sumSize.height-radius; j++) //row
	{
		uchar* current = mOneChn.ptr<uchar>(j-radius);
		int* pSum = mCumulSum.ptr<int>(j);
		int* pSqrSum = mCumulSquareSum.ptr<int>(j);

		// We add one zero colunmn before first column to help calculate local sums which include pixels in first column.
		// i.e., to get the sum of column 1 to column 7, we use cumulative sums (col1 to col7) minus 0.
		// This is because we need to put in a buffer of zero values along x-axis and y-axis in order to make 
		// computation efficient.
		pSum[0] = 0; pSqrSum[0] = 0;

		// Compute cumulative sums starts from column_1 to column_radius
		for(int i=1; i<=radius; i++)
		{
			int current_pixel = current[radius - i + 1];
			pSum[i] = pSum[i-1] + current_pixel;
			pSqrSum[i] = pSqrSum[i-1] + table_u8Squares[current_pixel];		
		}

	    // Compute cumulative sums starts from column_(radius+1) to column_(radius+1) last
		for(int i=(radius+1); i<=(sumSize.width-radius-1); i++)
		{
			int current_pixel = current[i - radius - 1];
			pSum[i] = pSum[i-1] + current_pixel;
			pSqrSum[i] = pSqrSum[i-1] + table_u8Squares[current_pixel];
		}

		// Compute cumulative sums starts from column_radius to end
		for(int i=(sumSize.width-radius); i<sumSize.width; i++)
		{
			int current_pixel = current[ mOneChn.cols - (radius - (sumSize.width - i)) -1];
			pSum[i] = pSum[i-1] + current_pixel;
			pSqrSum[i] = pSqrSum[i-1] + table_u8Squares[current_pixel];
		}
	}

	int r2 = radius * 2, rl = sumSize.height-radius;
	for(int j=0; j<radius; j++) //row
	{
		mCumulSum.row(r2-j).copyTo(mCumulSum.row(j)); 
		mCumulSum.row(rl-j-2).copyTo(mCumulSum.row(rl+j));
		mCumulSquareSum.row(r2-j).copyTo(mCumulSquareSum.row(j)); 
		mCumulSquareSum.row(rl-j-2).copyTo(mCumulSquareSum.row(rl+j));
	}
}


/*
 * Assume that mSrc is 8UC3_BGR or 8UC1_Y !!!
 */
void cvLocalStatisticsFiltering_YChannel(Mat& mSrc, Mat& mDst, int radius, int level)
{
	Mat mCumulSum, mCumulSquareSum, mtY;
	Mat channels[3];
	Mat &mY = channels[0];

    if( mSrc.channels() == 3 )  //8UC3_BGR
    {
        cvConvertAndSplit(mSrc, channels);
    }else                       //8UC1_Y
    {
        mY = mSrc;
    }
    
	//CalCumulDist_Rows(mY, mCumulSum, mCumulSquareSum, radius);
    cvGetFunCostTime( CalCumulDist_Rows(mY, mCumulSum, mCumulSquareSum, radius), "CalCumulDist_Rows" );

	//FastLSV_NoiseFiltering(mY, mtY, mCumulSum, mCumulSquareSum, radius, level);
    cvGetFunCostTime( FastLSV_NoiseFiltering(mY, mtY, mCumulSum, mCumulSquareSum, radius, level), "FastLSV_NoiseFiltering" );


    Mat m1, m2;
    cvGetFunCostTime(boxFilter(mSrc, m1, CV_32SC1, cv::Size(radius, radius), Point(-1,-1), false), "box1");
    cvGetFunCostTime(boxFilter(mSrc.mul(mSrc), m2, CV_32SC1, cv::Size(radius, radius), Point(-1,-1), false), "box2");

    if( mSrc.channels() == 3 )  //8UC3_BGR
    {
        channels[0] = mtY;     
    	cvMergeAndConvert(channels, mDst);
    }else                       //8UC1_Y
    {
        mDst = mtY;
    }
}


/*
 * Assume that mSrc is 8UC3_BGR or 8UC1_Y !!!
 */
void cvLocalStatisticsFiltering_YChannel_Rough(Mat& mSrc, Mat& mDst, int radius, int level)
{
    //Mat mTemp;
    //resize(mSrc, mTemp, Size((mSrc.cols)>>2, (mSrc.rows)>>2));
    //mSrc = mTemp;

	Mat mCumulSum, mCumulSquareSum, mtY;
	Mat channels[3];
	Mat &mY = channels[0];

    if( mSrc.channels() == 3 )  //8UC3_BGR
    {
        cvConvertAndSplit(mSrc, channels);
    }else                       //8UC1_Y
    {
        mY = mSrc;
    }
    
	CalCumulDist_Rows(mY, mCumulSum, mCumulSquareSum, radius);
	FastLSV_NoiseFiltering_Rough(mY, mtY, mCumulSum, mCumulSquareSum, radius, level);

    if( mSrc.channels() == 3 )  //8UC3_BGR
    {
        channels[0] = mtY;     
    	cvMergeAndConvert(channels, mDst);
    }else                       //8UC1_Y
    {
        //resize(mtY, mTemp, Size((mSrc.cols)<<2, (mSrc.rows)<<2));
        //mDst = mTemp;
        mDst = mtY;
    }
}
