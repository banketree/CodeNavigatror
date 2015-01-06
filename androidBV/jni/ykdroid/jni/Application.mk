APP_PROJECT_PATH :=$(call my-dir)/../../..

APP_MODULES :=libIDBT liblinphone libmediastreamer2 liboRTP libeXosip2 libosip2 libspeex libgsm libx264 libmfc libpolarss libDaemon ladderdaemon watchdog libjpeg9 #mjpegtest ideotest audiotest pcmplaytest mpeg4test linuxaudiotest#aplay 
#APP_STL :=stlport_static

APP_BUILD_SCRIPT :=$(call my-dir)/Android.mk
APP_PLATFORM :=android-17
APP_ABI :=armeabi-v7a

YK-IDBT_ROOT=$(APP_PROJECT_PATH)/jni

YKDROID_ROOT=$(YK-IDBT_ROOT)/ykdroid
#FRW_ROOT=/opt/FriendlyARM/tiny210/android/android-2.3.1/frameworks/base
#FRW_CORE_ROOT=/opt/FriendlyARM/tiny210/android/android-2.3.1/system/core
ANDROID_INC=$(YK-IDBT_ROOT)/LP/INC/android
FRW_ROOT=$(YK-IDBT_ROOT)/LP/INC/android/frameworks/base
FRW_CORE_ROOT=$(YK-IDBT_ROOT)/LP/INC/android/system/core

NDK_BUILD_PATH := /usr/local/android-ndk-r10
NDK_LIB_PATH := $(NDK_BUILD_PATH)/platforms/android-17/arch-arm/usr/lib

YK_IDBT_CFLAGS := -D_YK_S5PV210_ -D_HAS_X264_ -D_BRUSHIDCARD_SUPPORT -DCODEC_AUDIO_GSM -DDTMF_INBAND_FLOAT -D_CODEC_H264_ -DTARGET_BOARD -DHAVE_CONFIG_H -DVIDEO_ENABLED -DIN_LINPHONE -DOSIP_MT -DNO_FFMPEG -DLOG_MODULE #-D_CAPTURE_PIC_ 

YK_IDBT_CFLAGS += -DTM_CONFIG_DOWNLOAD

APP_CFLAGS:=-DDISABLE_NEON 

YK_C_INCLUDES := \
    $(FRW_ROOT)/include     \
    $(FRW_CORE_ROOT)/include    \
	$(YK-IDBT_ROOT)/AT/INC		\
	$(YK-IDBT_ROOT)/SM/INC		\
	$(YK-IDBT_ROOT)/CC/INC		\
	$(YK-IDBT_ROOT)/CP/INC		\
	$(YK-IDBT_ROOT)/FTP/INC		\
	$(YK-IDBT_ROOT)/SS/INC		\
	$(YK-IDBT_ROOT)/IDBT/INC	\
	$(YK-IDBT_ROOT)/LOG/INC		\
	$(YK-IDBT_ROOT)/LOG/SRC/log4c/log4c	\
	$(YK-IDBT_ROOT)/LOG/SRC/log4c		\
	$(YK-IDBT_ROOT)/NM/INC			\
	$(YK-IDBT_ROOT)/TM/INC			\
	$(YK-IDBT_ROOT)/TM/3DES			\
	$(YK-IDBT_ROOT)/TM/sort			\
	$(YK-IDBT_ROOT)/TM/SRC/I6/INC		\
	$(YK-IDBT_ROOT)/TM/SRC/YK/INC		\
	$(YK-IDBT_ROOT)/TM/SRC/TR		\
	$(YK-IDBT_ROOT)/TM/SRC/TR/INC		\
	$(YK-IDBT_ROOT)/TM/SRC/TR/compat	\
	$(YK-IDBT_ROOT)/TM/SRC/TR/libevent 	\
	$(YK-IDBT_ROOT)/TM/SRC/TR/polarss 	\
	$(YK-IDBT_ROOT)/TM/SRC/TR/polarssl/include \
	$(YK-IDBT_ROOT)/TS/INC			\
	$(YK-IDBT_ROOT)/YKCRT/INC		\
	$(YK-IDBT_ROOT)/QS			\
	$(YK-IDBT_ROOT)/QS/Audio 	\
	$(YK-IDBT_ROOT)/QS/Video 	\
	$(YK-IDBT_ROOT)/QS/INC		\
	$(YK-IDBT_ROOT)/LP/INC		\
	$(YK-IDBT_ROOT)/LP/INC/LPLibInc	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/console	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/coreapi	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/oRTP	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/oRTP/include	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/oRTP/src	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/include	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/src	\
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/src/GSM/gsm/inc \
	$(YK-IDBT_ROOT)/LP/SRC/LPSrc/mediastreamer2/include/mediastreamer2 \
	$(YK-IDBT_ROOT)/ykdroid/jni/IDBT-JNI	\
	$(YK-IDBT_ROOT)/ykdroid/jni/daemon 
	#/usr/local/arm/4.4.0/arm-none-linux-gnueabi/include	
	
