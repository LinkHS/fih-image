#include "includes.h"
#include "opencv.hpp"

#include "cvFaceDetect.h"

#include "3rdParty/flandmark/flandmark_detector.h"

using namespace cv;

//#define TAIWAN_TEST

#define LOGE bspLOGD

/** Function Headers */

/** Global variables */
// Haar Cascade file, used for Face Detection.
#ifdef TAIWAN_TEST
  static const char* face_cascade_name = "D:\\NJImage\\frontalface.xml";
#else
  static const char* face_cascade_name = "/sdcard/haarcascade_frontalface_alt2.xml";
#endif

static const char* eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
static const char* mouth_cascade_name = "haarcascade_mcs_mouth.xml";

#ifdef TAIWAN_TEST
  static const char* flandmark_model_name = "D:\\NJImage\\model.dat";
#else
  static const char* flandmark_model_name = "flandmark_model.dat";
#endif


static CascadeClassifier face_cascade;

#if USE_EYES_CASCADE == 1
  static CascadeClassifier eyes_cascade;
#endif

#if USE_MOUTH_CASCADE == 1
  static CascadeClassifier mouth_cascade;
#endif


enum {
	FACE_CASCADE  = 1,
	EYES_CASCADE  = 2,
	MOUTH_CASCADE = 4
};
	
static bool cascadeIsLoaded(int cascade)
{
	static unsigned char loadedbits = 0;
	
	switch( cascade )
	{
        case FACE_CASCADE:
			if( loadedbits & 0x01 )
				return true;
			loadedbits &= 0x01;
			break;
		case EYES_CASCADE:
			if( loadedbits & 0x02 )
				return true;
			loadedbits &= 0x02;
			break;
		case MOUTH_CASCADE:
			if( loadedbits & 0x04 )
				return true;
			loadedbits &= 0x04;
			break;
	}

	return false;
}

/*
 * return value: true  => successful
 *               false => failed
 */
static bool FDLoadCascade(int cascade)
{
    if( cascadeIsLoaded(cascade) )
    	return true;
	
    switch( cascade )
    {
        case FACE_CASCADE:
        	if( !face_cascade.load( face_cascade_name ) )
        	{ 
        	    LOGE("--(!)Error loading face cascade\n");
        		return false;
        	}
        	break;
        case EYES_CASCADE:
        	//if( !eyes_cascade.load( eyes_cascade_name ) )
            {
                LOGE("--(!)Error loading eyes cascade\n");
                return false;
        	}
        	break;
        case MOUTH_CASCADE:
        	//if( !mouth_cascade.load( mouth_cascade_name ) )
        	{ 
        	    LOGE("--(!)Error loading mouth cascade\n");
        	    return false;
        	}
        	break;
    }
	
	return true;
}

void cvFaceDectect(uchar* pYUYV, int width, int height, std::vector<Rect> &faces, bool overlay)
{   
    Mat mSrcYUV( height, width, CV_8UC2, pYUYV );
    Mat mChannels[2];
    Mat &frame_gray = mChannels[0];
    split(mSrcYUV, mChannels);
    
    if( FDLoadCascade(FACE_CASCADE) == false )
        return;
        
    //equalizeHist( frame_gray, frame_gray );

    face_cascade.detectMultiScale( frame_gray, faces, 2, 3, (0 | CASCADE_SCALE_IMAGE), Size(100, 100) );
    
    if( overlay == true )
    {
        for ( size_t i = 0; i < faces.size(); i++ )
        {
            Point center( faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );
            ellipse( frame_gray, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0); 
        }
    }

    merge(mChannels, 2, mSrcYUV);
}


void cvFaceDectect(Mat &mSrc, std::vector<Rect> &faces, FIH_FaceDectParms& faceDectParms)
{
    Mat frame_gray;
    
    if( FDLoadCascade(FACE_CASCADE) == false )
        return;

    if( mSrc.channels() != 1 )
        cvtColor( mSrc, frame_gray, COLOR_BGR2GRAY );
    else    
        frame_gray = mSrc;
    
    //equalizeHist( frame_gray, frame_gray );

    face_cascade.detectMultiScale( frame_gray,
                                   faces, 
                                   faceDectParms.scaleFactor,
                                   faceDectParms.minNeighbors,
                                   faceDectParms.flags,
                                   cv::Size(faceDectParms.minSizeW,faceDectParms.minSizeH)
                                 );

    bspLOGD("Dawei: Faces are %d", faces.size());
    
    if( faceDectParms.overlay == true )
    {
        for ( size_t i = 0; i < faces.size(); i++ )
        {
            Point center( faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );
            ellipse( mSrc, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0); 
        }
    }
}

void cvFaceDectect(Mat &mSrc, std::vector<Rect> &faces, bool overlay)
{
    Mat frame_gray;
    
    if( FDLoadCascade(FACE_CASCADE) == false )
        return;

    cvtColor( mSrc, frame_gray, COLOR_BGR2GRAY );
    //equalizeHist( frame_gray, frame_gray );

    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 3, 0|CASCADE_SCALE_IMAGE, Size(80, 80) );
    
    if( overlay == true )
    {
        for ( size_t i = 0; i < faces.size(); i++ )
        {
            Point center( faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );
            ellipse( mSrc, center, Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar( 255, 0, 255 ), 4, 8, 0); 
        }
    }
}

#if 0
void cvFaceFeatureDectect(Mat &mSrc, std::vector<Rect> &faces)
{
    Mat mGray;
    int bbox[4];

	/*
	 * Load flandmark model
	 */ 
	FLANDMARK_Model * model = flandmark_init(flandmark_model_name);
	
    if( model == 0 )
    {
        LOGE("Structure model wasn't created. Corrupted file flandmark_model.dat?\n");
        return;
    }

    /*
     * convert mSrc to gray if needed
     */ 
    if( mSrc.channels() == 1 )      // if channels == 1, assume mSrc as gray image
    {
        mGray = mSrc;
    }
    else if( mSrc.channels() == 3 ) // otherwise convert image to grayscale
    {
        cvtColor( mSrc, mGray, COLOR_BGR2GRAY );       
    }
    
    
    double *landmarks = (double*)malloc(2*model->data.options.M*sizeof(double));
    
	for ( int i = 0; i < faces.size(); i++ )
    {
        bbox[0] = faces[i].x;
        bbox[1] = faces[i].y;
        bbox[2] = faces[i].x + faces[i].width;
        bbox[3] = faces[i].y + faces[i].height;
        
		IplImage ipl_img = mGray;
        flandmark_detect(&ipl_img, bbox, model, landmarks);

        // display landmarks
		Point center( faces[i].x + faces[i].width/2, faces[i].y + faces[i].height/2 );
        ellipse( mSrc, center, cv::Size( faces[i].width/2, faces[i].height/2 ), 0, 0, 360, cv::Scalar( 255, 0, 255 ), 4, 8, 0 );

		rectangle(mSrc, cvPoint(bbox[0], bbox[1]), cvPoint(bbox[2], bbox[3]), CV_RGB(255,0,0) );
        rectangle(mSrc, cvPoint(model->bb[0], model->bb[1]), cvPoint(model->bb[2], model->bb[3]), CV_RGB(0,0,255) );
        circle(mSrc, cvPoint((int)landmarks[0], (int)landmarks[1]), 3, CV_RGB(0, 0,255), CV_FILLED);
        for (int i = 2; i < 2*model->data.options.M; i += 2)
        {
            circle(mSrc, cvPoint(int(landmarks[i]), int(landmarks[i+1])), 3, CV_RGB(255,0,0), CV_FILLED);
        }
    }
   
    // cleanup
    free(landmarks);
    flandmark_free(model);
}

#endif

void cvEyesDectect(Mat &mSrc, std::vector<Rect> &faces, std::vector<Rect> &eyes, bool overlay)
{
/*
    Mat frame_gray;

	//it assumes input is a gray image when channels == 1, otherwise convert it to gray
	if( mSrc.channels() != 1 ) 
	{
    	cvtColor( mSrc, frame_gray, COLOR_BGR2GRAY );    	
	}
    equalizeHist( frame_gray, frame_gray );

    for ( size_t i = 0; i < faces.size(); i++ )
    {
        //-- In each face, detect eyes
        eyes_cascade.detectMultiScale( faceROI, eyes, 1.1, 4, 0 |CASCADE_SCALE_IMAGE, Size(30, 30) );
        for ( size_t j = 0; j < eyes.size(); j++ )
        {
            Point eye_center( faces[i].x + eyes[j].x + eyes[j].width/2, faces[i].y + eyes[j].y + eyes[j].height/2 );
            int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
            circle( frame, eye_center, radius, Scalar( 255, 0, 0 ), 4, 8, 0 );
        }
    }
*/
}

void cvMouthDectect(Mat &mSrc, std::vector<Rect> &faces, std::vector<Rect> &mouthes, bool overlay)
{
/*
    Mat frame_gray;

    cvtColor( mSrc, frame_gray, COLOR_BGR2GRAY );
    equalizeHist( frame_gray, frame_gray );
    face_cascade.detectMultiScale( frame_gray, faces, 1.1, 3, 0|CASCADE_SCALE_IMAGE, Size(30, 30) );

    //-- In each face, detect mouthes
    for ( size_t i = 0; i < faces.size(); i++ )
    {
		mouth_cascade.detectMultiScale( faceROI, mouthes, 1.1, 4, 0 |CASCADE_SCALE_IMAGE, Size(30, 30) );
        for ( size_t j = 0; j < mouthes.size(); j++ )
        {
            Point mouth_center( faces[i].x + mouthes[j].x + mouthes[j].width/2, faces[i].y + mouthes[j].y + mouthes[j].height/2 );
            int radius = cvRound( (mouthes[j].width + mouthes[j].height)*0.25 );
            circle( frame, mouth_center, radius, Scalar( 255, 255, 0 ), 4, 8, 0 );
        }
    }
*/
}

