## Android.mk -Android build script-
##
##
## Copyright (C) 2010  Belledonne Communications, Grenoble, France
##
##  This program is free software; you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation; either version 2 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU Library General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##

LOCAL_PATH:= $(YK-IDBT_ROOT)
include $(CLEAR_VARS)

LOCAL_MODULE := videotest

LOCAL_ARM_MODE := arm

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true  

LOCAL_C_INCLUDES := $(YK_C_INCLUDES)
#test
LOCAL_C_INCLUDES += \
        $(YKDROID_ROOT)/jni/YK-IDBT/mediastream2/android \
        $(YK-IDBT_ROOT)/LP/INC/LPLibInc/ \
        $(YK-IDBT_ROOT)/LP/INC/android/hardware/include \
        $(YK-IDBT_ROOT)/ykdroid/submodules/externals/x264       \
        $(YK-IDBT_ROOT)/ykdroid/submodules/mfc_c110/include     \
        $(FRW_ROOT)/include     \
        $(FRW_ROOT)/include/media/stagefright/openmax \
        $(FRW_CORE_ROOT)/include        \
        $(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/src/android \
        $(YK-IDBT_ROOT)/ykdroid/submodules/externals/ffmpeg \
        $(YK-IDBT_ROOT)/ykdroid/submodules/jpeg-9 \
        $(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/include/mediastreamer2

LOCAL_SRC_FILES = \
	LP/SRC/LPSrc/mediastreamer2/src/CFimc.cpp
	
LOCAL_SRC_FILES += \
	ykdroid/jni/videotest/videotest.cpp
	
LOCAL_CFLAGS := $(YK_IDBT_CFLAGS)
LOCAL_CXXFLAGS += -DHAVE_PTHREADS

LOCAL_STATIC_LIBRARIES := \
	libpolarss	\
	liblinphone	\
	libeXosip2 \
	libosip2	\
	libmediastreamer2 \
	liboRTP \
	libspeex \
	libx264 \
	libmfc \
	libgsm \
	libDaemon	\
	libogg \
	libjpeg9

LOCAL_STATIC_LIBRARIES += \
	libavcodec \
	libswscale \
	libavcore \
	libavutil

#LOCAL_LDLIBS := -lspeex -lgsm -losip2 -leXosip2 -logg -loRTP -lmediastreamer2 -llinphone
	
LOCAL_LDLIBS += -ldl -llog -lcamera_client -lcutils -lutils -lbinder -lhardware -lsurfaceflinger_client -lui -lz -lEGL -lpixelflinger -lhardware_legacy -lwpa_client -lstagefright -ls3cjpeg
LOCAL_LDLIBS += -lmedia -lsonivox -lvorbisidec -lstagefright_amrnb_common -lstagefright_enc_common -lstagefright_avc_common -lstagefright_foundation -lstagefright_color_conversion -licuuc -lexpat

LOCAL_STATIC_LIBRARIES += cpufeatures

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)
$(call import-module,android/cpufeatures)
