#include "../INC/YKApi.h"

YKHandle    YKCreateThread(YKThreadCallBackFunc Func, void* pvParam)
{
#if _YK_OS_==_YK_OS_WIN32_
    return (YKHandle)_beginthreadex( NULL, 0, Func, pvParam, 0, NULL);
#else
    pthread_t thread_id;
    if(pthread_create(&thread_id, NULL, Func, pvParam) < 0)
    {
        return NULL;
    }
    return (YKHandle)thread_id;
#endif
}

void        YKJoinThread(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    WaitForSingleObject(hHandle, INFINITE);
#else
    pthread_join((pthread_t)hHandle, NULL);
#endif
}

void        YKDestroyThread(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    CloseHandle(hHandle);
#else
    close((int)hHandle);
#endif
}

YKHandle    YKCreateMutex(void*pvAttr)
{
#if _YK_OS_==_YK_OS_WIN32_
    return (YKHandle)CreateMutex(NULL, FALSE, NULL);
#else
    pthread_mutex_t* mutex = YKNew0(pthread_mutex_t, 1);
	if(pvAttr != NULL)
    {
    	pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_settype(&mattr,pvAttr);
		pthread_mutex_init(mutex, &mattr);
		pthread_mutexattr_destroy(&mattr);
		return (YKHandle)mutex;
    }
    pthread_mutex_init(mutex, NULL);
    return (YKHandle)mutex;
#endif
}

void        YKLockMutex(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    WaitForSingleObject(hHandle, INFINITE); /* == WAIT_TIMEOUT; */
#else
    pthread_mutex_lock((pthread_mutex_t*)hHandle);
#endif
}

void        YKUnLockMutex(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    ReleaseMutex(hHandle);
#else
    pthread_mutex_unlock((pthread_mutex_t*)hHandle);
#endif
}

void        YKDestroyMutex(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
    CloseHandle(hHandle);
#else
    pthread_mutex_destroy((pthread_mutex_t*)hHandle);
    YKFree(hHandle);
#endif
}

YKHandle    YKCreateSemaphore(YKLong lMaxCount)
{
#if _YK_OS_==_YK_OS_WIN32_
	return (YKHandle)CreateSemaphore(NULL , lMaxCount, lMaxCount,NULL);
#else
	sem_t* pstSem = YKNew0(sem_t, 1);
	sem_init(pstSem,0, lMaxCount);
	return pstSem;
#endif
}

int        YKWaitSemaphore(YKHandle hHandle, YKLong lTimeOut)
{
#if _YK_OS_==_YK_OS_WIN32_
	return WaitForSingleObject(hHandle, lTimeOut<=0?INFINITE:lTimeOut); /* == WAIT_TIMEOUT; */
#else
	int iRst = -1;
	if(lTimeOut <= 0)
	{
		return sem_wait((sem_t *)hHandle);
	}
	while(lTimeOut>0)
	{
        if((iRst=sem_trywait((sem_t *)hHandle))==0)
        {
            break;
        }
        lTimeOut-=10;
		YKSleep(10);
	}
	return iRst;
#endif
}

void        YKPostSemaphore(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
	ReleaseSemaphore(hHandle,1,NULL);
#else
	sem_post((sem_t *)hHandle);
#endif
}

void        YKDestroySemaphore(YKHandle hHandle)
{
#if _YK_OS_==_YK_OS_WIN32_
	CloseHandle(hHandle);
#else
	sem_destroy((sem_t*)hHandle);
	YKFree(hHandle);
#endif
}

void		YKGetCurrtime(YK_CURTIME_ST *pstCurtime )
{
	time_t timer;
	struct tm *pstTime;
	timer = time(NULL);
	pstTime = localtime(&timer);
	pstCurtime->iHour = pstTime->tm_hour;
	pstCurtime->iMin = pstTime->tm_min;
	pstCurtime->iSec = pstTime->tm_sec;
	pstCurtime->iMday = pstTime->tm_mday;
	pstCurtime->iMon = pstTime->tm_mon;
	pstCurtime->iYear = pstTime->tm_year;
	pstCurtime->iWday = pstTime->tm_wday;
	pstCurtime->iYday = pstTime->tm_yday;
	pstCurtime->iIsdst = pstTime->tm_isdst;
}

void        YKGetTimeOfDay(YK_TIME_VAL_ST *pstTv)
{
#if _YK_OS_==_YK_OS_WIN32_
    DWORD timemillis = GetTickCount();
    pstTv->lSeconds  = timemillis/1000;
    pstTv->lMilliSeconds = (timemillis - (pstTv->lSeconds*1000)) * 1000;
#else
    struct timeval time;
    gettimeofday(&time, NULL);
    pstTv->lSeconds=time.tv_sec;
    pstTv->lMilliSeconds=time.tv_usec;
#endif
}

YKLong      YKGetTickCount()
{
#if _YK_OS_==_YK_OS_WIN32_
    return GetTickCount();
#else
    struct timeval pstTv;
    gettimeofday(&pstTv, NULL);
    return (pstTv.tv_sec*1000000LL) + (pstTv.tv_usec);
#endif
}

void        YKSleep(YKLong lMillSeconds)
{
#if _YK_OS_==_YK_OS_WIN32_
    Sleep(lMillSeconds);
#else
    usleep(lMillSeconds * 1000);
#endif
}

typedef struct _YKDirectory{
    YKHandle hFolder;
    YKHandle hFindData;
}YK_DIRECTORY_ST;

YKHandle    YKFindFirstFile(const char* pcDirectory)
{
    YK_DIRECTORY_ST* pstDir=YKNew0(YK_DIRECTORY_ST, 1);
#if _YK_OS_==_YK_OS_WIN32_
    char acFind[256]={0x0};
    WIN32_FIND_DATAA* findData=NULL;
    strcpy(acFind,pcDirectory);
    strcat(acFind,"*.*");
    if(pstDir)
    {
        pstDir->hFindData=findData=YKNew0(WIN32_FIND_DATAA, 1);
        pstDir->hFolder=FindFirstFileA(acFind, findData);
        if(INVALID_HANDLE_VALUE == pstDir->hFolder)
        {
            YKFree(pstDir->hFindData);
            YKFree(pstDir);
            pstDir=NULL;
        }
    }
#else
    pstDir->hFolder = (YKHandle)opendir(pcDirectory);
    if(NULL==pstDir->hFolder || NULL==(pstDir->hFindData= (YKHandle)readdir(pstDir->hFolder)))
    {
        if(pstDir->hFolder) closedir(pstDir->hFolder);
        YKFree(pstDir);
        pstDir=NULL;
    }
#endif
    return pstDir;
}

void YKFindClose(YKHandle hFolder)
{
    YK_DIRECTORY_ST* pstDir=(YK_DIRECTORY_ST*)hFolder;
#if _YK_OS_==_YK_OS_WIN32_
    FindClose(pstDir->hFolder);
    YKFree(pstDir->hFindData);
#else
    closedir(pstDir->hFolder);
#endif
    YKFree(pstDir);
}

BOOL YKFindNextFile(YKHandle hFolder)
{
    YK_DIRECTORY_ST* pstDir=(YK_DIRECTORY_ST*)hFolder;
#if _YK_OS_==_YK_OS_WIN32_
    return FindNextFileA(pstDir->hFolder, (WIN32_FIND_DATAA*)pstDir->hFindData);
#else
    return (pstDir->hFindData = (YKHandle)readdir(pstDir->hFolder));
#endif
}

YK_FILE_TYPE_EN  YKGetFileType(YKHandle hFolder)
{
    YK_DIRECTORY_ST* pstDir=(YK_DIRECTORY_ST*)hFolder;
#if _YK_OS_==_YK_OS_WIN32_
    WIN32_FIND_DATAA* findData=( WIN32_FIND_DATAA*)pstDir->hFindData;
    if(findData->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
    {
        return YK_TYPE_DIRECTORY;
    }
    return YK_TYPE_FILE;
#else
    struct dirent *pDirent= (struct dirent *)pstDir->hFindData;
    if(pDirent->d_name[0]=='.')
    {
        return YK_TYPE_UNKNOW;
    }
    if(pDirent->d_type != 8)
    {
        return YK_TYPE_DIRECTORY;
    }
    return YK_TYPE_FILE;
#endif
}

const char* YKGetFileName(YKHandle hFolder)
{
    YK_DIRECTORY_ST* pstDir=(YK_DIRECTORY_ST*)hFolder;
#if _YK_OS_==_YK_OS_WIN32_
    WIN32_FIND_DATAA* findData=( WIN32_FIND_DATAA*)pstDir->hFindData;
    return findData->cFileName;
#else
    struct dirent *pDirent= (struct dirent *)pstDir->hFindData;
    return pDirent->d_name;
#endif
}
