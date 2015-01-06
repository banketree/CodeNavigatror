# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_MODULE_TAGS := eng

LOCAL_MODULE:=watchdog       

LOCAL_SRC_FILES:= \
    		watchdog-simple.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) 

include $(BUILD_EXECUTABLE)
