#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>

#define DEBUG_IN_ANDROID1           0
#define DEBUG_IN_ANDROID2           1
#define DEBUG_IN_LINUX              2
#define DEBUG_IN_WINDOWS            3

#define DEBUG_LEVEL_0               0
#define DEBUG_LEVEL_1               1
#define DEBUG_LEVEL_2               2

#if defined (_WIN32) || (_MSC_VER) || (WIN32) || (_WINDOWS)
    #define DEBUG_PLATFORM          DEBUG_IN_WINDOWS
#elif defined (__ANDROID__)
    #define DEBUG_PLATFORM          DEBUG_IN_ANDROID1
#elif defined (__linux__)
    #define DEBUG_PLATFORM          DEBUG_IN_LINUX
#endif
/* These functions will print using the LOG() function, using the same format as printf(). If you want it to be printed using a different
   function (such as for Android logcat output), then define LOG as your output function, otherwise it will use printf() by default. */
	
/* For stdout debug logging, with a new-line character on the end */

#if (DEBUG_PLATFORM == DEBUG_IN_ANDROID1)
#pragma message("DEBUG_IN_ANDROID1")
    #include <cutils/log.h>
    #define FIH_LOGV(fmt, args...)       ALOGV(fmt, ## args)
    #define FIH_LOGD(fmt, args...)       ALOGD(fmt, ## args)
    #define FIH_LOGI(fmt, args...)       ALOGI(fmt, ## args)
    #define FIH_LOGW(fmt, args...)       ALOGW(fmt, ## args)
    //#define FIH_LOGE(fmt, args...)       ALOGE(fmt, " (%s){#%d:%s}", ##args, __FUNCTION__, __LINE__, __FILE__)
    #define FIH_LOGE(fmt, args...)       ALOGE(fmt, ## args)

/* For Android debug logging to logcat: */
#elif DEBUG_PLATFORM == DEBUG_IN_ANDROID2
#pragma message("DEBUG_IN_ANDROID2")
  #if 0
    typedef enum android_LogPriority {
        ANDROID_LOG_UNKNOWN = 0,
        ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
        ANDROID_LOG_VERBOSE,
        ANDROID_LOG_DEBUG,
        ANDROID_LOG_INFO,
        ANDROID_LOG_WARN,
        ANDROID_LOG_ERROR,
        ANDROID_LOG_FATAL,
        ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
    } android_LogPriority;
  #endif
    #include <android/log.h
    #define FIH_LOGV(fmt, args...)       __android_log_print(ANDROID_LOG_VERBOSE, "........", fmt, ## args)
    #define FIH_LOGD(fmt, args...)       __android_log_print(ANDROID_LOG_DEBUG, "........", fmt, ## args)
    #define FIH_LOGI(fmt, args...)       __android_log_print(ANDROID_LOG_INFO, "........", fmt, ## args)
    #define FIH_LOGW(fmt, args...)       __android_log_print(ANDROID_LOG_WARN, "........", fmt, ## args)
    #define FIH_LOGE(fmt, args...)       __android_log_print(ANDROID_LOG_ERROR, "........", fmt, ## args)

#elif DEBUG_PLATFORM == DEBUG_IN_LINUX
#pragma message("DEBUG_IN_LINUX")
    /* Compiles on GCC but not MSVC: */
    #define FIH_LOGD(fmt, args...)       do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)
    #define FIH_LOGI(fmt, args...)       do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)
    #define FIH_LOGW(fmt, args...)       do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)
    #define FIH_LOGE(fmt, args...)       do {printf(fmt, ## args); printf("\n"); fflush(stdout);} while (0)

#elif DEBUG_PLATFORM == DEBUG_IN_WINDOWS
#pragma message("DEBUG_IN_WINDOWS")
   // #define FIH_LOGD(fmt, args...)       do {printf(fmt, ## args); printf("\n");} while (0)
   // #define FIH_LOGI(fmt, args...)       printf(fmt, ## args) 
   // #define FIH_LOGW(fmt, args...)       printf(fmt, ## args) 
   // #define FIH_LOGE(fmt, args...)       printf(fmt, ## args) 

    #define FIH_LOGD                      printf 
    //#define FIH_LOGD(fmt, ...)            do { printf(fmt,__VA_ARGS__); printf("\n"); } while (0)
    #define FIH_LOGI(fmt, ...)            do { printf(fmt,__VA_ARGS__); printf("\n"); } while (0) 
    #define FIH_LOGW(fmt, ...)            do { printf(fmt,__VA_ARGS__); printf("\n"); } while (0)
    #define FIH_LOGE(fmt, ...)            do { printf(fmt,__VA_ARGS__); printf("\n"); } while (0)
#endif

#endif /* __DEBUG_H__ */
