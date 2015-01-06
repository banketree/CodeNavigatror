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

LOCAL_MODULE := libIDBT

LOCAL_ARM_MODE := arm

LOCAL_ALLOW_UNDEFINED_SYMBOLS := true  

LOCAL_C_INCLUDES := $(YK_C_INCLUDES)
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
        $(YK-IDBT_ROOT)/ykdroid/submodules/jpeg-9

#YKCRT
LOCAL_SRC_FILES := \
	YKCRT/SRC/YKApi.c    \
	YKCRT/SRC/YKList.c	\
	YKCRT/SRC/YKMsgQue.c	\
	YKCRT/SRC/YKTimer.c
	
###############add by xuqd############
#IDBT-JNI
LOCAL_SRC_FILES += \
	ykdroid/jni/IDBT-JNI/IDBTIntfLayer.c \
	ykdroid/jni/IDBT-JNI/IDBTJni.cpp 

#################add end##############

#SM
LOCAL_SRC_FILES += \
	SM/SRC/SMEventHndl.c	\
	SM/SRC/SMRfCard.c	\
	SM/SRC/SMSerial.c	\
	SM/SRC/SMUtil.c	
#	SM/SRC/SMXt8126SndTool.c
	

#AT
LOCAL_SRC_FILES += \
	AT/SRC/AutoTest.c

#CC
LOCAL_SRC_FILES += \
	CC/SRC/CCTask.c	\
	CC/SRC/I6CCTask.c

#NM
LOCAL_SRC_FILES += \
	NM/SRC/NetworkMonitor.c

#FTP
LOCAL_SRC_FILES += \
	FTP/SRC/FPCmd.c	\
	FTP/SRC/Ftp.c	\
	FTP/SRC/FtpCore.c
	
#SS
LOCAL_SRC_FILES += \
	SS/SRC/Snapshot.c

#IDBT
LOCAL_SRC_FILES += \
	IDBT/SRC/IDBT.c \
	IDBT/SRC/IDBTCfg.c

#LAN
#LOCAL_SRC_FILES += \
#	LAN/Video/CFimcMjpeg.cpp	\
#	LAN/SRC/arp.c	\
#	LAN/SRC/door26.c	\
#	LAN/SRC/sndtools.c	\
#	LAN/SRC/fmpeg4_avcodec.c	\
#	LAN/SRC/timer.c	\
#	LAN/SRC/talk.c	\
#	LAN/SRC/udp.c	\
#	LAN/LANIntfLayer.c  \
#	LAN/Audio/AudioRecord.c \
#	LAN/Audio/AudioPlay.c \
#	LAN/Audio/alsa_pcm.c 
#	LP/SRC/LPSrc/mediastreamer2/src/CFimc.cpp	\	

#QS
LOCAL_SRC_FILES += \
	QS/SRC/arp.c	\
	QS/SRC/door26.c	\
	QS/SRC/sndtools.c	\
	QS/SRC/fmpeg4_avcodec.c	\
	QS/SRC/timer.c	\
	QS/SRC/talk.c	\
	QS/SRC/udp.c	\
	QS/LANIntfLayer.c  \
	QS/Audio/linuxaudio.cpp 
#	QS/Video/MfcMpeg4Enc.c \
#	QS/Video/CFimcMpeg4.cpp \
#	QS/Video/CFimcMjpeg.cpp	\
#	QS/Audio/AudioRecord.c \
#	QS/Audio/AudioPlay.c \
#	QS/Audio/alsa_pcm.c 


#LOG
LOCAL_SRC_FILES += \
	LOG/SRC/LOGIntfLayer/LOGExtLayoutTypes.c	\
	LOG/SRC/LOGIntfLayer/LOGExtRollingPolicyType.c	\
	LOG/SRC/LOGIntfLayer/LOGIntfLayer.c	\
	LOG/SRC/log4c/log4c/appender.c		\
	LOG/SRC/log4c/log4c/appender_type_rollingfile.c   \
	LOG/SRC/log4c/log4c/appender_type_stream.c        \
	LOG/SRC/log4c/log4c/category.c                    \
	LOG/SRC/log4c/log4c/init.c                        \
	LOG/SRC/log4c/log4c/layout.c                      \
	LOG/SRC/log4c/log4c/layout_type_basic.c           \
	LOG/SRC/log4c/log4c/layout_type_dated.c           \
	LOG/SRC/log4c/log4c/logging_event.c               \
	LOG/SRC/log4c/log4c/priority.c                    \
	LOG/SRC/log4c/log4c/rc.c                          \
	LOG/SRC/log4c/log4c/rollingpolicy.c               \
	LOG/SRC/log4c/log4c/rollingpolicy_type_sizewin.c  \
	LOG/SRC/log4c/log4c/version.c  \
	LOG/SRC/log4c/sd/domnode.c                 \
	LOG/SRC/log4c/sd/domnode-xml.c             \
	LOG/SRC/log4c/sd/domnode-xml-parser.c      \
	LOG/SRC/log4c/sd/domnode-xml-scanner.c     \
	LOG/SRC/log4c/sd/error.c                   \
	LOG/SRC/log4c/sd/factory.c                 \
	LOG/SRC/log4c/sd/hash.c                    \
	LOG/SRC/log4c/sd/list.c                    \
	LOG/SRC/log4c/sd/malloc.c                  \
	LOG/SRC/log4c/sd/sd_xplatform.c            \
	LOG/SRC/log4c/sd/sprintf.c                 \
	LOG/SRC/log4c/sd/stack.c                   

#TR
LOCAL_SRC_FILES += \
	TM/3DES/Des.c	\
	TM/3DES/MD5.c	\
	TM/SRC/TR/SRC/TMInterface.c	\
	TM/SRC/TR/SRC/PicInterface.c \
	TM/SRC/I6/SRC/TMInterface.c	\
	TM/SRC/I6/SRC/GI6Interface.c \
	TM/sort/Sort.c \
	TM/SRC/TR/libevent/buffer.c	\
	TM/SRC/TR/libevent/epoll.c	\
	TM/SRC/TR/libevent/epoll_sub.c	\
	TM/SRC/TR/libevent/evbuffer.c	\
	TM/SRC/TR/libevent/evdns.c	\
	TM/SRC/TR/libevent/event.c	\
	TM/SRC/TR/libevent/event_tagging.c	\
	TM/SRC/TR/libevent/evrpc.c	\
	TM/SRC/TR/libevent/evutil.c	\
	TM/SRC/TR/libevent/http.c	\
	TM/SRC/TR/libevent/log.c	\
	TM/SRC/TR/libevent/poll.c	\
	TM/SRC/TR/libevent/select.c	\
	TM/SRC/TR/libevent/signal.c	\
	TM/SRC/TR/libevent/strlcpy.c	\
	TM/SRC/TR/access_list.c	\
	TM/SRC/TR/accessor.c	\
	TM/SRC/TR/add_object.c	\
	TM/SRC/TR/add_object_xml.c	\
	TM/SRC/TR/attr.c	\
	TM/SRC/TR/attr_schema.c	\
	TM/SRC/TR/attr_xml.c	\
	TM/SRC/TR/class.c	\
	TM/SRC/TR/class_xml.c	\
	TM/SRC/TR/constraint.c	\
	TM/SRC/TR/cookie.c	\
	TM/SRC/TR/cpe.c	\
	TM/SRC/TR/data_xml.c	\
	TM/SRC/TR/delete_object.c	\
	TM/SRC/TR/delete_object_xml.c	\
	TM/SRC/TR/dns.c	\
	TM/SRC/TR/dns_cache.c	\
	TM/SRC/TR/download.c	\
	TM/SRC/TR/download_xml.c	\
	TM/SRC/TR/evcpe.c	\
	TM/SRC/TR/event_list.c	\
	TM/SRC/TR/fault.c	\
	TM/SRC/TR/fault_xml.c	\
	TM/SRC/TR/get_param_attrs.c	\
	TM/SRC/TR/get_param_names.c	\
	TM/SRC/TR/get_param_names_xml.c	\
	TM/SRC/TR/get_param_values.c	\
	TM/SRC/TR/get_param_values_xml.c	\
	TM/SRC/TR/get_rpc_methods.c	\
	TM/SRC/TR/get_rpc_methods_xml.c	\
	TM/SRC/TR/handler.c	\
	TM/SRC/TR/https.c	\
	TM/SRC/TR/inform.c	\
	TM/SRC/TR/inform_xml.c	\
	TM/SRC/TR/log.c	\
	TM/SRC/TR/method.c	\
	TM/SRC/TR/method_list.c	\
	TM/SRC/TR/minixml.c	\
	TM/SRC/TR/msg.c	\
	TM/SRC/TR/msg_xml.c	\
	TM/SRC/TR/obj.c	\
	TM/SRC/TR/obj_list.c	\
	TM/SRC/TR/obj_xml.c	\
	TM/SRC/TR/param_attr_list.c	\
	TM/SRC/TR/param_info_list.c	\
	TM/SRC/TR/param_name_list.c	\
	TM/SRC/TR/param_value_list.c	\
	TM/SRC/TR/persister.c	\
	TM/SRC/TR/reboot.c	\
	TM/SRC/TR/reboot_xml.c	\
	TM/SRC/TR/repo.c	\
	TM/SRC/TR/request.c	\
	TM/SRC/TR/response.c	\
	TM/SRC/TR/session.c	\
	TM/SRC/TR/set_param_attr_list.c	\
	TM/SRC/TR/set_param_attrs.c	\
	TM/SRC/TR/set_param_value_list.c	\
	TM/SRC/TR/set_param_values.c	\
	TM/SRC/TR/set_param_values_xml.c	\
	TM/SRC/TR/transfercomplete.c	\
	TM/SRC/TR/transfercomplete_xml.c	\
	TM/SRC/TR/TRManage.c	\
	TM/SRC/TR/type.c	\
	TM/SRC/TR/upload.c	\
	TM/SRC/TR/upload_xml.c	\
	TM/SRC/TR/url.c	\
	TM/SRC/TR/util.c	\
	TM/SRC/TR/xml.c	\
	TM/SRC/TR/xmlns.c	\
	TM/SRC/TR/xml_stack.c


LOCAL_CFLAGS := $(YK_IDBT_CFLAGS)

LOCAL_SHARED_LIBRARIES := \
	libcamera_client

LOCAL_STATIC_LIBRARIES := \
	libpolarss	\
	liblinphone	\
	libeXosip2 \
	libosip2	\
	libjpeg9	\
	libmediastreamer2 \
	liboRTP \
	libspeex \
	libx264 \
	libgsm \
	libDaemon	\
	libogg 

LOCAL_STATIC_LIBRARIES += \
	libavcodec \
	libswscale \
	libavcore \
	libavutil

#LOCAL_LDLIBS := -lspeex -lgsm -losip2 -leXosip2 -logg -loRTP -lmediastreamer2 -llinphone
	
#LOCAL_LDLIBS += -ldl -llog -lcutils -lbinder -lutils -lhardware -lz -ls3cjpeg
LOCAL_LDLIBS += -ldl -llog -lcamera_client -lcutils -lutils -lbinder -lhardware -lui -lz -lEGL -lpixelflinger -lhardware_legacy -lwpa_client -lstagefright
LOCAL_LDLIBS += -lmedia -lsonivox -lstagefright_amrnb_common -lstagefright_enc_common -lstagefright_avc_common -lstagefright_foundation -licuuc -lexpat

#LOCAL_LDLIBS += -lpolarssl

LOCAL_LDFLAGS := -L$(NDK_LIB_PATH)

include $(BUILD_SHARED_LIBRARY)
#include $(BUILD_EXECUTABLE)
