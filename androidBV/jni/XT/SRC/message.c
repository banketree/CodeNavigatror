
#include <pthread.h>
#include <semaphore.h>       //sem_t
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "message.h"

#define QS_SYNCMSG				0x10000000
#define QS_NOTIFYMSG			0x20000000
#define QS_POSTMSG				0x40000000
#define QS_QUITMSG				0x80000000
#define QS_PAINTMSG			0x01000000

#define QS_TIMER				0x0000FFFF
#define QS_EMPTY				0x00000000

#define CIF_W    352
#define CIF_H    240
#define WINDOWMODE 1
#define FULLSCREEN 0

int message_flag;
pthread_t message_thread;
void message_thread_func(void);

PMsgQueue pMsgQueue;
//---------------------------------------------------------------------------
BOOL InitMsgQueue(void)
{
   	pMsgQueue = (PMsgQueue)malloc(sizeof(struct _MsgQueue));
	if(!(pMsgQueue))
		return false;
	memset(pMsgQueue,0,sizeof(MsgQueue));
	pMsgQueue->dwState = QS_EMPTY;
	pthread_mutex_init(&pMsgQueue->mutex,NULL);
	sem_init(&pMsgQueue->sem,0,0);

        pMsgQueue->pHeadSyncMsg = NULL;
        pMsgQueue->pTailSyncMsg = NULL;

        pMsgQueue->pHeadNtfMsg = NULL;
	pMsgQueue->pTailNtfMsg = NULL;

	pMsgQueue->pHeadPntMsg = NULL;
	pMsgQueue->pTailPntMsg = NULL;

        //printf("InitMsgQueue :  pMsgQueue->pHeadSyncMsg = 0x%08X, 0x%08X\n", pMsgQueue->pHeadSyncMsg, pMsgQueue->pTailSyncMsg);

        pthread_attr_t attr;
        message_flag = 1;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&message_thread,&attr,(void *)message_thread_func, NULL);
        pthread_attr_destroy(&attr);
        if ( message_thread == 0 ) {
            printf("无法创建消息线程\n");
            return -1;
         }

	return true;
}
//---------------------------------------------------------------------------
void DestroyMsgQueue(void)
{
	PNtfMsgLink pNext, pHead;

        PSyncMsgLink pSyncNext, pSyncHead;
	pSyncNext = pSyncHead = pMsgQueue->pHeadSyncMsg;
	while(pSyncHead){
		pSyncNext = pSyncHead->pNext;
		free(pSyncHead);
		pSyncHead = pSyncNext;
	}

	pNext = pHead = pMsgQueue->pHeadNtfMsg;
	while(pHead){
		pNext = pHead->pNext;
		free(pHead);
		pHead = pNext;
	}
	pNext = pHead = pMsgQueue->pHeadPntMsg;
	while(pHead){
		pNext = pHead->pNext;
		free(pHead);
		pHead = pNext;
	}

        message_flag = 0;
        usleep(40*1000);
        pthread_cancel(message_thread);

	sem_destroy(&pMsgQueue->sem);
        pthread_mutex_destroy(&pMsgQueue->mutex);
	free(pMsgQueue);
}
//---------------------------------------------------------------------------
void message_thread_func(void)
{
  int i;
  MSG msg;
  printf("创建消息线程\n" );

  while(message_flag == 1)
   {
    if(GetMessage(&msg))
     {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
     }
    sem_wait (&pMsgQueue->sem);
   }
}
//---------------------------------------------------------------------------
BOOL GetMessage(PMSG pMsg)
{
	int slot;
	PNtfMsgLink pHead;

	memset(pMsg,0,sizeof(MSG));

	if(pMsgQueue->dwState & QS_QUITMSG){
		pMsg->message =LMSG_QUIT;
		pMsg->wParam = pMsg->lParam = 0;
                printf("exit................................................................\n");
		//ExitWindow(hWnd);
		return 0;
	}

	if (pMsgQueue->dwState & QS_SYNCMSG) {
		pthread_mutex_lock (&pMsgQueue->mutex);
		if (pMsgQueue->pHeadSyncMsg) {
			memcpy(pMsg,&pMsgQueue->pHeadSyncMsg->msg,sizeof(MSG));
			pMsg->pData = (void*)(pMsgQueue->pHeadSyncMsg);
			pMsgQueue->pHeadSyncMsg = pMsgQueue->pHeadSyncMsg->pNext;

			pthread_mutex_unlock (&pMsgQueue->mutex);
			return 1;
		}
		else
			pMsgQueue->dwState &= ~QS_SYNCMSG;

		pthread_mutex_unlock (&pMsgQueue->mutex);
	}

	if (pMsgQueue->dwState & QS_NOTIFYMSG) {
		pthread_mutex_lock (&pMsgQueue->mutex);
		if (pMsgQueue->pHeadNtfMsg) {
			pHead = pMsgQueue->pHeadNtfMsg;
			pMsgQueue->pHeadNtfMsg = pHead->pNext;
			memcpy(pMsg,&pHead->msg,sizeof(MSG));
			free((void*)pHead);
			pthread_mutex_unlock (&pMsgQueue->mutex);
			return 1;
		}
		else{
			pMsgQueue->dwState &= ~QS_NOTIFYMSG;
		}
		pthread_mutex_unlock (&pMsgQueue->mutex);
	}
	if(pMsgQueue->dwState &QS_POSTMSG){
		pthread_mutex_lock (&pMsgQueue->mutex);
		if(pMsgQueue->wndMailBox.iReadPos != pMsgQueue->wndMailBox.iWritePos){
			memcpy(pMsg,
				&pMsgQueue->wndMailBox.msg[pMsgQueue->wndMailBox.iReadPos],
				sizeof(MSG));
			pMsgQueue->wndMailBox.iReadPos ++;
			
			if (pMsgQueue->wndMailBox.iReadPos == SIZE_WND_MAILBOX) 
				pMsgQueue->wndMailBox.iReadPos = 0;
			pthread_mutex_unlock (&pMsgQueue->mutex);
			//printf("%d,%d\n",pMsgQueue->wndMailBox.iReadPos,pMsgQueue->wndMailBox.iWritePos);
			return 1;
		}
		else{
			pMsgQueue->dwState &= ~QS_POSTMSG;
		}
		pthread_mutex_unlock (&pMsgQueue->mutex);
	}

	if (pMsgQueue->dwState & QS_PAINTMSG) {
		pthread_mutex_lock (&pMsgQueue->mutex);
		if (pMsgQueue->pHeadPntMsg) {
			pHead = pMsgQueue->pHeadPntMsg;
			pMsgQueue->pHeadPntMsg = pHead->pNext;
			memcpy(pMsg,&pHead->msg,sizeof(MSG));
			free((void*)pHead);
			pthread_mutex_unlock (&pMsgQueue->mutex);
			return 1;
		}
		else{
			pMsgQueue->dwState &= ~QS_PAINTMSG;
		}
		pthread_mutex_unlock (&pMsgQueue->mutex);

	}

	return 1;
}

//---------------------------------------------------------------------------
BOOL PostMessage(int iMsg, WPARAM wParam, LPARAM lParam)
{
	PMSG pMsg;
	int sem_value;;

	if(!pMsgQueue)
		return false;
	pthread_mutex_lock(&pMsgQueue->mutex);
	if(iMsg == LMSG_QUIT){
		pMsgQueue->dwState |= QS_QUITMSG;
	}
	else{
		if((pMsgQueue->wndMailBox.iWritePos + 1) % SIZE_WND_MAILBOX
			== pMsgQueue->wndMailBox.iReadPos)
			return false;
		
		pMsg = &(pMsgQueue->wndMailBox.msg[pMsgQueue->wndMailBox.iWritePos]);
		pMsg->message = iMsg;
		pMsg->wParam = wParam;
		pMsg->lParam = lParam;
		
		pMsgQueue->wndMailBox.iWritePos ++;
		if(pMsgQueue->wndMailBox.iWritePos >= SIZE_WND_MAILBOX)
			pMsgQueue->wndMailBox.iWritePos = 0;
	
		pMsgQueue->dwState |= QS_POSTMSG;
	}

	pthread_mutex_unlock (&pMsgQueue->mutex);
	sem_getvalue (&pMsgQueue->sem, &sem_value);
	if (sem_value == 0)
		sem_post(&pMsgQueue->sem);       
	return true;
}

//---------------------------------------------------------------------------
BOOL PostQuitMessage(void)
{
      	return PostMessage(LMSG_QUIT,(WPARAM)NULL,(LPARAM)NULL);
}

//---------------------------------------------------------------------------
int SendMessage(int iMsg, WPARAM wParam, LPARAM lParam)
{
        return PostSyncMessage(iMsg, wParam, lParam);
}

//---------------------------------------------------------------------------
BOOL SendNotifyMessage(int iMsg, WPARAM wParam, LPARAM lParam)
{
	PNtfMsgLink pLinkNode;
	int sem_value;

	if(!pMsgQueue)
		return false;

	pLinkNode = (PNtfMsgLink) malloc(sizeof(NtfMsgLink));

	pthread_mutex_lock(&pMsgQueue->mutex);
	pLinkNode->msg.message = iMsg;
	pLinkNode->msg.wParam = wParam;
	pLinkNode->msg.lParam = lParam;
	pLinkNode->msg.pData = NULL;
	pLinkNode->pNext = NULL;

	if(!pMsgQueue->pHeadNtfMsg){
		pMsgQueue->pHeadNtfMsg = pMsgQueue->pTailNtfMsg = pLinkNode;
	}
	else{
		pMsgQueue->pTailNtfMsg->pNext = pLinkNode;
		pMsgQueue->pTailNtfMsg = pLinkNode;
	}
	pMsgQueue->dwState |= QS_NOTIFYMSG;

	pthread_mutex_unlock (&pMsgQueue->mutex);
	sem_getvalue (&pMsgQueue->sem, &sem_value);
	if (sem_value == 0)
		sem_post(&pMsgQueue->sem);
	return true;
}
//---------------------------------------------------------------------------
BOOL SendPaintMessage(void)
{
	PNtfMsgLink pLinkNode;
	int sem_value;


	if(!pMsgQueue)
		return false;
        pLinkNode = (PNtfMsgLink) malloc(sizeof(NtfMsgLink));
	
	pthread_mutex_lock(&pMsgQueue->mutex);
	pLinkNode->msg.message = LMSG_PAINT;
	pLinkNode->msg.wParam = (WPARAM)NULL;
	pLinkNode->msg.lParam = (LPARAM)NULL;
	pLinkNode->msg.pData = NULL;
	pLinkNode->pNext = NULL;

	if(!pMsgQueue->pHeadPntMsg){
		pMsgQueue->pHeadPntMsg = pMsgQueue->pTailPntMsg = pLinkNode;
	}
	else{
		pMsgQueue->pTailPntMsg->pNext = pLinkNode;
		pMsgQueue->pTailPntMsg = pLinkNode;
	}

	pMsgQueue->dwState |= QS_PAINTMSG;

	pthread_mutex_unlock (&pMsgQueue->mutex);
	sem_getvalue (&pMsgQueue->sem, &sem_value);
	if (sem_value == 0)
		sem_post(&pMsgQueue->sem);     
	return true;
}
//---------------------------------------------------------------------------
BOOL TranslateMessage(PMSG pMsg)
{
	return true;
}

//---------------------------------------------------------------------------
int PostSyncMessage(int msg, WPARAM wParam, LPARAM lParam)
{
	SyncMsgLink	syncMsgLink;
	int sem_value;

	if(!pMsgQueue)
		return false;

	pthread_mutex_lock(&pMsgQueue->mutex);


	syncMsgLink.msg.message = msg;
	syncMsgLink.msg.wParam = wParam;
	syncMsgLink.msg.lParam = lParam;
	syncMsgLink.pNext = NULL;
	sem_init(&syncMsgLink.sem,0,0);
        //printf("PostSyncMessage 2  pMsgQueue->pHeadSyncMsg = 0x%08X, 0x%08X\n", pMsgQueue->pHeadSyncMsg, pMsgQueue->pTailSyncMsg);

	if(pMsgQueue->pHeadSyncMsg == NULL){
		pMsgQueue->pHeadSyncMsg = pMsgQueue->pTailSyncMsg = &syncMsgLink;
	}
	else{
		pMsgQueue->pTailSyncMsg->pNext = &syncMsgLink;
		pMsgQueue->pTailSyncMsg = &syncMsgLink;
	}
    pMsgQueue->dwState |= QS_SYNCMSG;

    pthread_mutex_unlock (&pMsgQueue->mutex);

    // Signal that the msg queue contains one more element for reading
    sem_getvalue (&pMsgQueue->sem, &sem_value);
    if (sem_value == 0)
        sem_post(&pMsgQueue->sem);
    sem_wait(&syncMsgLink.sem);
    sem_destroy(&syncMsgLink.sem);
    return syncMsgLink.iRetValue;
}
//---------------------------------------------------------------------------
int DispatchMessage(PMSG pMsg)
{
  PSyncMsgLink pSyncMsgLink;
  switch(pMsg->message)
   {
    case LMSG_PAINT:
                    break;
    case LMSG_RECVIDEO:      //对讲视频
	            switch(pMsg->wParam)
                     {
                      case CIF_WINDOW:
                             printf("DispatchMessage::LMSG_RECVIDEO 1\n");
                             StartRecVideo(CIF_W, CIF_H, WINDOWMODE);
                             break;
                      case D1_WINDOW:
                             printf("DispatchMessage::LMSG_RECVIDEO 2\n");
                             StartRecVideo(CIF_W, CIF_H, FULLSCREEN);
                             break;
                      case STOP_REC:
                             printf("DispatchMessage::LMSG_RECVIDEO 3\n");
                             StopRecVideo();
                             break;
                     }
                    pSyncMsgLink = (PSyncMsgLink)pMsg->pData; 
                    sem_post(&pSyncMsgLink->sem);
                    break;
    case LMSG_PLAYAUDIO:      //对讲音频
	            switch(pMsg->wParam)
                     {
                      case START_REC_AUDIO:
                             printf("DispatchMessage::LMSG_PLAYAUDIO StartRecAudio\n");
                             StartRecAudio();
                             break;
                      case STOP_REC_AUDIO:
                             printf("DispatchMessage::LMSG_PLAYAUDIO StopRecAudio\n");
                             StopRecAudio();
                             break;
                      case START_PLAY_AUDIO:
                             printf("DispatchMessage::LMSG_PLAYAUDIO StartPlayAudio\n");
                             StartPlayAudio();
                             break;
                      case STOP_PLAY_AUDIO:
                             printf("DispatchMessage::LMSG_PLAYAUDIO StopPlayAudio\n");
                             StopPlayAudio();
                             break;
                     }
                    pSyncMsgLink = (PSyncMsgLink)pMsg->pData;
                    sem_post(&pSyncMsgLink->sem);
                    break;
    case LMSG_SHOW_WINDOW:        //显示窗口
	            switch(pMsg->wParam)
                     {
                      default:
                             break;
                     }
                    pSyncMsgLink = (PSyncMsgLink)pMsg->pData;
                    sem_post(&pSyncMsgLink->sem);
                    break;
   }

  return 1;
}
//---------------------------------------------------------------------------






