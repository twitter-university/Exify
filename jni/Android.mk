LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	exif.c \
	gpsinfo.c \
	iptc.c \
	jhead.c \
	jpgfile.c \
	jpgqguess.c \
	makernote.c \
	paths.c
LOCAL_MODULE := jhead
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := com_twitter_university_exify_JHead.c
LOCAL_STATIC_LIBRARIES := jhead
LOCAL_LDLIBS += -llog
LOCAL_MODULE := jhead_jni
include $(BUILD_SHARED_LIBRARY)
