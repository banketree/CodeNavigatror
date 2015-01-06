/*
 * DMNM.c
 *
 *  Created on: 2013-4-11
 *      Author: chensq
 */

#include "DMPM.h"
#include <stdio.h>
#include <pthread.h>
#include <android/log.h>

static char g_cPMRunning = 0;
static pthread_t g_DMPMThread_id;

void *DMPMEventHndl(void *ctx) {

	__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM DMPMEventHndl begin");




	while (g_cPMRunning) {

		sleep(30);

		system("echo `ps | busybox grep com.fsti.ladder | busybox wc -l` > /data/data/com.fsti.ladder/DMPMTemp");
		FILE *stream;
		char msg[5] = {0x00};
		stream = fopen("/data/data/com.fsti.ladder/DMPMTemp", "r");
		fgets(msg, 5, stream);
		__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM DMPMTemp count=%s", msg);
		fclose(stream);
		if(strncmp(msg, "1", 1) == 0)
		{
			__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM app alive");
		}
		else
		{
			__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM app is dead will restart");
			system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");
			system("am start -n com.fsti.ladder/com.fsti.ladder.activity.SplashActivity");

		}

	}
}

int DMPMInit(void) {
	__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM init begin");

	g_cPMRunning = 1;



	if (pthread_create(&g_DMPMThread_id, NULL, DMPMEventHndl, NULL) < 0) {
		return -1;
	}
	__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM init success");
	return 0;
}

void DMPMFini(void) {
	__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM finish begin");

	g_cPMRunning = 0;
	pthread_join(g_DMPMThread_id, NULL);
	close(g_DMPMThread_id);
	__android_log_print(ANDROID_LOG_INFO, "DMPM", "DMPM finish success");

}
