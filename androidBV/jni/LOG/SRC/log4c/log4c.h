/* $Id$
 *
 * log4c.h
 *
 * Copyright 2001-2002, Meiosys (www.meiosys.com). All rights reserved.
 *
 * See the COPYING file for the terms of usage and distribution.
 */

#ifndef log4c_log4c_h
#define log4c_log4c_h
#include <log4c/version.h>
#include <log4c/init.h>
#include <log4c/rc.h>
#include <log4c/appender.h>
#include <log4c/rollingpolicy.h>
#include <log4c/category.h>
#include <log4c/layout.h>
#include <log4c/logging_event.h>
#include <log4c/priority.h>

#include "android/log.h"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "ProjectName", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "ProjectName", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "ProjectName", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , "ProjectName", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "ProjectName", __VA_ARGS__)




#endif

