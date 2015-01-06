/**
 *@defgroup YKCRT
 *@{
 */

/**
 *@file
 *@brief System dependant service routines
 *
 *Different platforms and the definition of basic data types go here
 *@author Amos
 *@version 1.0
 *@date 2012-04-27
 */
#ifndef __YK_SYSTEM_H__
#define __YK_SYSTEM_H__

/**
 *@name OS Defines
 *@{
 */

/**@brief Windows OS(from Windows95. eg.XP/VISTA/WIN7). */
#define _YK_OS_WIN32_   1
/**@brief Linux OS. */
#define _YK_OS_LINUX_   2
/**@brief MAC OS. */
#define _YK_OS_MAC_     3
/**@brief Android OS. */
#define _YK_OS_ANDROID_ 4

#define _YK_OS_ _YK_OS_ANDROID_

#ifndef _YK_OS_
    #if	defined(_WIN32) || defined(_WIN32_WCE)   
        #define _YK_OS_  _YK_OS_WIN32_
    #elif defined(__linux__)
        #define _YK_OS_  _YK_OS_LINUX_
    #elif defined(__APPLE__)
        #define _YK_OS_ _YK_OS_MAC_
    #else
        #define _YK_OS_ _YK_OS_ANDROID_
    #endif
#endif/*!_YK_OS_*/

#if !defined(_YK_OS_) || _YK_OS_ == 0
#error Sorry, can not figure out what OS you are targeting to. Please specify _YK_OS_ macro.
#endif

/**@}*///OS Defines

/* Include basic headers */
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#if   (_YK_OS_ == _YK_OS_WIN32_) 
#include <windows.h>
#elif (_YK_OS_ == _YK_OS_LINUX_)
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <iconv.h>
#include <memory.h>
#include <time.h>
#include<dirent.h>
#include<sys/wait.h>
#include<semaphore.h>
#elif (_YK_OS_ == _YK_OS_MAC_)
#elif (_YK_OS_ == _YK_OS_ANDROID_)
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <memory.h>
#include <time.h>
#include<dirent.h>
#include<sys/wait.h>
#include<semaphore.h>
#endif

/**
 *@name Basic data types
 *@{
 */
/**@brief Byte (8bits). */
typedef unsigned char   YKByte;
/**@brief Short integer(16 bits).*/
typedef short           YKShort;
/**@brief Unsigned integer. */
typedef unsigned short  YKUShort;
/**@brief 32-bits signed integer. */
typedef int             YKInt32;
/**@brief 32-bits unsigned integer. */
typedef unsigned int    YKUInt32;
/**@brief Long integer. */
typedef long            YKLong;
/**@brief Unsigned long integer. */
typedef unsigned long   YKULong;
/**@brief Boolean variable. Should be TRUE or FALSE.*/
typedef int             YKBool;
#ifndef BOOL
typedef int             BOOL;
#endif
/**@brief 8-bits ANSI character. */
typedef char            YKChar;
/** @brief 32-bit floating-point number. */
typedef float			YKFloat;
/**@brief Point to any types*/
typedef void*   YKHandle;

/**@}*///Basic pData types.

/**
 *@name Keywords Defines.
 *@{
 */
#ifndef TRUE
/** @brief Keyword which value is 1. */
#define TRUE 1
#endif

#ifndef FALSE
/** @brief Keyword which value is 0. */
#define FALSE 0
#endif

#ifndef NULL
/** @brief The null-pointer value. */
#define NULL 0
#endif

/** @brief quit status which success. */
#define SUCCESS 0

/** @brief quit status which failed. */
#define FAILURE -1

/**@}*///Keywords Defines.


#endif/*!__YK_SYSTEM_H__*/

/**@}*///defgroup YKCRT
