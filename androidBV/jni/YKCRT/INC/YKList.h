                     /**
 *@addtogroup YKCRT
 *@{
 */

/**
 *@file YKList.h
 *@brief Basic types defines. 
 *@author Amos
 *@version 1.0
 *@date 2012-05-22
 */

#ifndef __YK_LIST_H__
#define __YK_LIST_H__

#include "YKSystem.h"

//#ifdef __cplusplus
//extern "C"{
//#endif
/**
 *@name List defines
 *@{
 */

/**@brief List data structure. */
typedef struct _YKList {
    struct _YKList *pstNext;   /**<!Point under a linked list of nodes. */
    struct _YKList *pstPrev;   /**<!Point prev a linked list of nodes. */
    void *pvData;             /**<!List data.*/
}YK_LIST_ST;

/**@brief Get pNext element form special element. */
#define YKListNext(pstElem) ((pstElem)->pstNext)

/**
 *@brief Create new list.
 *@param[in] data Init data.
 *@return List head node.
 */
YK_LIST_ST* YKListNew(void *pvData);

/**
 *@brief Append new node to special list.
 *@param[in] elem Special list
 *@param[in] data New element data.
 *@return  New list head node.
 */
YK_LIST_ST* YKListAppend(YK_LIST_ST *pstElem, void * pvData);

/**
 *@brief Add new node to the prev of a special element
 *@param[in] elem Special element
 *@param[in] data New element data.
 *@return New list head node.
 */
YK_LIST_ST* YKListPrepend(YK_LIST_ST *pstElem, void *pvData);

/**
 *@brief Destroy list.
 *@param[in] list Special list.
 */
YK_LIST_ST* YKListFree(YK_LIST_ST *pstList);

/**
 *@brief Delete the node containing the specified data
 *@param[in] first The list which containing the specified data
 *@param[in] data The special data.
 *@return New list
 */
YK_LIST_ST* YKListRemove(YK_LIST_ST *pstFirst, void *pvData);

/**
 *@brief Get the numbers of nodes from special list.
 *@param[in] first Special list.
 *@return Numbers of nodes.
 */
int YKListSize(const YK_LIST_ST *pstFirst);

/**
 *@brief Loop through the list, the list data as parameters, to invoke the specified function
 *@param[in] list Special list.
 *@param[in] func Special function.
 *@return void.
 */
void    YKListForEach(const YK_LIST_ST *pstList, void (*func)(void *));

/**
 *@brief Remove Special element.
 *@param[in] list Special list.
 *@param[in] elem Which will removed.
 *@return New list.
 */
YK_LIST_ST* YKListRemoveLink(YK_LIST_ST *pstList, YK_LIST_ST *pstElem);

/**
 *@brief Find the element which containing the specified data form list.
 *@param[in] list Special list.
 *@param[in] data Special data.
 *@return The element which found.
 */
YK_LIST_ST* YKListFind(YK_LIST_ST *pstList, void *pvData);

/**
 *@brief Get the position of element.
 *@param[in] list Special list.
 *@param[in] elem Special element.
 *@return The position of element
 */
int YKListPosition(const YK_LIST_ST *pstList, YK_LIST_ST *pstElem);

/**
 *@brief Get the index of element.
 *@param[in] list Special list.
 *@param[in] data Special element.
 *@return The index of element
 */
int YKListIndex(const YK_LIST_ST *pstList, void *pvData);

/**
 *@brief Insert a new node to a list pstBefore special element.
 *@param[in] list Special list.
 *@param[in] before Before the special element a new element will inserted.
 *@param[in] data Which will init new element.
 *@return New list.
 */
YK_LIST_ST* YKListInsert(YK_LIST_ST *pstList, YK_LIST_ST *pstBefore, void *pvData);

/**
 *@brief Copy list.
 *@param[in] list Special list.
 *@return New list.
 */
YK_LIST_ST* YKListCopy(const YK_LIST_ST *pstList);

/**
 *@brief Pop first data.
 *@param[in/out] list  Out put new list. 
 *@return The data of element.
 */
void* YKListPop(YK_LIST_ST**ppstList);

/**@}*///List defines

//#ifdef __cplusplus
//}
//#endif

#endif//! __YK_LIST_H__
