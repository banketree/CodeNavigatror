# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#LOCAL_MODULE_TAGS := eng

LOCAL_MODULE:=Daemon

LOCAL_SRC_FILES:= \
       Ladderclient.cpp
       

LOCAL_C_INCLUDES += \
		$(YK-IDBT_ROOT)/IDBT/INC/ \
        $(YK-IDBT_ROOT)/LP/INC/LPLibInc/ \
        $(FRW_ROOT)/include     \
        $(FRW_CORE_ROOT)/include        

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libbinder \
    libutils \
    libhardware

LOCAL_CFLAGS := -DRIL_SHLIB


LOCAL_LDLIBS:= -lcutils -lbinder -lutils -lhardware -lz -llog

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES:= \
           	LadderService.cpp \
           	../../../IDBT/SRC/IDBTCfg.c \
           	DMNM/DMNM.c	\
           	DMPM/DMPM.c	\
    		DMNM/DMNMUtils.c
    		
    		#
    		#DMNMTest.c \

           

LOCAL_C_INCLUDES += \
		$(YK-IDBT_ROOT)/IDBT/INC/ \
        $(YK-IDBT_ROOT)/LP/INC/LPLibInc/ \
        $(FRW_ROOT)/include     \
        $(FRW_CORE_ROOT)/include
        

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH) \
         $(LOCAL_PATH)/DMNM \
         $(LOCAL_PATH)/DMPM


LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libbinder \
    libutils \
    libhardware

LOCAL_CFLAGS := -DRIL_SHLIB

LOCAL_MODULE:= ladderdaemon 
LOCAL_LDLIBS:=  -lcutils -lbinder -lutils -lhardware -lz -llog

include $(BUILD_EXECUTABLE)
