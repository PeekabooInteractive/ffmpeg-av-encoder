LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE:= libavcodec

LOCAL_SRC_FILES:= lib/libavcodec.a

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_STATIC_LIBRARY)


 

include $(CLEAR_VARS)

LOCAL_MODULE:= libavformat

LOCAL_SRC_FILES:= lib/libavformat.a

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_STATIC_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libswscale

LOCAL_SRC_FILES:= lib/libswscale.a

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_STATIC_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libavutil

LOCAL_SRC_FILES:= lib/libavutil.a

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_STATIC_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libavfilter

LOCAL_SRC_FILES:= lib/libavfilter.a

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_STATIC_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libswresample

LOCAL_SRC_FILES:= lib/libswresample.a

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_STATIC_LIBRARY)


#PREBUILT_SHARED_LIBRARY
#PREBUILT_STATIC_LIBRARY
#BUILD_STATIC_LIBRARY

include $(CLEAR_VARS)

LOCAL_MODULE    := VideoEncoder
LOCAL_SRC_FILES := VideoEncoder.c
LOCAL_LDLIBS := -llog -ljnigraphics -lz -lm  -lGLESv1_CM
LOCAL_SHARED_LIBRARIES := 
LOCAL_STATIC_LIBRARIES := libavformat libavcodec libswscale libavutil libswresample libavfilter

include $(BUILD_SHARED_LIBRARY)




include $(CLEAR_VARS)

LOCAL_MODULE    := ImagesGenerator
LOCAL_SRC_FILES := ImagesGenerator.c
LOCAL_LDLIBS := -llog -lz -lm -lGLESv1_CM
LOCAL_SHARED_LIBRARIES := 
LOCAL_STATIC_LIBRARIES :=

include $(BUILD_SHARED_LIBRARY)

#$(call import-module,ffmpeg-2.2/android/arm)
