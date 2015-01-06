##
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
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program; if not, write to the Free Software
##  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
##

LOCAL_PATH:= $(YK-IDBT_ROOT)/LP/SRC/


include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm

LOCAL_CPP_EXTENSION := .cc

LOCAL_C_INCLUDES := $(YK_C_INCLUDES)
	

LOCAL_SRC_FILES := \
	LP.c	\
	LPUtils/LPUtils.c	\
	LPIntfLayer/LPIntfLayer.c	\
	LPIntfLayer/LPMsgOp.c	\
	LPSrc/console/commands.c \
	LPSrc/console/linphonec.c	\
	LPSrc/coreapi/address.c                   \
	LPSrc/coreapi/authentication.c            \
	LPSrc/coreapi/callbacks.c                 \
	LPSrc/coreapi/chat.c                      \
	LPSrc/coreapi/ec-calibrator.c             \
	LPSrc/coreapi/enum.c                      \
	LPSrc/coreapi/friend.c                    \
	LPSrc/coreapi/linphonecall.c              \
	LPSrc/coreapi/linphonecore.c              \
	LPSrc/coreapi/lpconfig.c                  \
	LPSrc/coreapi/lsd.c                       \
	LPSrc/coreapi/misc.c                      \
	LPSrc/coreapi/offeranswer.c               \
	LPSrc/coreapi/presence.c                  \
	LPSrc/coreapi/proxy.c                     \
	LPSrc/coreapi/sal.c                       \
	LPSrc/coreapi/sal_eXosip2.c               \
	LPSrc/coreapi/sal_eXosip2_presence.c      \
	LPSrc/coreapi/sal_eXosip2_sdp.c           \
	LPSrc/coreapi/siplogin.c                  \
	LPSrc/coreapi/sipsetup.c                  
	
#ifeq ($(LINPHONE_VIDEO),1)
LOCAL_SHARED_LIBRARIES += \
	libavcodec \
	libswscale \
	libavcore \
	libavutil
#endif	

LOCAL_MODULE := liblinphone
LOCAL_CFLAGS := $(YK_IDBT_CFLAGS)

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_STATIC_LIBRARY)

$(call import-module,android/cpufeatures)
