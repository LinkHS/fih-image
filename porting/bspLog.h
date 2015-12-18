#ifndef __BSP_LOG_H
#define __BSP_LOG_H

#define EN_BSP_LOG                   1 //enable bsp log
#define MTK_ANDROID_PLATFORM          //use codes on mtk android platform, not android app

// These functions will print using the LOG() function, using the same format as printf(). If you want it to be printed using a different
// function (such as for Android logcat output), then define LOG as your output function, otherwise it will use printf() by default.
#if (EN_BSP_LOG == 1)
	#include <stdio.h>
	
	// For stdout debug logging, with a new-line character on the end:
	
#if defined(MTK_ANDROID_PLATFORM)
		#include <cutils/log.h>
		//#define bspLOGV(fmt, arg...)       ALOGV(fmt, ##arg)
		#define bspLOGD       							ALOGD
		#define bspLOGI(fmt, args...)       ALOGI(fmt, ## arg)
		#define bspLOGW(fmt, args...)       ALOGW(fmt, ## arg)
		#define bspLOGE(fmt, args...)       ALOGE(fmt, " (%s){#%d:%s}", ##arg, __FUNCTION__, __LINE__, __FILE__)
#else
	#if defined(__linux__) && !defined(__ANDROID__)
        // Compiles on GCC but not MSVC:
		#define bspLOGD(fmt, args...)  do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)
    #define bspLOGI(fmt, args...)  do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)
		#define bspLOGW(fmt, args...)  do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)
		#define bspLOGE(fmt, args...)  do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)
	#endif

	// For Android debug logging to logcat:
  #if defined(__ANDROID__)
		#include <android/log.h>
		#define bspLOGI(fmt, args...) (__android_log_print(ANDROID_LOG_INFO, "........", fmt, ## args))
		#define bspLOGE(fmt, args...) (__android_log_print(ANDROID_LOG_ERROR, "........", fmt, ## args))
	#endif
#endif
 
	#if defined(_WIN32) || (_MSC_VER)
		#define bspLOGI  printf
		#define bspLOGE  printf
	#endif
	
#else
	#define bspLOGI(fmt, args...)
	#define bspLOGE(fmt, args...)
#endif //BSP_LOG

#endif //__BSP_LOG_H