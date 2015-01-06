
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
	enc/src/SsbSipMfcEncAPI.c	
	#dec/src/SsbSipMfcDecAPI.c	\
	#dec/src/tile_to_linear_64x32_4x2_neon.s
	

LOCAL_MODULE := libmfc

LOCAL_PRELINK_MODULE := false

LOCAL_CFLAGS := -DUSE_FIMC_FRAME_BUFFER

#LOCAL_ARM_MODE := arm

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES := liblog

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include

include $(BUILD_STATIC_LIBRARY)

