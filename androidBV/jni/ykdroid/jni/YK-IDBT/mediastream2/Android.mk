## ## Android.mk -Android build script-
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

LOCAL_PATH:= $(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/src/
include $(CLEAR_VARS)


LOCAL_ARM_MODE := arm

MEDIASTREAMER2_INCLUDES := \
        $(YKDROID_ROOT)/jni/YK-IDBT/mediastream2/android \
        $(LOCAL_PATH)/../include \
        $(YK-IDBT_ROOT)/LP/INC/ \
        $(YK-IDBT_ROOT)/LP/INC/LPLibInc/ \
		$(YK-IDBT_ROOT)/SM/INC \
		$(YK-IDBT_ROOT)/YKCRT/INC		\
		$(YK-IDBT_ROOT)/IDBT/INC		\
        $(YK-IDBT_ROOT)/LP/INC/android/hardware/include \
        $(LOCAL_PATH)/../       \
        $(LOCAL_PATH)/../../oRTP \
        $(LOCAL_PATH)/../../oRTP/include \
		$(YK-IDBT_ROOT)/LOG/SRC/log4c	\
        $(YK-IDBT_ROOT)/ykdroid/submodules/externals/x264       \
        $(YK-IDBT_ROOT)/ykdroid/submodules/mfc_c110/include     \
        $(FRW_ROOT)/include     \
        $(FRW_CORE_ROOT)/include        \
		$(ANDROID_INC)/hardware/include	\
        $(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/src/android \
        $(LOCAL_PATH)/GSM/gsm/inc \
        $(YK-IDBT_ROOT)/ykdroid/submodules/externals/ffmpeg \
        $(YK-IDBT_ROOT)/ykdroid/submodules/jpeg-9 \
        $(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/include/mediastreamer2

LOCAL_MODULE := libmediastreamer2

LOCAL_SRC_FILES = \
	alaw.c                    \
	audiomixer.c              \
	audiostream.c             \
	chanadapt.c               \
	dsptools.c                \
	dtmfgen.c                 \
	dtmfinband_fixed.c        \
	dtmfinband_float.c        \
	equalizer.c               \
	eventqueue.c              \
	extdisplay.c              \
	ice.c                     \
	itc.c                     \
	kiss_fft.c                \
	kiss_fftr.c               \
	layouts.c                 \
	mire.c                    \
	mscommon.c                \
	msconf.c                  \
	msfileplayer.c            \
	msfilerec.c               \
	msfilter.c                \
	msjoin.c                  \
	msqueue.c                 \
	msresample.c              \
	msrtp.c                   \
	mssndcard.c               \
	msspeex.c                 \
	msticker.c                \
	msvideo.c                 \
	msvolume.c                \
	mswebcam.c                \
	mtu.c                     \
	pixconv.c                 \
	rfc3984.c                 \
	sizeconv.c                \
	speexec.c                 \
	tee.c                     \
	tonedetector.c            \
	ulaw.c                    \
	videostream.c             \
	msjava.c		\
	void.c			\
	msvideo_neon.c	\
	scaler.c	\
	CFimc.cpp	\
	androidfimcvideo.cpp	\
	androidvideo.cpp		\
	msandroid.cpp	\
	android/androidsound.cpp	\
	android/AudioRecord.cpp	\
	android/AudioSystem.cpp	\
	android/AudioTrack.cpp	\
	android/loader.cpp	\
	android/String8.cpp


LOCAL_SRC_FILES += \
	GSM/filter/gsm.c	\
	msx264/src/msx264.c	\
	msmfc210/src/msmfc.cpp


LOCAL_CFLAGS += $(YK_IDBT_CFLAGS)
LOCAL_CXXFLAGS += -DHAVE_PTHREADS -DCODEC_AUDIO_GSM

LOCAL_C_INCLUDES :=$(MEDIASTREAMER2_INCLUDES)


LOCAL_STATIC_LIBRARIES := \
	libortp \
	libspeex \
	libspeexdsp \
	libjpeg9

LOCAL_STATIC_LIBRARIES += cpufeatures

LOCAL_STATIC_LIBRARIES += libavcodec \
	libswscale \
	libavcore \
	libavutil

include $(BUILD_STATIC_LIBRARY)
LOCAL_ARM_MODE := arm

$(call import-module,android/cpufeatures)
