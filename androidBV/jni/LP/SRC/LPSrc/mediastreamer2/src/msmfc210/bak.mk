LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := libmsmfc

LOCAL_MODULE_TAGS := optional


LOCAL_SRC_FILES
	src/msmfc.c	\	
	mfc_c110/enc/src/SsbSipMfcEncAPI.c


LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../linphone/oRTP/include \
	$(LOCAL_PATH)/../linphone/mediastreamer2/include \
	$(LOCAL_PATH)/mfc_c110/include

LOCAL_CFLAGS += -DVERSION=\"android\"

include $(BUILD_STATIC_LIBRARY)


