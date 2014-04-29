LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE:= libavcodec

LOCAL_SRC_FILES:= lib/libavcodec-55.so

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_SHARED_LIBRARY)


 

include $(CLEAR_VARS)

LOCAL_MODULE:= libavformat

LOCAL_SRC_FILES:= lib/libavformat-55.so

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_SHARED_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libswscale

LOCAL_SRC_FILES:= lib/libswscale-2.so

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_SHARED_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libavutil

LOCAL_SRC_FILES:= lib/libavutil-52.so

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_SHARED_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libavfilter

LOCAL_SRC_FILES:= lib/libavfilter-4.so

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_SHARED_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_MODULE:= libwsresample

LOCAL_SRC_FILES:= lib/libswresample-0.so

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_SHARED_LIBRARY)





include $(CLEAR_VARS)

LOCAL_MODULE    := VideoEncoder
LOCAL_SRC_FILES := VideoEncoder.c
LOCAL_LDLIBS := -llog -ljnigraphics -lz -lm
LOCAL_SHARED_LIBRARIES := libavformat libavcodec libswscale libavutil libwsresample

include $(BUILD_SHARED_LIBRARY)
#$(call import-module,ffmpeg-2.2/android/arm)
