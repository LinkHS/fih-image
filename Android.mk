LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := data/frontalface.xml
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES :=  $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
#LOCAL_CFLAGS += -Wno-error=non-virtual-dtor
#LOCAL_CFLAGS += -Wno-error=return-type
LOCAL_CFLAGS += -fopenmp -pthread
#LOCAL_CPPFLAGS += -fopenmp -pthread
#LOCAL_CFLAGS += -frtti -fexceptions -fno-rtti -fno-exceptions -rpath -rpath-link

#LOCAL_LDFLAGS += -O3 -fopenmp -lgcc -latomic -lgomp


LOCAL_STATIC_LIBRARIES += libopenmp

LOCAL_SHARED_LIBRARIES += liblog
LOCAL_SHARED_LIBRARIES += libutils
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libstlport 
#LOCAL_SHARED_LIBRARIES += libopenmp 

#LOCAL_SHARED_LIBRARIES += libstlport libstdc++ libc

#For using OpenCV2 or OpenCV3
LOCAL_SHARED_LIBRARIES += libopencv_calib3d
LOCAL_SHARED_LIBRARIES += libopencv_core      
LOCAL_SHARED_LIBRARIES += libopencv_features2d
LOCAL_SHARED_LIBRARIES += libopencv_flann     
LOCAL_SHARED_LIBRARIES += libopencv_highgui   
LOCAL_SHARED_LIBRARIES += libopencv_imgproc   
LOCAL_SHARED_LIBRARIES += libopencv_ml        
LOCAL_SHARED_LIBRARIES += libopencv_objdetect 
LOCAL_SHARED_LIBRARIES += libopencv_photo     
LOCAL_SHARED_LIBRARIES += libopencv_stitching 
LOCAL_SHARED_LIBRARIES += libopencv_superres  
LOCAL_SHARED_LIBRARIES += libopencv_video     
LOCAL_SHARED_LIBRARIES += libopencv_videostab 

#For using OpenCV2
#LOCAL_SHARED_LIBRARIES += libopencv_contrib  
#LOCAL_SHARED_LIBRARIES += libopencv_gpu  
#LOCAL_SHARED_LIBRARIES += libopencv_legacy    
#LOCAL_SHARED_LIBRARIES += libopencv_nonfree   
#LOCAL_SHARED_LIBRARIES += libopencv_ocl 

#For using OpenCV3
LOCAL_SHARED_LIBRARIES += libopencv_imgcodecs
LOCAL_SHARED_LIBRARIES += libopencv_shape
LOCAL_SHARED_LIBRARIES += libopencv_videoio

#For using standard libraries
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/external/include
LOCAL_C_INCLUDES += $(TOP)/bionic
LOCAL_C_INCLUDES += $(TOP)/external/stlport/stlport

#For using bsp image processing
LOCAL_C_INCLUDES += ${LOCAL_PATH}/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/3rdParty/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/common/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/modules/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/others/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/porting/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/opencv3_include/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/opencv3_include/opencv/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/opencv3_include/opencv2/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/modules/dehazing/
LOCAL_C_INCLUDES += ${LOCAL_PATH}/modules/flandmark/

#start MTK LOG
LOCAL_C_INCLUDES += $(MTKCAM_C_INCLUDES)
LOCAL_C_INCLUDES += $(TOP)/$(MTK_PATH_SOURCE)/hardware/include
#end


LOCAL_SRC_FILES := bspApiForPlatform.cpp
LOCAL_SRC_FILES += modules/cvFaceDetect.cpp
LOCAL_SRC_FILES += modules/cvSkinDetect.cpp
LOCAL_SRC_FILES += modules/cvLocalStatisticsFiltering.cpp
LOCAL_SRC_FILES += modules/VideoLSFilter.cpp
LOCAL_SRC_FILES += basic_process/cvOthers.cpp
LOCAL_SRC_FILES += basic_process/cvSharpen.cpp
LOCAL_SRC_FILES += others/ImageUtils_0.7.cpp
LOCAL_SRC_FILES += common/PreCalculatedTables.cpp
LOCAL_SRC_FILES += porting/MTK_FDManager.cpp

#for dehazing module
LOCAL_SRC_FILES += 3rdParty/dehazing/dehazing.cpp
LOCAL_SRC_FILES += 3rdParty/dehazing/functions.cpp
LOCAL_SRC_FILES += 3rdParty/dehazing/guidedfilter.cpp
LOCAL_SRC_FILES += 3rdParty/dehazing/Transmission.cpp
#end for dehazing module

#LOCAL_SRC_FILES += 3rdParty/flandmark/flandmark_detector.cpp
#LOCAL_SRC_FILES += 3rdParty/flandmark/liblbp.cpp


LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE := libimgprocess_fih
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_PRELINK_MODULE:= false
include $(BUILD_SHARED_LIBRARY)




