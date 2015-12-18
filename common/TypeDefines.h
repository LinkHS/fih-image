#ifndef __TYPE_DEFINES_H
#define __TYPE_DEFINES_H

typedef unsigned char uchar;
typedef unsigned char u8;
typedef unsigned int  u32;
typedef unsigned int  uint;

typedef int int32;
typedef int32 intneg;
typedef int32 intpos;

enum cvInputType{
	CV_INPUT_Y,
	CV_INPUT_GRAY,
    CV_INPUT_BGR,
	CV_INPUT_YUV
};

#endif