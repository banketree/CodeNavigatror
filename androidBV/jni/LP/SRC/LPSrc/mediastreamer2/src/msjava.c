/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2010  Belledonne Communications SARL 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "utils/Log.h"
#include "mediastreamer2/msjava.h"
#include "mediastreamer2/mscommon.h"

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "AUDIO", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "AUDIO", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , "AUDIO", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , "AUDIO", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , "AUDIO", __VA_ARGS__)

static JavaVM *ms2_vm=NULL;

#ifndef WIN32
#include <pthread.h>

static pthread_key_t jnienv_key;

void _android_key_cleanup(void *data){
	ms_message("Thread end, detaching jvm from current thread");
	JNIEnv* env=(JNIEnv*)pthread_getspecific(jnienv_key);
	if (env != NULL) {
		(*ms2_vm)->DetachCurrentThread(ms2_vm);
		pthread_setspecific(jnienv_key,NULL);
	}
}
#endif

int set_thread_priority(int prio) {
    int err = 0;
    JNIEnv* env;
    jclass clz;
    jmethodID mid;
    jobject thread;

	JavaVM *jvm = ms2_vm;
    if (!jvm)
        return -1;
    err = (*jvm)->AttachCurrentThread(jvm, &env, 0);
    if (err < 0) {
        err = -1;
        goto done;
    }
    clz = (*env)->FindClass(env, "java/lang/Thread");
    if (!clz) {
        err = -1;
        goto done;
    }
    mid = (*env)->GetStaticMethodID(env, clz, "currentThread", "()Ljava/lang/Thread;");
    if (!mid) {
        err = -1;
        goto done;
    }
    thread = (*env)->CallStaticObjectMethod(env, clz, mid);
    if (!thread) {
        err = -1;
        goto done;
    }
    mid = (*env)->GetMethodID(env, clz, "setPriority", "(I)V");
    if (!mid) {
        err = -1;
        goto done;
    }
    (*env)->CallVoidMethod(env, thread, mid, prio);
    mid = (*env)->GetMethodID(env, clz, "getPriority", "()I");
    if (mid) {
        prio = (*env)->CallIntMethod(env, thread, mid);
        printf("new thread priority is %d\n", prio);
    }
done:
    (*jvm)->DetachCurrentThread(jvm);
    return err;
}

void ms_set_jvm(JavaVM *vm){
	ms2_vm=vm;
#ifndef WIN32
	pthread_key_create(&jnienv_key,_android_key_cleanup);
#endif
}

JavaVM *ms_get_jvm(void){
	return ms2_vm;
}

JNIEnv *ms_get_jni_env(void){
	JNIEnv *env=NULL;

	if (ms2_vm==NULL){
		LOGD("wangle file =%s,line=%d\n",__FILE__,__LINE__);
		LOGD("Calling ms_get_jni_env() while no jvm has been set using ms_set_jvm().");

	}else{
#ifndef WIN32
		env=(JNIEnv*)pthread_getspecific(jnienv_key);
		if (env==NULL){
			if ((*ms2_vm)->AttachCurrentThread(ms2_vm,&env,NULL)!=0){
				LOGD("AttachCurrentThread() failed !");
				return NULL;
			}
			pthread_setspecific(jnienv_key,env);
		}
#else
		LOGD("ms_get_jni_env() not implemented on windows.");
#endif
	}
	return env;
}



