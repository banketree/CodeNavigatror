/*
 * Sort.c
 *
 *  Created on: 2013-4-1
 *      Author: root
 */
#include <IDBT.h>
#include <sd/list.h>
#include "sd/domnode.h"

#ifdef CONFIG_SORT

typedef enum
{
	SORT_CONFIG_CODE 	= 	0x1,
	SORT_CONFIG_CORNET	=	0x2,

}SORT_TYPE_EN;

typedef struct
{
	double dCode;
	double dCornet;
	char acCode[64];
	char acCCornet[32];
}SORT_CONFIG_ST;

typedef struct
{
	int iState;
	int iModify;
	int bThread;
	YKHandle hLock;
	sd_list_t *pstList;
}SORT_STATE_ST;

typedef void (*SortCallback)(void);
typedef void* (*SortMax)(void *arg1,void *arg2);

#define SORT_OK		1
#define NO_SORT 	0

sd_list_t *g_lConfigCodeSortTemp = NULL;
sd_list_t *g_lConfigCornetSortTemp = NULL;
SORT_STATE_ST g_stSortConfigCode;
SORT_STATE_ST g_stSortConfigCornet;
extern char g_acTMPhoneNum[128];

int bubble_sort_list(sd_list_iter_t * header, const int size ,SortMax max)
{
    int i, j;
    sd_list_iter_t *p1;

    for(i=size-1; i>0; i--)
    {
    	p1 = header;
        for(j=0; j<i; j++)
        {
        	//比较排序函数
        	if(max(p1,p1->__next) == 0)
        	{
        		return 0;
        	}
        	p1 = p1->__next;
        }
    }

    return 1;
}

int SortMaxCodeList(sd_list_iter_t *arg1,sd_list_iter_t *arg2)
{
	SORT_CONFIG_ST *p1 = (SORT_CONFIG_ST *)arg1->data;
	SORT_CONFIG_ST *p2 = (SORT_CONFIG_ST *)arg2->data;
	void *p;

	if(g_stSortConfigCode.iModify == TRUE)
	{
		return 0;
	}

	if(p1->dCode > p2->dCode)
	{
		p = arg1->data ;
		arg1->data = arg2->data;
		arg2->data = p;
	}

	return 1;
}

int SortMaxCodeDouble(SORT_CONFIG_ST *arg1,double *arg2)
{
	SORT_CONFIG_ST *p1 = (SORT_CONFIG_ST *)arg1;

	if(p1->dCode > *arg2)
	{
		return 1;
	}
	else if(p1->dCode <  (int)*arg2)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

int SortMaxCornetList(sd_list_iter_t *arg1,sd_list_iter_t *arg2)
{
	SORT_CONFIG_ST *p1 = (SORT_CONFIG_ST *)arg1->data;
	SORT_CONFIG_ST *p2 = (SORT_CONFIG_ST *)arg2->data;
	void *p;

	if(g_stSortConfigCornet.iModify == TRUE)
	{
		return 0;
	}

	if(p1->dCornet > p2->dCornet)
	{
		p = arg1->data ;
		arg1->data = arg2->data;
		arg2->data = p;
	}

	return 1;
}

int SortMaxCornetDouble(SORT_CONFIG_ST *arg1,double *arg2)
{
	SORT_CONFIG_ST *p1 = (SORT_CONFIG_ST *)arg1;

//	printf("cornet = %lf\n",p1->dCornet);
	if(p1->dCornet > *arg2)
	{
		return 1;
	}
	else if(p1->dCornet <  *arg2)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}

sd_list_iter_t * binarysearch(sd_list_iter_t * header,double arg,int size,SortMax max)
{
	sd_list_iter_t *p1;
	int mid, start = 0, end = size - 1;
	int i=  0,j = 0;

	while (start <= end)
	{
		p1 = header;
		mid = (start + end) / 2;
//		printf("mid = %d\n",mid);
		for(i=0;i<mid;i++)
		{
			p1 = p1->__next;
		}
		j = max((p1->data),&arg);
		if(j > 0)
		{
			end = mid - 1;
		}
		else if (j < 0)
		{
			start = mid + 1;
		}
 		else
 		{
 			return p1;
 		}
	}
	return NULL;
}

void ConfigCodeFilter(const char *src,char *dest)
{
	char *p;
	char buf[128];
	int len;
	if(!src || !dest)
	{
		return;
	}
	strcpy(buf,src);
	//去后缀
	p = strstr(buf,"@");
	if(p)
	{
		*p = '\0';
	}
	len = strlen(buf);
	if(len >= 7)
		memcpy(dest,&buf[len - 7],7);
	else
		strcpy(dest,buf);

}

BOOL I6CCIsValidNumSort(const char* pcPhoneNum, char* pcHouseCode)
{
	int iPhoneNum;
	char acCode[64];
	int num,i;
	sd_list_iter_t *p1;

	//索引查找
	ConfigCodeFilter(pcPhoneNum,acCode);
	sscanf(acCode,"%d",&iPhoneNum);
	YKLockMutex(g_stSortConfigCode.hLock);
	p1 = binarysearch(sd_list_begin(g_stSortConfigCode.pstList),iPhoneNum,sd_list_get_nelem(g_stSortConfigCode.pstList),SortMaxCodeDouble);
	YKUnLockMutex(g_stSortConfigCode.hLock);
	if(p1)
	{
		if(pcHouseCode && p1->data)
		{
			strcpy(pcHouseCode,((SORT_CONFIG_ST *)p1->data)->acCCornet);
		}
		return TRUE;
	}
	return FALSE;
}

char *I6CCGeNumSort(const char* pcHouseCode)
{
	sd_list_iter_t *p1;
	char acCode[64];
	char acPhoneTemp[64] = {0x0};
	char acDomain[64] = {0x0};
	double dCornet;

	//索引查找
	sscanf(pcHouseCode,"%lf",&dCornet);
	YKLockMutex(g_stSortConfigCornet.hLock);
	p1 = binarysearch(sd_list_begin(g_stSortConfigCornet.pstList),dCornet,sd_list_get_nelem(g_stSortConfigCornet.pstList),SortMaxCornetDouble);
	YKUnLockMutex(g_stSortConfigCornet.hLock);
	if(p1 && p1->data)
	{
		strcpy(acPhoneTemp,((SORT_CONFIG_ST *)p1->data)->acCode);
		//判断是否有＠后缀
		if(strstr(acPhoneTemp,"@") == NULL)
		{
			if(TMNMGetDomainName(acDomain))
			{
				if(strlen(acPhoneTemp) == 18)
				{
					sprintf(g_acTMPhoneNum,"+86%s@%s",acPhoneTemp,acDomain);
					return g_acTMPhoneNum;
				}
				sprintf(g_acTMPhoneNum,"%s@%s",acPhoneTemp,acDomain);
				return g_acTMPhoneNum;
			}
		}
		strcpy(g_acTMPhoneNum,acPhoneTemp);
		return g_acTMPhoneNum;
	}
	return NULL;
}

sd_list_t * ReadCodeFromConfig()
{
	sd_domnode_t* document = sd_domnode_new(NULL, NULL);
	sd_domnode_t* phonebook;
	sd_domnode_t* child;
	sd_domnode_t* Code = NULL;
	sd_domnode_t* Cornet = NULL;
	sd_list_t *SortList = NULL;
	SORT_CONFIG_ST *newsort;
	char acCode[64];

	if(g_stSortConfigCode.iModify == TRUE)
	{
		g_stSortConfigCode.iModify = FALSE;
	}
	if(-1 == sd_domnode_load(document, I6_CONFIG_PATH))
	{
		return NULL;
	}

	SortList = sd_list_new(1);

	phonebook = sd_domnode_search(document, "PHONE_BOOK");//find phone book node.
	child = sd_domnode_first_child(phonebook);
	while(child && g_stSortConfigCode.iModify == FALSE)
	{
		if( (Code = sd_domnode_attrs_get(child, "Code")) &&
				(Cornet = sd_domnode_attrs_get(child, "Cornet")))
		{
			if(Code->value && Cornet->value)
			{
				//开辟空间 添加数据
				newsort = (SORT_CONFIG_ST*)malloc(sizeof(SORT_CONFIG_ST));
				ConfigCodeFilter(Code->value,acCode);
				sscanf(acCode,"%lf",&newsort->dCode);
				sscanf(Cornet->value,"%lf",&newsort->dCornet);
				strcpy(newsort->acCode,Code->value);
				strcpy(newsort->acCCornet,Cornet->value);
				//添加数据到链表
				sd_list_add_last(SortList,newsort);
			}
		}

		child = sd_domnode_next_child(phonebook, child);
	}
	sd_domnode_delete(document);
	return SortList;
}


int IsSortConfigCodeOK()
{
	return g_stSortConfigCode.iState;
}

int IsSortConfigCornetOK()
{
	return g_stSortConfigCornet.iState;
}

void SortForConfigCode()
{
	printf("code sort start===============\n");
	g_stSortConfigCode.bThread = TRUE;
	do
	{
		//读取数据
		g_lConfigCodeSortTemp = ReadCodeFromConfig();

		if(g_lConfigCodeSortTemp == NULL)
		{
			printf("code sort end, file error===============\n");
			g_stSortConfigCode.bThread = FALSE;
			return;
		}

		//排序
		if(g_stSortConfigCode.iModify != TRUE && bubble_sort_list(sd_list_begin(g_lConfigCodeSortTemp),sd_list_get_nelem(g_lConfigCodeSortTemp),SortMaxCodeList) > 0)
		{
			break;
		}

		sd_list_delete(g_lConfigCodeSortTemp);
		g_stSortConfigCode.iModify = FALSE;
	}while(1);
	g_stSortConfigCode.bThread = FALSE;
	//调试
	printf("code sort num = %d\n",sd_list_get_nelem(g_lConfigCodeSortTemp));
//	sd_list_iter_t*p1 = sd_list_begin(g_lConfigCodeSortTemp);
//	while(p1)
//	{
//		printf("code = %lf,cornet = %s, acCode = %s\n",((SORT_CONFIG_ST *)(p1->data))->dCode,((SORT_CONFIG_ST *)(p1->data))->acCCornet,((SORT_CONFIG_ST *)(p1->data))->acCode);
//		p1 = p1->__next;
//	}
	printf("code sort ok===============\n");
	//替换为新的链表
	YKLockMutex(g_stSortConfigCode.hLock);
	if(g_stSortConfigCode.pstList)
	{
		sd_list_delete(g_stSortConfigCode.pstList);
	}
	g_stSortConfigCode.pstList = g_lConfigCodeSortTemp;
	g_lConfigCodeSortTemp = NULL;
	g_stSortConfigCode.iState = SORT_OK;
	YKUnLockMutex(g_stSortConfigCode.hLock);
}

void SortForConfigCornet()
{
	printf("Cornet sort start===============\n");
	g_stSortConfigCornet.bThread = TRUE;
	do
	{
		//读取数据
		g_lConfigCornetSortTemp = ReadCodeFromConfig();

		if(g_lConfigCornetSortTemp == NULL)
		{
			printf("Cornet sort end, file error===============\n");
			g_stSortConfigCornet.bThread = FALSE;
			return;
		}

		//排序
		if(g_stSortConfigCornet.iModify != TRUE && bubble_sort_list(sd_list_begin(g_lConfigCornetSortTemp),sd_list_get_nelem(g_lConfigCornetSortTemp),SortMaxCornetList) > 0)
		{
			break;
		}

		sd_list_delete(g_lConfigCornetSortTemp);
		g_stSortConfigCornet.iModify = FALSE;
	}while(1);
	g_stSortConfigCornet.bThread = FALSE;
	//调试
	printf("Cornet sort num = %d\n",sd_list_get_nelem(g_lConfigCornetSortTemp));
//	sd_list_iter_t*p1 = sd_list_begin(g_lConfigCornetSortTemp);
//	int i = 0;
//	while(p1)
//	{
//		if(i++ >10)
//		{
//			break;
//		}
//		printf("code = %lf,cornet = %s, dCornet = %lf\n",((SORT_CONFIG_ST *)(p1->data))->dCode,((SORT_CONFIG_ST *)(p1->data))->acCCornet,((SORT_CONFIG_ST *)(p1->data))->dCornet);
//		p1 = p1->__next;
//	}
	printf("Cornet sort ok===============\n");
	//替换为新的链表
	YKLockMutex(g_stSortConfigCornet.hLock);
	if(g_stSortConfigCornet.pstList)
	{
		sd_list_delete(g_stSortConfigCornet.pstList);
	}
	g_stSortConfigCornet.pstList = g_lConfigCornetSortTemp;
	g_lConfigCornetSortTemp = NULL;
	g_stSortConfigCornet.iState = SORT_OK;
	YKUnLockMutex(g_stSortConfigCornet.hLock);
}

void SortThread(SortCallback callback)
{
	//调用回调
	if(callback)
	{
		pthread_t threadId;
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_create(&threadId, &attr, callback, NULL);
		pthread_attr_destroy(&attr);
	}
}

void SortConfigCode()
{
	if(g_stSortConfigCode.bThread == TRUE)
	{
		g_stSortConfigCode.iModify = TRUE;
	}
	else
	{
		SortThread(SortForConfigCode);
	}
}

void SortConfigCornet()
{
	if(g_stSortConfigCornet.bThread == TRUE)
	{
		g_stSortConfigCornet.iModify = TRUE;
	}
	else
	{
		SortThread(SortForConfigCornet);
	}
}

void SortInit()
{
	g_stSortConfigCode.hLock = YKCreateMutex(NULL);
	g_stSortConfigCornet.hLock = YKCreateMutex(NULL);
}

void SortFini()
{
	YKDestroyMutex(g_stSortConfigCode.hLock);
	YKDestroyMutex(g_stSortConfigCornet.hLock);
	if(g_stSortConfigCode.pstList)
	{
		sd_list_delete(g_stSortConfigCode.pstList);
	}
	if(g_stSortConfigCornet.pstList)
	{
		sd_list_delete(g_stSortConfigCornet.pstList);
	}
}

#endif
