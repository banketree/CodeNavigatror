#default valus

#polarssl
include $(YKDROID_ROOT)/submodules/externals/build/polarss/Android.mk

#libIDBT
include $(YKDROID_ROOT)/jni/YK-IDBT/Android.mk

#linphone
include $(YKDROID_ROOT)/jni/YK-IDBT/LP/Android.mk

#mediastream2
include $(YKDROID_ROOT)/jni/YK-IDBT/mediastream2/Android.mk

#oRTP
include $(YKDROID_ROOT)/jni/YK-IDBT/ortp/Android.mk

#eXosip
include $(YKDROID_ROOT)/submodules/externals/build/exosip/Android.mk

#osip
include $(YKDROID_ROOT)/submodules/externals/build/osip/Android.mk

#speex
include $(YKDROID_ROOT)/submodules/externals/build/speex/Android.mk

#Gsm
include $(YKDROID_ROOT)/submodules/externals/build/gsm/Android.mk


#x264 plubin for mediastream2
#include $(YKDROID_ROOT)/submodules/msx264/Android.mk
include $(YKDROID_ROOT)/submodules/externals/build/x264/Android.mk

#samsung s5pv210 mfc driver h264 codec plubin
include $(YKDROID_ROOT)/submodules/mfc_c110/Android.mk

#g.729
include $(YKDROID_ROOT)/submodules/bcg729/Android.mk
include $(YKDROID_ROOT)/submodules/bcg729/msbcg729/Android.mk

#libjpeg9
include $(YKDROID_ROOT)/submodules/jpeg-9/Android.mk

#ogg
include $(YKDROID_ROOT)/submodules/externals/build/ogg/Android.mk

#ffmpeg
include $(YKDROID_ROOT)/submodules/externals/build/ffmpeg/Android.mk

#Daemon
include $(YKDROID_ROOT)/jni/daemon/Android.mk

#Watchdog
include $(YKDROID_ROOT)/jni/watchdog/Android.mk

#for video test
include $(YKDROID_ROOT)/jni/videotest/Android.mk

#for mjpeg video test
include $(YKDROID_ROOT)/jni/mjpegtest/Android.mk

#for audio test
include $(YKDROID_ROOT)/jni/audiotest/Android.mk

#for mpeg4 test
include $(YKDROID_ROOT)/jni/mpeg4test/Android.mk

#for linux audio test
include $(YKDROID_ROOT)/jni/linuxaudiotest/Android.mk