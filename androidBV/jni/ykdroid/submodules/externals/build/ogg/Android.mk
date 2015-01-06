
LOCAL_PATH:= $(call my-dir)/../../libogg
include $(CLEAR_VARS)

LOCAL_MODULE:= libogg

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := \
                src/bitwise.c     \
                src/framing.c


#LOCAL_CFLAGS += \

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)

