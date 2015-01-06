/***************************************************************************
 *            INIConfig.c
 *
 *  Thu Mar 10 11:13:44 2005
 *  Copyright  2005  Simon Morlat
 *  Email simon.morlat@linphone.org
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include "../INC/TMConfig.h"
#include <YKSystem.h>
#include <IDBT.h>

#if _TM_TYPE_ == _YK069_

#define TMCalloc(type,n)	(type*)calloc(sizeof(type),n)
#if _YK_OS_ == _YK_OS_WIN32_
#define snprintf    _snprintf
#endif

static TM_CONFIG_ITEM_ST * TMItemNew(const char *pcKey, const char *pcValue)
{
    TM_CONFIG_ITEM_ST *pstItem=TMCalloc(TM_CONFIG_ITEM_ST,1);
    pstItem->pcKey=strdup(pcKey);
    pstItem->pcValue=strdup(pcValue);
    return pstItem;
}

static void TMItemWrite(TM_CONFIG_ITEM_ST *pstItem, FILE *hFile)
{
    fprintf(hFile,"%s=%s\n",pstItem->pcKey,pstItem->pcValue);
}

static void TMItemDestroy(void *pvItem)
{
    TM_CONFIG_ITEM_ST *pstItem=(TM_CONFIG_ITEM_ST*)pvItem;
    free(pstItem->pcKey);
    free(pstItem->pcValue);
    free(pstItem);
}

static TM_CONFIG_SETION_ST * TMSectionNew(const char *pcName)
{
    TM_CONFIG_SETION_ST *pstItem=TMCalloc(TM_CONFIG_SETION_ST,1);
    pstItem->pcName=strdup(pcName);
    return pstItem;
}

static void TMSectionWrite(TM_CONFIG_SETION_ST *pstSec, FILE *hFile)
{
    fprintf(hFile,"[%s]\n",pstSec->pcName);
    YKListForEach2(pstSec->pstItems,(void (*)(void*, void*))TMItemWrite,(void *)hFile);
    fprintf(hFile,"\n");
}

static void TMSectionDestroy(TM_CONFIG_SETION_ST *pstSec)
{
    free(pstSec->pcName);
    YKListForEach(pstSec->pstItems,TMItemDestroy);
    YKListFree(pstSec->pstItems);
    free(pstSec);
}

static int TMIsFirstChar(const char *pcStart, const char *pcPos)
{
    const char *p;
    for(p=pcStart;p<pcPos;p++)
    {
        if (*p!=' ') 
            return 0;
    }
    return 1;
}

static TM_CONFIG_SETION_ST *TMConfigFindSection(TM_CFG_OBJECT_ST stObject, const char *pcName)
{
    TM_CONFIG_SETION_ST *pstSec=NULL;
    YK_LIST_ST *pstElem=NULL;
    for (pstElem=stObject->pstSections;pstElem!=NULL;pstElem=YKListNext(pstElem))
    {
        pstSec=(TM_CONFIG_SETION_ST*)pstElem->pvData;
        if (strcmp(pstSec->pcName,pcName)==0)
        {
            return pstSec;
        }
    }
    return NULL;
}

static TM_CONFIG_ITEM_ST *TMSectionFindItem(TM_CONFIG_SETION_ST *pstSec, const char *pcName)
{
    YK_LIST_ST *pstElem=NULL;
    TM_CONFIG_ITEM_ST *pstItem=NULL;
    for (pstElem=pstSec->pstItems;pstElem!=NULL;pstElem=YKListNext(pstElem))
    {
        pstItem=(TM_CONFIG_ITEM_ST*)pstElem->pvData;
        if (strcmp(pstItem->pcKey,pcName)==0)
        {
            return pstItem;
        }
    }
    return NULL;
}

static void TMSectionRemoveItem(TM_CONFIG_SETION_ST *pstSec, TM_CONFIG_ITEM_ST *pstItem)
{
    pstSec->pstItems=YKListRemove(pstSec->pstItems,(void *)pstItem);
    TMItemDestroy(pstItem);
}

static void TMItemSetValue(TM_CONFIG_ITEM_ST *pstItem, const char *pcValue)
{
    free(pstItem->pcValue);
    pstItem->pcValue=strdup(pcValue);
}

static void TMConfigAddSection(TM_CFG_OBJECT_ST stObject, TM_CONFIG_SETION_ST *pcSection)
{
    stObject->pstSections=YKListAppend(stObject->pstSections,(void *)pcSection);
}

static void TMConfigRemoveSection(TM_CFG_OBJECT_ST stObject, TM_CONFIG_SETION_ST *pcSection)
{
    stObject->pstSections=YKListRemove(stObject->pstSections,(void *)pcSection);
    TMSectionDestroy(pcSection);
}

static void TMSectionAddItem(TM_CONFIG_SETION_ST *pstSec,TM_CONFIG_ITEM_ST *pstItem)
{
    pstSec->pstItems=YKListAppend(pstSec->pstItems,(void *)pstItem);
}

static void TMConfigParse(TM_CFG_OBJECT_ST stObject, FILE *hFile)
{
    char acTmp[MAX_LEN]={0x0};
    TM_CONFIG_SETION_ST *cur=NULL;
    if (hFile==NULL) return;

    while(fgets(acTmp,MAX_LEN,hFile)!=NULL)
    {
        char *pos1,*pos2;
        pos1=strchr(acTmp,'[');
        if (pos1!=NULL && TMIsFirstChar(acTmp,pos1) )
        {
            pos2=strchr(pos1,']');
            if (pos2!=NULL)
            {
                int nbs;
                char secname[MAX_LEN];
                secname[0]='\0';
                /* found pcSection */
                *pos2='\0';
                nbs = sscanf(pos1+1,"%s",secname);
                if (nbs == 1 )
                {
                    if (strlen(secname)>0)
                    {
                        cur=TMConfigFindSection (stObject,secname);
                        if (cur==NULL)
                        {
                            cur=TMSectionNew(secname);
                            TMConfigAddSection(stObject,cur);
                        }
                    }
                }
				else
				{
                }
            }
        }
		else
		{
            pos1=strchr(acTmp,'=');
            if (pos1!=NULL)
            {
                char acKey[MAX_LEN];
                acKey[0]='\0';

                *pos1='\0';
                if (sscanf(acTmp,"%s",acKey)>0)
                {

                    pos1++;
                    pos2=strchr(pos1,'\r');
                    if (pos2==NULL)
                        pos2=strchr(pos1,'\n');
                    if (pos2==NULL)
                        pos2=pos1+strlen(pos1);
                    else 
                    {
                        *pos2='\0'; /*replace the '\n' */
                        pos2--;
                    }
                    /* remove ending white spaces */
                    for (; pos2>pos1 && *pos2==' ';pos2--) *pos2='\0';
                    if (pos2-pos1>=0)
                    {
                        /* found a pair key,value */
                        if (cur!=NULL)
                        {
                            TM_CONFIG_ITEM_ST *pstItem=TMSectionFindItem(cur,acKey);
                            if (pstItem==NULL)
                            {
                                TMSectionAddItem(cur,TMItemNew(acKey,pos1));
                            }
                            else
                            {
                                free(pstItem->pcValue);
                                pstItem->pcValue=strdup(pos1);
                            }
                            /*printf("Found %s %s=%s\n",cur->pcName,key,pos1);*/
                        }
						else
						{
                        }
                    }
                }
            }
        }
    }
}
TM_CFG_OBJECT_ST TMConfigNew(const char *pcFileName)
{
    TM_CONFIG_ST* pstObject=TMCalloc(TM_CONFIG_ST,1);
    if (NULL!=pcFileName)
    {
        pstObject->pcFileName=strdup(pcFileName);
        pstObject->hFile=fopen(pcFileName,"r+");
        if (pstObject->hFile!=NULL)
        {
            TMConfigParse(pstObject, pstObject->hFile);
            fclose(pstObject->hFile);
            pstObject->hFile=NULL;
            pstObject->iModified=0;
        }
    }
    return pstObject;
}

int TMConfigNew2(TM_CFG_OBJECT_ST stObject, const char *pcFileName)
{
    FILE* f=fopen(pcFileName,"r");
    if (f!=NULL)
    {
        TMConfigParse(stObject, f);
        fclose(f);
        return 0;
    }
    return -1;
}

void TMConfigDestroy(TM_CFG_OBJECT_ST stObject)
{
    if (stObject->pcFileName!=NULL)
    {
        free(stObject->pcFileName);
    }
    YKListForEach(stObject->pstSections,(void (*)(void*))TMSectionDestroy);
    YKListFree(stObject->pstSections);
    free(stObject);
}

const char *TMConfigGetString(TM_CFG_OBJECT_ST stObject, const char *pcSection, const char *pcKey, const char *pcDefaultValue)
{
    TM_CONFIG_SETION_ST *pstSec=NULL;
    TM_CONFIG_ITEM_ST *pstItem=NULL;
    pstSec=TMConfigFindSection(stObject,pcSection);
    if (NULL!=pstSec)
    {
        pstItem=TMSectionFindItem(pstSec,pcKey);
        if (pstItem!=NULL)
            return pstItem->pcValue;
    }
    return pcDefaultValue;
}

int TMConfigGetInt(TM_CFG_OBJECT_ST stObject,const char *pcSection, const char *pcKey, int iDefaultValue)
{
    const char *str=TMConfigGetString(stObject,pcSection,pcKey,NULL);
    if (NULL!=str)
    {
        return atoi(str);
    }
    else
    {
        return iDefaultValue;
    }
}

float TMConfigGetFloat(TM_CFG_OBJECT_ST stObject,const char *pcSection, const char *pcKey, float fDefaultValue)
{
    const char *str=TMConfigGetString(stObject,pcSection,pcKey,NULL);
    float ret=fDefaultValue;
    if (str==NULL)
    {
        return fDefaultValue;
    }
    else
    {
        sscanf(str,"%f",&ret);
        return ret;
    }
}

void TMConfigSetString(TM_CFG_OBJECT_ST stObject,const char *pcSection, const char *pcKey, const char *pcValue)
{
    TM_CONFIG_ITEM_ST *pstItem=NULL;
    TM_CONFIG_SETION_ST *pstSec=TMConfigFindSection(stObject,pcSection);
    if (pstSec!=NULL)
    {
        pstItem=TMSectionFindItem(pstSec,pcKey);
        if (pstItem!=NULL)
        {
            if (pcValue!=NULL)
                TMItemSetValue(pstItem,pcValue);
            else 
                TMSectionRemoveItem(pstSec,pstItem);
        }
        else
        {
            if (pcValue!=NULL)
                TMSectionAddItem(pstSec,TMItemNew(pcKey,pcValue));
        }
    }
    else if (pcValue!=NULL)
    {
        pstSec=TMSectionNew(pcSection);
        TMConfigAddSection(stObject,pstSec);
        TMSectionAddItem(pstSec,TMItemNew(pcKey,pcValue));
    }
    stObject->iModified++;
}

void TMConfigSetInt(TM_CFG_OBJECT_ST stObject,const char *pcSection, const char *pcKey, int iValue)
{
    char acTmp[30]={0x0};
    snprintf(acTmp,sizeof(acTmp),"%i",iValue);
    TMConfigSetString(stObject,pcSection,pcKey,acTmp);
}

void TMConfigSetFloat(TM_CFG_OBJECT_ST stObject, const char *pcSection, const char *pcKey, float fValue)
{
    char acTmp[30];
    snprintf(acTmp,sizeof(acTmp),"%f",fValue);
    TMConfigSetString(stObject,pcSection,pcKey,acTmp);
}

int TMConfigHasSection(TM_CFG_OBJECT_ST stObject, const char *pcSection)
{
    if (TMConfigFindSection(stObject,pcSection)!=NULL)
        return 1;
    return 0;
}

void TMConfigCleanSection(TM_CFG_OBJECT_ST stObject, const char *pcSection)
{
    TM_CONFIG_SETION_ST *pstSec=TMConfigFindSection(stObject,pcSection);
    if (pstSec!=NULL)
    {
        TMConfigRemoveSection(stObject,pstSec);
    }
    stObject->iModified++;
}

int TMConfigSync(TM_CFG_OBJECT_ST stObject)
{
    FILE *hFile;
    if (stObject->pcFileName==NULL) 
        return -1;
    if (stObject->iReadOnly) 
        return 0;
#ifndef WIN32
    /* don't create group/world-accessible files */
    (void) umask(S_IRWXG | S_IRWXO);
#endif
    hFile=fopen(stObject->pcFileName,"w");
    if (hFile==NULL)
    {
        stObject->iReadOnly=1;
        return -1;
    }
    YKListForEach2(stObject->pstSections,(void (*)(void *,void*))TMSectionWrite,(void *)hFile);
    fclose(hFile);
    stObject->iModified=0;
    return 0;
}

#endif
