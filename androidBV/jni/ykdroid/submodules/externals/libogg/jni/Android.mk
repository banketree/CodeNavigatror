LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := src/bitwise.c src/framing.c
LOCAL_CFLAGS := -I$(LOCAL_PATH)/include

LOCAL_MODULE := libogg

LOCAL_LDLIBS := -llog


include $(BUILD_STATIC_LIBRARY)
