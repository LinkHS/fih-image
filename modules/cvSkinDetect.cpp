#include "includes.h"
#include "opencv.hpp"
#include "common/PreCalculatedTables.h"

using namespace cv;

//#define SKIN_MASK_IMSHOW
	

	
/*
	if (( R < 80) && ( B < 80 ) && (G < 80))
	{
		int mean;
		if ( ((G >= R) && (R >= B)) || ((B >= R) && (R >= G)) )
			mean = R;
		else if ( ((R >= G) && (G >= B)) || ((B >= G) && (G >= R)) )
			mean = G;
		else
			mean = B;
			
		int pureR = max(max(R, B), max(R, G)) - mean;
		int increase = 0.3 * (float)pureR;
		int max_dR = (float)(R * pureR) / 255.0; //R可以提高的最大值
		int max_dG = (float)(G * pureR) / 255.0; //G可以提高的最大值
		int max_dB = (float)(B * pureR) / 255.0; //B可以提高的最大值

		R = saturate_cast<uchar>( R - min(max_dR, increase) );
		G = saturate_cast<uchar>( G - min(max_dR, increase) );
		B = saturate_cast<uchar>( B - min(max_dR, increase) );

		p[j] = B; p[j+1] = G; p[j+2] = R;
	}
*/


void SkinDetectYUV(Mat& mSrc)
{
	Mat mYUV;
	CV_Assert( (mSrc.depth() == CV_8U) && (mSrc.channels() == 3) );

	cvtColor(mSrc, mYUV, CV_BGR2YCrCb);

	//As we only need to indicate if the pixel is skin or not, the type is CV_8UC1
	Mat mMask = Mat::zeros( mSrc.size(), CV_8UC1 );
	//mMask.create( mSrc.size(), CV_8UC1 );

	int channels = mYUV.channels();
	int nRows = mYUV.rows;
	int nCols = mYUV.cols * channels;

	if( mYUV.isContinuous() && mMask.isContinuous() )
	{
	    nCols *= nRows;
		nRows = 1;
	}

	int i, j;
	int Y, Cr, Cb;
	uchar* p; uchar* pMask;
	for( i = 0; i < nRows; ++i)
	{
		p = mYUV.ptr<uchar>(i);
		pMask = mMask.ptr<uchar>(i);
		for( j = 0; j < nCols; j+=3)
		{
			Y = p[j]; Cr = p[j+1]; Cb = p[j+2];

            if (( Y > 80 ) && ( Cb > 85 ) && ( Cb < 135 ) &&
                (Cr > 135) && (Cr < 180))
            {
                //skin pixel
				pMask[j/3] = 255;
            }
		}
	}

	imshow("skin", mMask);
}

void Median(uchar* current, uchar* next, uchar* previous, uchar* median)
{
	uchar arrayR[9], arrayG[9], arrayB[9];

	for(int i=-3; i<6; i+=3)
	{
		arrayB[i+3] = *(next+i); arrayB[i+4] = *(current+i); arrayB[i+5] = *(previous+i);

		arrayG[i+3] = *(next+i+1); arrayG[i+4] = *(current+i+1); arrayG[i+5] = *(previous+i+1);

		arrayR[i+3] = *(next+i+2); arrayR[i+4] = *(current+i+2); arrayR[i+5] = *(previous+i+2);
	}

	for( int i=0; i<9; i++ )
	{
		for( int j=1; j<(9-i); j++ )
		{
			if( arrayB[j] < arrayB[j-1] )
			{
				uchar temp = arrayB[j-1];
				arrayB[j-1] = arrayB[j];
				arrayB[j] = temp;
			}

			if( arrayG[j] < arrayG[j-1] )
			{
				uchar temp = arrayG[j-1];
				arrayG[j-1] = arrayG[j];
				arrayG[j] = temp;
			}

			if( arrayR[j] < arrayR[j-1] )
			{
				uchar temp = arrayR[j-1];
				arrayR[j-1] = arrayR[j];
				arrayR[j] = temp;
			}
		}
	}

	median[0] = arrayB[4]; median[1] = arrayG[4]; median[2] = arrayR[4];
}


void SkinPixelsDecect(Mat& mSrc, Mat& mDst)
{
    //As we only need to indicate if the pixel is skin or not, type is set to CV_8UC1
	mDst = Mat::zeros( mSrc.size(), CV_8UC1 );
	uchar R, G, B;

    const int nChannels = mSrc.channels();
	for(int j = 1; j < mSrc.rows - 1; ++j)
	{
		uchar* previous = mDst.ptr<uchar>(j - 1);
		uchar* next = mDst.ptr<uchar>(j + 1);
		
		uchar* p = mSrc.ptr<uchar>(j);
		uchar* pMask = mDst.ptr<uchar>(j);

		for(int i = nChannels; i < nChannels * (mSrc.cols - 1); i+=3)
		{
			B = p[i]; G = p[i+1]; R = p[i+2];
			if ( ( R > 95) && ( G > 40 ) && ( B > 20 ) && (R > G) && (R > B) )
			//if ( (R > G) && (R > B) )
			{
				//Mask Mat
				pMask[i/3] += 210; 
				pMask[i/3-1] += 1; pMask[i/3+1] += 1;
				previous[i/3] += 1; previous[i/3-1] += 1; previous[i/3+1] += 1;
				next[i/3] += 1; next[i/3-1] += 1; next[i/3+1] += 1;
			}
		}
	}

	//TODO, 加上第一行和最后一行的检测
	mDst.row(0).setTo(Scalar(0));
	mDst.row(mDst.rows - 1).setTo(Scalar(0));
	mDst.col(0).setTo(Scalar(0));
	mDst.col(mDst.cols - 1).setTo(Scalar(0));
}

void SkinSmooth(Mat& mSrc, Mat& mDst, Mat& mMask)
{   
	mDst.create(mSrc.size(), mSrc.type());

#ifdef SKIN_MASK_IMSHOW
	imshow("skin mask", mMask);
#endif

    const int nChannels = mSrc.channels();
	for(int j = 1; j < mSrc.rows - 1; ++j)  //因为是3x3小方块，所以要把第一行、第一列、最后一行和最后一列预留出来。
	{
		uchar* current = mSrc.ptr<uchar>(j);
		uchar* previous = mSrc.ptr<uchar>(j - 1);
		uchar* next = mSrc.ptr<uchar>(j + 1);

		uchar* p = mDst.ptr<uchar>(j);
		uchar* pMask = mMask.ptr<uchar>(j);

		for(int i = nChannels; i < nChannels * (mSrc.cols - 1); i+=3)
		{
			if ( pMask[i/3] )
			{
				//中值滤波
				Median(&current[i], &next[i], &previous[i], &p[i]);

				//均值滤波, BGR三个通道
				p[i]   = saturate_cast<uchar>((current[i]   + previous[i]   + next[i]   + current[i-3] + current[i+3] + previous[i-3] + previous[i+3] + next[i-3] + next[i+3])/9);
				p[i+1] = saturate_cast<uchar>((current[i+1] + previous[i+1] + next[i+1] + current[i-2] + current[i+4] + previous[i-2] + previous[i+4] + next[i-2] + next[i+4])/9);
				p[i+2] = saturate_cast<uchar>((current[i+2] + previous[i+2] + next[i+2] + current[i-1] + current[i+5] + previous[i-1] + previous[i+5] + next[i-1] + next[i+5])/9);
			}
			else
			{
			    p[i]   = current[i]; p[i+1] = current[i+1]; p[i+2] = current[i+2];
			}
		}
	}

	mDst.row(0).setTo(Scalar(0));
	mDst.row(mDst.rows - 1).setTo(Scalar(0));
	mDst.col(0).setTo(Scalar(0));
	mDst.col(mDst.cols - 1).setTo(Scalar(0));
}



void SkinColorOptimizeA(Mat& mSrc, Mat& mDst)
{
	//WYMC: White, Yellow, Magenta, Cyan
	const double WYMC_Params_RED[4] = {30, 10, 0, -40}; //Red
	const double WYMC_Params_YEL[4] = {30, 10, 0, -20}; //Yellow
	const double WYMC_Params_BLK[4] = {0};              //Black

	mDst.create(mSrc.size(), mSrc.type());

    const int nChannels = mSrc.channels();
	for(int j = 1; j < mSrc.rows - 1; ++j)
	{
		uchar* current = mSrc.ptr<uchar>(j);
		uchar* previous = mSrc.ptr<uchar>(j - 1);
		uchar* next = mSrc.ptr<uchar>(j + 1);

		uchar* p = mDst.ptr<uchar>(j);
		//uchar* pMask = mMask.ptr<uchar>(j);

		for(int i = nChannels; i < nChannels * (mSrc.cols - 1); i+=3)
		{
			int B = current[i]*10; int G = current[i+1]*10; int R = current[i+2]*10;
			int pureR;

			//if ( pMask[i/3] > 150 )
			if( (R > B) && (R > G) )
			{
				pureR = (R - max(G, B))/10;  //计算出红色差值，红色减去第二大颜色
				
				int max_dR = ((2550-R) * table_u8Divide255[pureR] + 500) / 1000; //R可以提高的最大值
				int max_dG = ((2550-G) * table_u8Divide255[pureR] + 500) / 1000; //G可以提高的最大值
				int max_dB = ((2550-B) * table_u8Divide255[pureR] + 500) / 1000; //B可以提高的最大值
				//int min_dR = (R * pureR / 255 + 5) / 10; //R可以减小的最大值
				//int min_dG = (G * pureR / 255 + 5) / 10; //G可以减小的最大值
				//int min_dB = (B * pureR / 255 + 5) / 10; //B可以减小的最大值

				int dW = table_u8Percent30[pureR];
				//(w% - y%) + w%*y%
				//int dB = (((WYMC_Params_RED[0] - WYMC_Params_RED[1]) + (WYMC_Params_RED[0] * WYMC_Params_RED[1])/100) * pureR + 500) / 1000;
				int dB = (23 * pureR + 50) / 100;
				//int dR = (((WYMC_Params_RED[0] - WYMC_Params_RED[3]) + (WYMC_Params_RED[0] * WYMC_Params_RED[3])/100) * pureR + 500) / 1000;
				int dR = (58 * pureR + 50) / 100;

				uchar tR = saturate_cast<uchar>( R/10 + min(max_dR, dR) );
				uchar tG = saturate_cast<uchar>( G/10 + min(max_dG, dW) );
				uchar tB = saturate_cast<uchar>( B/10 + min(max_dB, dB) );

				p[i] = tB; p[i+1] = tG; p[i+2] = tR;
				//B = tB*10; G = tG*10; R = tR*10;
			}
			/*
			if( (R > B) && (G > B) )
			{
				pureY = ( min(G, R) - B ) / 10;  //计算出红色差值，红色减去第二大颜色
				
				int max_dR = ((2550-R) * UcharDivide255[pureR] + 500) / 1000; //R可以提高的最大值
				int max_dG = ((2550-G) * UcharDivide255[pureR] + 500) / 1000; //G可以提高的最大值
				int max_dB = ((2550-B) * UcharDivide255[pureR] + 500) / 1000; //B可以提高的最大值
				//int min_dR = (R * pureR / 255 + 5) / 10; //R可以减小的最大值
				//int min_dG = (G * pureR / 255 + 5) / 10; //G可以减小的最大值
				//int min_dB = (B * pureR / 255 + 5) / 10; //B可以减小的最大值

				int dW = Percent30ofUchar[pureR];
				//(w% - y%) + w%*y%
				//int dB = (((WYMC_Params_RED[0] - WYMC_Params_RED[1]) + (WYMC_Params_RED[0] * WYMC_Params_RED[1])/100) * pureR + 500) / 1000;
				int dB = (23 * pureR + 50) / 100;
				//int dR = (((WYMC_Params_RED[0] - WYMC_Params_RED[3]) + (WYMC_Params_RED[0] * WYMC_Params_RED[3])/100) * pureR + 500) / 1000;
				int dR = (58 * pureR + 50) / 100;

				uchar tR = saturate_cast<uchar>( R/10 + min(max_dR, dR) );
				uchar tG = saturate_cast<uchar>( G/10 + min(max_dG, dW) );
				uchar tB = saturate_cast<uchar>( B/10 + min(max_dB, dB) );

				p[i] = tB; p[i+1] = tG; p[i+2] = tR;

			}
			*/
			/*
			else if ( pMask[i/3] )
			{
				int mean;
				if ( ((G >= R) && (R >= B)) || ((B >= R) && (R >= G)) )
					mean = R;
				else if ( ((R >= G) && (G >= B)) || ((B >= G) && (G >= R)) )
					mean = G;
				else
					mean = B;
			
				pureR = (max(max(R, B), max(R, G)) - mean)/10;
				//p[i] = current[i]; p[i+1] = current[i+1]; p[i+2] = current[i+2];
				//continue;
		    }
			*/
			else
			{
			    p[i] = current[i]; p[i+1] = current[i+1]; p[i+2] = current[i+2];
				continue;
			}
		}
	}

	mDst.row(0).setTo(Scalar(0));
	mDst.row(mDst.rows - 1).setTo(Scalar(0));
	mDst.col(0).setTo(Scalar(0));
	mDst.col(mDst.cols - 1).setTo(Scalar(0));
}

/* Compute different percentage of RGB based on the input WYMC parametes.
 *  WYMC[4] -> White, Yellow, Magenta, Cyan 
 *  diff_RGB[3] -> diff_PR, diff_PG, diff_PB
 */
void WYMC_ComputeDiffPerRGB( const int WYMC[4], int diff_RGB[3] )
{
    int White, Yellow, Magenta, Cyan;

    White   = WYMC[0];
    Yellow  = WYMC[1];
    Magenta = WYMC[2];
    Cyan    = WYMC[3];

    diff_RGB[0] = (White-Cyan) + (White*Cyan)/100;
    diff_RGB[1] = (White-Magenta) + (White*Magenta)/100;
    diff_RGB[2] = (White-Yellow) + (White*Yellow)/100;
}

void WYMCAdjustment_RedPixels(Mat& mSrc, Mat& mDst, const int WYMC[4])
{
	//WYMC: White, Yellow, Magenta, Cyan
	//        30     10       0     -40
	//int White = 50, Yellow = 0, Magenta = 0, Cyan = -70;

    int diff_RGB[3];
	int diff_PR, diff_PG, diff_PB; //adjustment percentage of Red, Green and Blue,

    WYMC_ComputeDiffPerRGB(WYMC, diff_RGB);
    diff_PR = diff_RGB[0];
    diff_PG = diff_RGB[1];
    diff_PB = diff_RGB[2];

	mDst.create(mSrc.size(), mSrc.type());

    const int nChannels = mSrc.channels();
#pragma omp parallel for
	for(int j = 0; j < mSrc.rows; ++j)
	{
		uchar* current = mSrc.ptr<uchar>(j);
		uchar* p = mDst.ptr<uchar>(j);

		for(int i = nChannels; i < nChannels * (mSrc.cols - 1); i+=3)
		{
			int B = current[i]*10; int G = current[i+1]*10; int R = current[i+2]*10;

			if( (R > B) && (R > G) )
			{
				int pureR = (R - max(G, B))/10;  //计算出红色差值，红色减去第二大颜色

				int max_dR = ((2550-R) * table_u8Divide255[pureR] + 500) / 1000; //R可以提高的最大值
				int max_dG = ((2550-G) * table_u8Divide255[pureR] + 500) / 1000; //G可以提高的最大值
				int max_dB = ((2550-B) * table_u8Divide255[pureR] + 500) / 1000; //B可以提高的最大值

				int dW = table_u8Percent30[pureR];
				int dB = (diff_PB * pureR + 50) / 100;
				int dR = (diff_PR * pureR + 50) / 100;

				uchar tR = saturate_cast<uchar>( R/10 + min(max_dR, dR) );
				uchar tG = saturate_cast<uchar>( G/10 + min(max_dG, dW) );
				uchar tB = saturate_cast<uchar>( B/10 + min(max_dB, dB) );

				p[i] = tB; p[i+1] = tG; p[i+2] = tR;
			}
			else
			{
			    p[i] = current[i]; p[i+1] = current[i+1]; p[i+2] = current[i+2];
				continue;
			}
		}
	}
}

void WYMCAdjustment_BlackPixels(Mat& mSrc, Mat& mDst, struct WYMCParams* pWYMCParams)
{
	//WYMC: White, Yellow, Magenta, Cyan
	//       -10      0       0      0
	int diff_PR, diff_PG, diff_PB; //adjustment percentage of Red, Green and Blue,

	if( pWYMCParams == NULL ) //default
	{
		diff_PR = -4; //(White-Cyan) + (White*Cyan)
		diff_PB = -4; //(White-Yellow) + (White*Yellow)
		diff_PG = -4;
	}
	else
	{
		diff_PR = 0; diff_PG = 0; diff_PB = 0;
	}

	mDst.create(mSrc.size(), mSrc.type());

    const int nChannels = mSrc.channels();
	for(int j=0; j<mSrc.rows; ++j)
	{
		uchar* current = mSrc.ptr<uchar>(j);
		uchar* p = mDst.ptr<uchar>(j);

		for(int i = nChannels; i < nChannels * (mSrc.cols - 1); i+=3)
		{
			int B = current[i]*10; int G = current[i+1]*10; int R = current[i+2]*10;

			if (( R < 500) && ( B < 500 ) && (G < 500))
			{
				int pureBL = 255 - max(max(R, G), max(R, B))/10;  //计算出黑色差值

				if( diff_PR > 0 )
				{
					int dR = (diff_PR * pureBL + 50) / 100; //Percent10ofUchar[pureBL];
					int max_dR = ((2550-R) * table_u8Divide255[pureBL] + 500) / 1000; //R可以提高的最大值
					p[i+2] = saturate_cast<uchar>( R/10 + min(max_dR, dR) );
				}
				else if (diff_PR < 0)
				{
					int dR = (diff_PR * pureBL - 50) / 100; //Percent10ofUchar[pureBL];
					int min_dR = (R * table_u8Divide255[pureBL] + 500) / 1000; //R可以减小的最大值
					p[i+2] = saturate_cast<uchar>( R/10 + max(-min_dR, dR) );
				}
				else
					p[i+2] = current[i+2];

				if( diff_PG > 0 )
				{
					int dG = (diff_PG * pureBL + 50) / 100;
					int max_dG = ((2550-G) * table_u8Divide255[pureBL] + 500) / 1000; //R可以提高的最大值
					p[i+1] = saturate_cast<uchar>( G/10 + min(max_dG, dG) );
				}
				else if (diff_PG < 0)
				{
					int dG = (diff_PG * pureBL - 50) / 100;
					int min_dG = (G * table_u8Divide255[pureBL] + 500) / 1000; //R可以减小的最大值
					p[i+1] = saturate_cast<uchar>( G/10 + max(-min_dG, dG) );
				}
				else
					p[i+1] = current[i+1];

				if( diff_PB > 0 )
				{
					int dB = (diff_PB * pureBL + 50) / 100;
					int max_dB = ((2550-B) * table_u8Divide255[pureBL] + 500) / 1000; //R可以提高的最大值
					p[i] = saturate_cast<uchar>( B/10 + min(max_dB, dB) );
				}
				else if (diff_PB < 0)
				{
					int dB = (diff_PB * pureBL - 50) / 100;
					int min_dB = (B * table_u8Divide255[pureBL] + 500) / 1000; //R可以减小的最大值
					p[i] = saturate_cast<uchar>( B/10 + max(-min_dB, dB) );
				}
				else
					p[i] = current[i];
			}
			else
			{
			    p[i] = current[i]; p[i+1] = current[i+1]; p[i+2] = current[i+2];
				continue;
			}
		}
	}
}