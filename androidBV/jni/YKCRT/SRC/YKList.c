#include "../INC/YKList.h"

typedef int (*YKCompareFunc)(const void *a, const void *b);

YK_LIST_ST *YKListNew(void *pvData)
{
    YK_LIST_ST *pstNewElem = (YK_LIST_ST *)malloc(sizeof(YK_LIST_ST));
    if(NULL == pstNewElem)
    {
        return NULL;
    }
    pstNewElem->pstPrev = pstNewElem->pstNext = NULL;
    pstNewElem->pvData  = pvData;
    return pstNewElem;
}

YK_LIST_ST * YKListAppend(YK_LIST_ST *pstElem, void * pvData)
{
    YK_LIST_ST *pstNewElem = YKListNew(pvData); 
    YK_LIST_ST *pstTmpElem = pstElem;
    if(NULL == pstNewElem)
    {
        return NULL;
    }
    if (NULL == pstElem) 
    {
        return pstNewElem;
    }
    while(NULL != pstTmpElem->pstNext) 
    {
        pstTmpElem = YKListNext(pstTmpElem);
    }
    pstTmpElem->pstNext = pstNewElem;
    pstNewElem->pstPrev = pstTmpElem;
    return pstElem;
}

void* YKListPop(YK_LIST_ST **ppstList)
{
    YK_LIST_ST *pstTmpElem = NULL;
    void* pvData = NULL;
    if(NULL == *ppstList)
    {
        return NULL;
    }
    while(NULL != (*ppstList)->pstNext)
    {
        pstTmpElem = (*ppstList)->pstNext;
        (*ppstList)->pstNext = pstTmpElem->pstNext;
        pstTmpElem->pstPrev  = (*ppstList);
        pvData = pstTmpElem->pvData;
        YKListRemoveLink((*ppstList), pstTmpElem);
        break;
    }
    return pvData;   
}

YK_LIST_ST * YKListPrepend(YK_LIST_ST *pstElem, void *pvData)
{
    YK_LIST_ST *pstNewElem = YKListNew(pvData);
    if (NULL != pstNewElem && NULL != pstElem) 
    {
        pstNewElem->pstNext = pstElem;                                                   
        pstElem->pstPrev = pstNewElem;
    }
    return pstNewElem;
}

YK_LIST_ST * YKListConcat(YK_LIST_ST *pstFirst, YK_LIST_ST *pstSecond)
{
    YK_LIST_ST *pstTmpElem = pstFirst;
    if (NULL == pstTmpElem || NULL == pstSecond)
    {
        return pstSecond;
    }
    while(NULL != pstTmpElem->pstNext)
    {
        pstTmpElem = YKListNext(pstTmpElem);
    }
    pstTmpElem->pstNext = pstSecond;
    pstSecond->pstPrev  = pstTmpElem;
    return pstFirst;
}

YK_LIST_ST * YKListFree(YK_LIST_ST *pstList)
{
    YK_LIST_ST *pstElem = pstList;
    YK_LIST_ST *pstTmpElem;
    if (NULL == pstList)
    {
        return NULL;
    }
    while(NULL != pstElem->pstNext)
    {
        pstTmpElem  = pstElem;
        pstElem     = pstElem->pstNext;
        free(pstTmpElem);
    }
    free(pstElem);
    return NULL;
}

YK_LIST_ST * YKListRemove(YK_LIST_ST *pstFirst, void *pvData)
{
    YK_LIST_ST *pstTmpElem = YKListFind(pstFirst,pvData);
    if (pstTmpElem)
    {
        return YKListRemoveLink(pstFirst,pstTmpElem);
    }
    else 
    {
        return pstFirst;
    }
}

int YKListSize(const YK_LIST_ST *pstFirst)
{
    int iCount = 0;
    while(NULL != pstFirst)
    {
        ++iCount;
        pstFirst = pstFirst->pstNext;
    }
    return iCount;
}

void YKListForEach(const YK_LIST_ST *pstList, void (*func)(void *))
{
    if(NULL == func)
    {
        return;
    }
    for(; NULL != pstList; pstList = pstList->pstNext)
    {
        func(pstList->pvData);
    }
}

void YKListForEach2(const YK_LIST_ST *pstList, void (*func)(void *, void *), void *pvUserData)
{
    if(NULL == func)
    {
        return;
    }
    for(; NULL != pstList;pstList = pstList->pstNext)
    {
        func(pstList->pvData,pvUserData);
    }
}

YK_LIST_ST *YKListRemoveLink(YK_LIST_ST *pstList, YK_LIST_ST *pstElem)
{
    YK_LIST_ST *ret = NULL;
    if (pstElem == pstList)
    {
        ret = pstElem->pstNext;
        pstElem->pstPrev = NULL;
        pstElem->pstNext = NULL;
        if (NULL != ret)
        {
            ret->pstPrev = NULL;
        }
        free(pstElem);
        return ret;
    }
    pstElem->pstPrev->pstNext = pstElem->pstNext;
    if (NULL != pstElem->pstNext)
    {
        pstElem->pstNext->pstPrev = pstElem->pstPrev;
    }
    pstElem->pstNext = NULL;
    pstElem->pstPrev = NULL;
    free(pstElem);
    return pstList;
}

YK_LIST_ST *YKListFind(YK_LIST_ST *pstList, void *pvData)
{
    for(; NULL != pstList;pstList = pstList->pstNext)
    {
        if (pstList->pvData == pvData) 
        {
            return pstList;
        }
    }
    return NULL;
}

YK_LIST_ST *YKListFindCustom(YK_LIST_ST *pstList, int (*compare_func)(const void *, const void*), const void *pvUserData)
{
    for(; NULL != pstList;pstList = pstList->pstNext)
    {
        if (0 == compare_func(pstList->pvData,pvUserData)) 
        {
            return pstList;
        }
    }
    return NULL;
}

void * YKListNthData(const YK_LIST_ST *pstList, int iIndex)
{
    int i;
    for(i = 0; NULL != pstList; pstList = pstList->pstNext, ++i)
    {
        if ( i== iIndex) 
        {
            return pstList->pvData;
        }
    }	
    return NULL;
}

int YKListPosition(const YK_LIST_ST *pstList, YK_LIST_ST *pstElem)
{
    int i;
    for(i = 0; NULL != pstList; pstList = pstList->pstNext, ++i)
    {
        if (pstElem == pstList)
        {
            return i;
        }
    }

    return -1;
}

int YKListIndex(const YK_LIST_ST *pstList, void *pvData)
{
    int i;
    for(i = 0; NULL != pstList; pstList = pstList->pstNext, ++i)
    {
        if (pvData == pstList->pvData) 
        {
            return i;
        }
    }
    return -1;
}

YK_LIST_ST *YKListInsertSorted(YK_LIST_ST *pstList, void *pvData, int (*compare_func)(const void *, const void*))
{
    YK_LIST_ST *pstTmpElem,*previt = NULL;
    YK_LIST_ST *nelem;
    YK_LIST_ST *ret = pstList;
    if (NULL == pstList) 
    {
        return YKListAppend(pstList,pvData);
    }
    else
    {
        nelem = YKListNew(pvData);
        for(pstTmpElem = pstList; NULL != pstTmpElem; pstTmpElem = pstTmpElem->pstNext)
        {
            previt = pstTmpElem;
            if (compare_func(pvData,pstTmpElem->pvData) <= 0)
            {
                nelem->pstPrev = pstTmpElem->pstPrev;
                nelem->pstNext = pstTmpElem;
                if (NULL != pstTmpElem->pstPrev)
                {
                    pstTmpElem->pstPrev->pstNext = nelem;
                }
                else
                {
                    ret = nelem;
                }
                pstTmpElem->pstPrev = nelem;
                return ret;
            }
        }
        previt->pstNext = nelem;
        nelem->pstPrev  = previt;
    }
    return ret;
}

YK_LIST_ST *YKListInsert(YK_LIST_ST *pstList, YK_LIST_ST *pstBefore, void *pvData)
{
    YK_LIST_ST *pstElem;
    if (NULL == pstList || NULL == pstBefore) 
    {
        return YKListAppend(pstList,pvData);
    }
    for(pstElem = pstList; NULL != pstElem; pstElem = YKListNext(pstElem))
    {
        if (pstElem == pstBefore)
        {
            if (NULL == pstElem->pstPrev)
            {
                return YKListPrepend(pstList,pvData);
            }
            else
            {
                YK_LIST_ST *nelem = YKListNew(pvData);
                nelem->pstPrev = pstElem->pstPrev;
                nelem->pstNext = pstElem;
                pstElem->pstPrev->pstNext = nelem;
                pstElem->pstPrev = nelem;
            }
        }
    }
    return pstList;
}

YK_LIST_ST *YKListCopy(const YK_LIST_ST *pstList)
{
    YK_LIST_ST *copy=NULL;
    const YK_LIST_ST *iter;
    for(iter = pstList; NULL != iter; iter = YKListNext(iter))
    {
        copy = YKListAppend(copy,iter->pvData);
    }
    return copy;
}