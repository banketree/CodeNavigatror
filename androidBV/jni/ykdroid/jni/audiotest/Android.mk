LOCAL_PATH:= $(YK-IDBT_ROOT)

#ifeq ($(filter-out s5pc110,$(TARGET_BOARD_PLATFORM)),)

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES:= \
#		aplay.c \
#		$(YK-IDBT_ROOT)/LAN/Audio/alsa_pcm.c \
#		$(YK-IDBT_ROOT)/LAN/Audio/alsa_mixer.c
#LOCAL_C_INCLUDES += $(YK-IDBT_ROOT)/LAN/Audio
#LOCAL_MODULE:= aplay
#LOCAL_SHARED_LIBRARIES:= libc libcutils
#LOCAL_MODULE_TAGS:= debug
#include $(BUILD_EXECUTABLE)

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES:= \
#		arec.c \
#		$(YK-IDBT_ROOT)/LAN/Audio/alsa_pcm.c \
#LOCAL_C_INCLUDES += $(YK-IDBT_ROOT)/LAN/Audio
#LOCAL_MODULE:= aplay
#LOCAL_SHARED_LIBRARIES:= libc libcutils
#LOCAL_MODULE_TAGS:= debug
#include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
		ykdroid/jni/audiotest/AudioPlayTest.c \
		LAN/Audio/AudioPlay.c \
		LAN/Audio/alsa_pcm.c 
		
LOCAL_C_INCLUDES := $(YK_C_INCLUDES)	
LOCAL_MODULE:= audiotest
LOCAL_SHARED_LIBRARIES:= libc libcutils
LOCAL_LDLIBS += -ldl -llog
LOCAL_MODULE_TAGS:= debug
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
		ykdroid/jni/audiotest/AudioPlayTest.c \
		LAN/Audio/AudioPlay.c \
		LAN/Audio/alsa_pcm.c 
		
LOCAL_C_INCLUDES := $(YK_C_INCLUDES)	
LOCAL_MODULE:= pcmplaytest
LOCAL_SHARED_LIBRARIES:= libc libcutils
LOCAL_LDLIBS += -ldl -llog
LOCAL_MODULE_TAGS:= debug
include $(BUILD_EXECUTABLE)

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES:= \
#		amix.c \
#		$(YK-IDBT_ROOT)/LAN/Audio/alsa_mixer.c
#LOCAL_C_INCLUDES += $(YK-IDBT_ROOT)/LAN/Audio
#LOCAL_MODULE:= amix
#LOCAL_SHARED_LIBRARIES := libc libcutils
#LOCAL_MODULE_TAGS:= debug
#include $(BUILD_EXECUTABLE)

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES:= \
#		AudioHardware.cpp \
#		alsa_mixer.c \
#		$(YK-IDBT_ROOT)/LAN/Audio/alsa_pcm.c
#LOCAL_C_INCLUDES += $(YK-IDBT_ROOT)/LAN/Audio
#LOCAL_MODULE:= libaudio
#LOCAL_STATIC_LIBRARIES:= libaudiointerface
#LOCAL_SHARED_LIBRARIES:= libc libcutils libutils libmedia libhardware_legacy
#ifeq ($(BOARD_HAVE_BLUETOOTH),true)
#  LOCAL_SHARED_LIBRARIES += liba2dp
#endif
#
#ifeq ($(TARGET_SIMULATOR),true)
# LOCAL_LDLIBS += -ldl
#else
# LOCAL_SHARED_LIBRARIES += libdl
#endif
#
#include $(BUILD_SHARED_LIBRARY)

#include $(CLEAR_VARS)
#LOCAL_SRC_FILES:= AudioPolicyManager.cpp
#LOCAL_C_INCLUDES += $(YK-IDBT_ROOT)/LAN/Audio
#LOCAL_MODULE:= libaudiopolicy
#LOCAL_STATIC_LIBRARIES:= libaudiopolicybase
#LOCAL_SHARED_LIBRARIES:= libc libcutils libutils libmedia
#ifeq ($(BOARD_HAVE_BLUETOOTH),true)
#  LOCAL_CFLAGS += -DWITH_A2DP
#endif
#include $(BUILD_SHARED_LIBRARY)

#endif
