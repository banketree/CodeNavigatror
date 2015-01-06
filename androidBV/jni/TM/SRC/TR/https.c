/*
 *  SSL client demonstration program
 *
 *  Copyright (C) 2006-2011, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <IDBT.h>
#include <AutoTest.h>
#include <LOGIntfLayer.h>
//#define TM_CONFIG_DOWNLOAD
#if  defined(_GYY_I5_) || defined(TM_CONFIG_DOWNLOAD) || defined(_GYY_I6_)
#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE 1
#endif

#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <signal.h>

#include "polarssl/config.h"
#include "polarssl/net.h"
#include "polarssl/ssl.h"
#include "polarssl/entropy.h"
#include "polarssl/ctr_drbg.h"
#include "polarssl/error.h"

#define SERVER_PORT 4433
#define SERVER_NAME "localhost"
//#define GET_REQUEST "GET %s HTTP/1.0\r\n\r\n"
#define GET_REQUEST "GET %s HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-cn\r\n\
User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n\
Host: %s:%d\r\nConnection: Close\r\n\r\n"

#define DEBUG_LEVEL 0


void my_debug( void *ctx, int level, const char *str )
{
    if( level < DEBUG_LEVEL )
    {
        fprintf( (FILE *) ctx, "%s", str );
        fflush(  (FILE *) ctx  );
    }
}

char* mystrstr1(const char *s1, const char *s2,int *outlen)
{
    int n;
    int len = 0;
    int len1 = 0;
    if (*s2)
    {
    	len1 = strlen(s2);
        while (*s1)
        {
            for (n=0; *(s1 + n) == *(s2 + n); n++)
            {
                if (!*(s2 + n + 1))
                {
                	*outlen = len + len1;
                    return (char *)s1;
                }
            }
            len++;
            s1++;
        }
        return NULL;
    }
    else
        return (char *)s1;
}

char* mystrstr(const char *s1, const char *s2,int *outlen)
{
    int n;
    int len = 0;
    if (*s2)
    {
        while (*s1)
        {
            for (n=0; *(s1 + n) == *(s2 + n); n++)
            {
                if (!*(s2 + n + 1))
                {
                	*outlen = len;
                    return (char *)s1;
                }
            }
            len++;
            s1++;
        }
        return NULL;
    }
    else
        return (char *)s1;
}

int StringFindFirst(unsigned char* pvBuffer, int iSize, char c)
{
    int i = 0;
    for(i = 0; i < iSize; i++)
    {
        if(pvBuffer[i] == c)
        {
            return i + 1;
        }
    }
    return 0;
}

/********************************************
功能：搜索字符串右边起的第一个匹配字符
********************************************/
char *Rstrchr(char *s, char x)
{
    int i = strlen(s);
    if (!(*s))
        return 0;
    while (s[i - 1])
        if (strchr(s + (i - 1), x))
            return (s + (i - 1));
        else
            i--;
    return 0;
}

/**************************************************************
功能：从字符串src中分析出网站地址和端口，并得到用户要下载的文件
***************************************************************/
int GetHost(char *src, char *web, char *file, int *port,int *flag)
{
    char *pA;
    char *pB;
    char acWeb[64];
    memset(web, 0, sizeof(web));
    memset(file, 0, sizeof(file));
    *port = 0;
    if (!(*src))
        return -1;
    pA = src;
    if (!strncmp(pA, "http://", strlen("http://")))
    {
        pA = src + strlen("http://");
        if(flag)
        	*flag = 0;
    }
    else if (!strncmp(pA, "https://", strlen("https://")))
    {
        pA = src + strlen("https://");
        if(flag)
        	*flag = 1;
    }
    else
    	return -1;
    pB = strchr(pA, '/');
    if (pB) {
        memcpy(web, pA, strlen(pA) - strlen(pB));
        if (pB + 1) {
            memcpy(file, pB , strlen(pB) );
            file[strlen(pB) ] = 0;
        }
    } else
        memcpy(web, pA, strlen(pA));
    if (pB)
    {
    	strcpy(acWeb,web);
    	web = strtok(web,":");
        //web[strlen(pA) - strlen(pB)] = 0;
    }
    else
        web[strlen(pA)] = 0;
    pA = strchr(acWeb, ':');
    if (pA)
    {
        *port = atoi(pA + 1);
    }
    else
    {
    	if(*flag == 1)
    	{
            *port = 443;
    	}
    	else
    	{
            *port = 80;
    	}
    }
    return 0;
}

int FindParamFromBuf(const char *buf_in, const char *pItemStr, char *buf_out)
{
    int	ret = -1;
    int i = 0, j= 0;
    char strBuf[128] = {0};

    sprintf(strBuf,"<%s>",pItemStr);
    if(mystrstr1(buf_in,strBuf,&i) == NULL)
    {
    	return -1;
    }
    sprintf(strBuf,"</%s>",pItemStr);
    if(mystrstr(buf_in,strBuf,&j) == NULL)
    {
    	return -1;
    }

    memcpy(buf_out,buf_in+i,j-i);

    buf_out[j-i] = 0x0;

    return 0;
}

char host_file[1024];
int GetFromHttps(const char *pcConfigUrl,const char* pcUserName, const char* pcPasswd,const char* pcFileName)
{
    int ret, len, server_fd;
    unsigned char buf[1024];
    char *pers = "ssl_client1";
    char host_addr[256] = {0x0};
    char file_name[256] = {0x0};
    int port;
    int i = 0,j = 0;
    char *removehead;
    int flag = 0;
    FILE *fp = NULL;
    entropy_context entropy;
    ctr_drbg_context ctr_drbg;
    ssl_context ssl;
	struct timeval tv;
	fd_set readfds;
	tv.tv_sec = 120;
	tv.tv_usec = 0;
    memset( &ssl, 0, sizeof( ssl ) );
    if(GetHost(pcConfigUrl,host_addr, host_file, &port,&flag) != 0)        //分析网址、端口、文件名等
    {
    	goto exit;
    }

    //
    // 1. Start the connection
    //
    LOG_RUNLOG_DEBUG( "  . Connecting to tcp/%s/%4d/%s/%d/%s/...", host_addr,
                                               port ,host_file,flag,pcFileName);
    fflush( stdout );

//    if( ( ret = net_connect( &server_fd, host_addr,
//                                         port ) ) != 0 )
//    {
//        LOG_RUNLOG_DEBUG( " failed\n  ! net_connect returned %d\n\n", ret );
//        goto exit;
//    }
    server_fd = ATConnect(host_addr,port,0);
	if(!server_fd)
	{
		ret = -1;
		goto exit;
	}

    LOG_RUNLOG_DEBUG( " ok\n" );
    //
    // 0. Initialize the RNG and the session data
    //
    if(flag)
	{
		memset( &ssl, 0, sizeof( ssl_context ) );

		LOG_RUNLOG_DEBUG( "\n  . Seeding the random number generator..." );
		fflush( stdout );

		entropy_init( &entropy );
		if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
								   (unsigned char *) pers, strlen( pers ) ) ) != 0 )
		{
			LOG_RUNLOG_DEBUG( " failed\n  ! ctr_drbg_init returned %d\n", ret );
			goto exit;
		}

		LOG_RUNLOG_DEBUG( " ok\n" );

    //
    //2. Setup stuff
    //
		LOG_RUNLOG_DEBUG( "  . Setting up the SSL/TLS structure..." );
		fflush( stdout );

		if( ( ret = ssl_init( &ssl ) ) != 0 )
		{
			LOG_RUNLOG_DEBUG( " failed\n  ! ssl_init returned %d\n\n", ret );
			goto exit;
		}

		LOG_RUNLOG_DEBUG( " ok\n" );

		ssl_set_endpoint( &ssl, SSL_IS_CLIENT );
		ssl_set_authmode( &ssl, SSL_VERIFY_NONE );

		ssl_set_rng( &ssl, ctr_drbg_random, &ctr_drbg );
		ssl_set_dbg( &ssl, my_debug, stdout );
		ssl_set_bio( &ssl, net_recv, &server_fd,
						   net_send, &server_fd );
    }
    //
    // 3. Write the GET request
    //
    LOG_RUNLOG_DEBUG( "  > Write to server:" );
    fflush( stdout );

    len = sprintf( (char *) buf, GET_REQUEST ,host_file,host_addr,port);

    if(flag)
    {
		while( ( ret = ssl_write( &ssl, buf, len ) ) <= 0 )
		{
			if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
			{
				LOG_RUNLOG_DEBUG( " failed\n  ! ssl_write returned %d\n\n", ret );
				goto exit;
			}
		}
    }
    else
    {
    	while( ( ret = send(server_fd, buf, len, 0)) <= 0 )
		{
			if( ret == -1)
			{
				LOG_RUNLOG_DEBUG( " failed\n  ! socket_write returned %d\n\n", ret );
				goto exit;
			}
		}
    }

    len = ret;
    LOG_RUNLOG_DEBUG( " %d bytes written\n\n%s", len, (char *) buf );

    //
    // 7. Read the HTTP response
    //
    LOG_RUNLOG_DEBUG( "  < Read from server:" );
    fflush( stdout );

    i = 0;
    if(!flag)
   {
		setsockopt((int)server_fd, SOL_SOCKET, SO_RCVTIMEO, (void *) &tv, sizeof(struct timeval)) ;
   }
    // 连接成功了，接收https响应，response
    do
    {
        len = sizeof( buf ) - 1;
        memset( buf, 0, sizeof( buf ) );
        if(flag)
        {
			ret = ssl_read( &ssl, buf, len );
        }
        else
        {
        	ret = recv(server_fd, buf, len, 0);
        }
		if( ret == POLARSSL_ERR_NET_WANT_READ || ret == POLARSSL_ERR_NET_WANT_WRITE )
			continue;

		if( ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY )
		{
			ret = 0;
			break;
		}
		if( ret < 0 )
		{
			LOG_RUNLOG_DEBUG( "failed\n  ! ssl_read returned %d\n\n", ret );
			break;
		}

		if( ret == 0 )
		{
			LOG_RUNLOG_DEBUG( "\n\nEOF\n\n" );
			break;
		}

//        LOG_RUNLOG_DEBUG("%s",buf);
        if (i == 0)
        {
            //-------------add by hb
			if(mystrstr1(buf,"filename=",&i) != NULL)
			{
				mystrstr(buf+i,"\r\n",&j);
				memcpy(&file_name,buf+i,j);
				file_name[j] = 0x0;
			}
//			else
//			{
//				ret = -1;
////				goto exit;
//			}

            if(pcFileName)
            {
            	i = strlen(pcFileName);
            	if((pcFileName[i - 1] == '/' || pcFileName[i - 1] == '\\') && file_name[0] != 0x0)
            	{
            		sprintf(host_file,"%s%s",pcFileName,file_name);
            	}
            	else
            	{
            		strcpy(host_file,pcFileName);
            	}
            }
//            else
//            {
//            	ret = -1;
//            }
            if(host_file[0] == 0x0)
            {
            	ret = -1;
				LOG_RUNLOG_DEBUG( "\n\nfilename is error\n\n" );
				goto exit;
            }

            fp = fopen(host_file, "w+");
            if(!fp)
            {
            	ret = -1;
            	LOG_RUNLOG_DEBUG( "\n\nfilename is error\n\n" );
            	goto exit;
            }
            //-------------add by hb

        	mystrstr(buf,"\r\n\r\n",&i);
        	fwrite(buf + i + 4, ret - i - 4, 1, fp);        //将https主体信息写入文件
			fflush(fp);
			if(ret < len)
			{
				ret = 0;
				break;
			}
			continue;
		}
        else
        {
			fwrite(buf, ret, 1, fp);        //将https主体信息写入文件
			fflush(fp);        //每1K时存盘一次
		}
//        //-------------add by hb
//        fwrite(buf,len,1,fp);
//        //-------------add by hb
    }
    while( 1 );

    if(flag)
    {
    	ssl_close_notify( &ssl );
    }

exit:
	//-------------add by hb
	if(fp != NULL)
	{
		fclose(fp);
	}
	//-------------add by hb
//#ifdef POLARSSL_ERROR_C
//    if( ret != 0 )
//    {
//        char error_buf[100];
//        error_strerror( ret, error_buf, 100 );
//        LOG_RUNLOG_DEBUG("Last error was: %d - %s\n\n", ret, error_buf );
//    }
//#endif

    net_close( server_fd );
    if(flag)
    {
		ssl_free( &ssl );

		memset( &ssl, 0, sizeof( ssl ) );
    }

#if defined(_WIN32)
    printf( "  + Press Enter to exit this program.\n" );
    fflush( stdout ); getchar();
#endif

    return( ret );
}

char *GetDownloadFileName()
{
	return host_file;
}

#endif

///*
// *  SSL client with certificate authentication
// *
// *  Copyright (C) 2006-2011, Brainspark B.V.
// *
// *  This file is part of PolarSSL (http://www.polarssl.org)
// *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
// *
// *  All rights reserved.
// *
// *  This program is free software; you can redistribute it and/or modify
// *  it under the terms of the GNU General Public License as published by
// *  the Free Software Foundation; either version 2 of the License, or
// *  (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// *  GNU General Public License for more details.
// *
// *  You should have received a copy of the GNU General Public License along
// *  with this program; if not, write to the Free Software Foundation, Inc.,
// *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
// */
//
//#ifndef _CRT_SECURE_NO_DEPRECATE
//#define _CRT_SECURE_NO_DEPRECATE 1
//#endif
//
//#include <string.h>
//#include <stdlib.h>
//#include <stdio.h>
//
//#include "polarssl/config.h"
//
//#include "polarssl/net.h"
//#include "polarssl/ssl.h"
//#include "polarssl/entropy.h"
//#include "polarssl/ctr_drbg.h"
//#include "polarssl/certs.h"
//#include "polarssl/x509.h"
//#include "polarssl/error.h"
//
//#define DFL_SERVER_NAME         "192.168.0.9"
//#define DFL_SERVER_PORT         8543
//#define DFL_REQUEST_PAGE        "/imsms-acs/download/405"
//#define DFL_DEBUG_LEVEL         0
//#define DFL_CA_FILE             ""
//#define DFL_CA_PATH             ""
//#define DFL_CRT_FILE            ""
//#define DFL_KEY_FILE            ""
//#define DFL_FORCE_CIPHER        0
//#define DFL_RENEGOTIATION       SSL_RENEGOTIATION_ENABLED
//#define DFL_ALLOW_LEGACY        SSL_LEGACY_NO_RENEGOTIATION
//#define DFL_MIN_VERSION         -1
//#define DFL_MAX_VERSION         -1
////https://192.168.0.9:8543/imsms-acs/download/405
//#define GET_REQUEST "GET %s HTTP/1.0\r\n\r\n"
//
///*
// * global options
// */
//struct options
//{
//    char *server_name;          /* hostname of the server (client only)     */
//    int server_port;            /* port on which the ssl service runs       */
//    int debug_level;            /* level of debugging                       */
//    char *request_page;         /* page on server to request                */
//    char *ca_file;              /* the file with the CA certificate(s)      */
//    char *ca_path;              /* the path with the CA certificate(s) reside */
//    char *crt_file;             /* the file with the client certificate     */
//    char *key_file;             /* the file with the client key             */
//    int force_ciphersuite[2];   /* protocol/ciphersuite to use, or all      */
//    int renegotiation;          /* enable / disable renegotiation           */
//    int allow_legacy;           /* allow legacy renegotiation               */
//    int min_version;            /* minimum protocol version accepted        */
//    int max_version;            /* maximum protocol version accepted        */
//} opt;
//
//void my_debug( void *ctx, int level, const char *str )
//{
//    if( level < opt.debug_level )
//    {
//        fprintf( (FILE *) ctx, "%s", str );
//        fflush(  (FILE *) ctx  );
//    }
//}
//
// * Enabled if debug_level > 1 in code below
// */
//int my_verify( void *data, x509_cert *crt, int depth, int *flags )
//{
//    char buf[1024];
//    ((void) data);
//
//    printf( "\nVerify requested for (Depth %d):\n", depth );
//    x509parse_cert_info( buf, sizeof( buf ) - 1, "", crt );
//    printf( "%s", buf );
//
//    if( ( (*flags) & BADCERT_EXPIRED ) != 0 )
//        printf( "  ! server certificate has expired\n" );
//
//    if( ( (*flags) & BADCERT_REVOKED ) != 0 )
//        printf( "  ! server certificate has been revoked\n" );
//
//    if( ( (*flags) & BADCERT_CN_MISMATCH ) != 0 )
//        printf( "  ! CN mismatch\n" );
//
//    if( ( (*flags) & BADCERT_NOT_TRUSTED ) != 0 )
//        printf( "  ! self-signed or not signed by a trusted CA\n" );
//
//    if( ( (*flags) & BADCRL_NOT_TRUSTED ) != 0 )
//        printf( "  ! CRL not trusted\n" );
//
//    if( ( (*flags) & BADCRL_EXPIRED ) != 0 )
//        printf( "  ! CRL expired\n" );
//
//    if( ( (*flags) & BADCERT_OTHER ) != 0 )
//        printf( "  ! other (unknown) flag\n" );
//
//    if ( ( *flags ) == 0 )
//        printf( "  This certificate has no flags\n" );
//
//    return( 0 );
//}
//
//#if defined(POLARSSL_FS_IO)
//#define USAGE_IO \
//    "    ca_file=%%s          default: \"\" (pre-loaded)\n" \
//    "    ca_path=%%s          default: \"\" (pre-loaded) (overrides ca_file)\n" \
//    "    crt_file=%%s         default: \"\" (pre-loaded)\n" \
//    "    key_file=%%s         default: \"\" (pre-loaded)\n"
//#else
//#define USAGE_IO \
//    "    No file operations available (POLARSSL_FS_IO not defined)\n"
//#endif /* POLARSSL_FS_IO */
//
//#define USAGE \
//    "\n usage: ssl_client2 param=<>...\n"                   \
//    "\n acceptable parameters:\n"                           \
//    "    server_name=%%s      default: localhost\n"         \
//    "    server_port=%%d      default: 4433\n"              \
//    "    debug_level=%%d      default: 0 (disabled)\n"      \
//    USAGE_IO                                                \
//    "    request_page=%%s     default: \".\"\n"             \
//    "    renegotiation=%%d    default: 1 (enabled)\n"       \
//    "    allow_legacy=%%d     default: 0 (disabled)\n"      \
//    "\n"                                                    \
//    "    min_version=%%s      default: \"\" (ssl3)\n"       \
//    "    max_version=%%s      default: \"\" (tls1_2)\n"     \
//    "    force_version=%%s    default: \"\" (none)\n"       \
//    "                        options: ssl3, tls1, tls1_1, tls1_2\n" \
//    "\n"                                                    \
//    "    force_ciphersuite=<name>    default: all enabled\n"\
//    " acceptable ciphersuite names:\n"
//
//int GetFromHttps(const char *pcConfigUrl,const char* pcUserName, const char* pcPasswd,const char* pcFileName)
//{
//    int ret = 0, len, server_fd;
//    unsigned char buf[1024];
//    char *pers = "ssl_client2";
//
//    entropy_context entropy;
//    ctr_drbg_context ctr_drbg;
//    ssl_context ssl;
//    x509_cert cacert;
//    x509_cert clicert;
//    rsa_context rsa;
//    int i;
//    char *p, *q;
//    const int *list;
//    char host_addr[256];
//    char host_file[1024];
//    /*
//     * Make sure memory references are valid.
//     */
//    server_fd = 0;
//    memset( &ssl, 0, sizeof( ssl_context ) );
//    memset( &cacert, 0, sizeof( x509_cert ) );
//    memset( &clicert, 0, sizeof( x509_cert ) );
//    memset( &rsa, 0, sizeof( rsa_context ) );
//
//    opt.server_name         = DFL_SERVER_NAME;
//    opt.server_port         = DFL_SERVER_PORT;
//    opt.debug_level         = DFL_DEBUG_LEVEL;
//    opt.request_page        = DFL_REQUEST_PAGE;
//    opt.ca_file             = DFL_CA_FILE;
//    opt.ca_path             = DFL_CA_PATH;
//    opt.crt_file            = DFL_CRT_FILE;
//    opt.key_file            = DFL_KEY_FILE;
//    opt.force_ciphersuite[0]= DFL_FORCE_CIPHER;
//    opt.renegotiation       = DFL_RENEGOTIATION;
//    opt.allow_legacy        = DFL_ALLOW_LEGACY;
//    opt.min_version         = DFL_MIN_VERSION;
//    opt.max_version         = DFL_MAX_VERSION;
//
//    GetHost(pcConfigUrl,host_addr, host_file, &opt.server_port);        /*分析网址、端口、文件名等 */
//	opt.server_name = &host_addr;
//	opt.request_page = &host_file;
//    /*
//     * 0. Initialize the RNG and the session data
//     */
//    printf( "\n  . Seeding the random number generator..." );
//    fflush( stdout );
//
//    entropy_init( &entropy );
//    if( ( ret = ctr_drbg_init( &ctr_drbg, entropy_func, &entropy,
//                               (unsigned char *) pers, strlen( pers ) ) ) != 0 )
//    {
//        printf( " failed\n  ! ctr_drbg_init returned -0x%x\n", -ret );
//        goto exit;
//    }
//
//    printf( " ok\n" );
//
//    /*
//     * 1.1. Load the trusted CA
//     */
//    printf( "  . Loading the CA root certificate ..." );
//    fflush( stdout );
//
//#if defined(POLARSSL_FS_IO)
//    if( strlen( opt.ca_path ) )
//        ret = x509parse_crtpath( &cacert, opt.ca_path );
//    else if( strlen( opt.ca_file ) )
//        ret = x509parse_crtfile( &cacert, opt.ca_file );
//    else
//#endif
//#if defined(POLARSSL_CERTS_C)
//        ret = x509parse_crt( &cacert, (unsigned char *) test_ca_crt,
//                strlen( test_ca_crt ) );
//#else
//    {
//        ret = 1;
//        printf("POLARSSL_CERTS_C not defined.");
//    }
//#endif
//    if( ret < 0 )
//    {
//        printf( " failed\n  !  x509parse_crt returned -0x%x\n\n", -ret );
//        goto exit;
//    }
//
//    printf( " ok (%d skipped)\n", ret );
//
//    /*
//     * 1.2. Load own certificate and private key
//     *
//     * (can be skipped if client authentication is not required)
//     */
//    printf( "  . Loading the client cert. and key..." );
//    fflush( stdout );
//
//#if defined(POLARSSL_FS_IO)
//    if( strlen( opt.crt_file ) )
//        ret = x509parse_crtfile( &clicert, opt.crt_file );
//    else
//#endif
//#if defined(POLARSSL_CERTS_C)
//        ret = x509parse_crt( &clicert, (unsigned char *) test_cli_crt,
//                strlen( test_cli_crt ) );
//#else
//    {
//        ret = 1;
//        printf("POLARSSL_CERTS_C not defined.");
//    }
//#endif
//    if( ret != 0 )
//    {
//        printf( " failed\n  !  x509parse_crt returned -0x%x\n\n", -ret );
//        goto exit;
//    }
//
//#if defined(POLARSSL_FS_IO)
//    if( strlen( opt.key_file ) )
//        ret = x509parse_keyfile( &rsa, opt.key_file, "" );
//    else
//#endif
//#if defined(POLARSSL_CERTS_C)
//        ret = x509parse_key( &rsa, (unsigned char *) test_cli_key,
//                strlen( test_cli_key ), NULL, 0 );
//#else
//    {
//        ret = 1;
//        printf("POLARSSL_CERTS_C not defined.");
//    }
//#endif
//    if( ret != 0 )
//    {
//        printf( " failed\n  !  x509parse_key returned -0x%x\n\n", -ret );
//        goto exit;
//    }
//
//    printf( " ok\n" );
//
//    /*
//     * 2. Start the connection
//     */
//    printf( "  . Connecting to tcp/%s/%-4d...", opt.server_name,
//                                                opt.server_port );
//    fflush( stdout );
//
//    if( ( ret = net_connect( &server_fd, opt.server_name,
//                                         opt.server_port ) ) != 0 )
//    {
//        printf( " failed\n  ! net_connect returned -0x%x\n\n", -ret );
//        goto exit;
//    }
//
//    printf( " ok\n" );
//
//    /*
//     * 3. Setup stuff
//     */
//    printf( "  . Setting up the SSL/TLS structure..." );
//    fflush( stdout );
//
//    if( ( ret = ssl_init( &ssl ) ) != 0 )
//    {
//        printf( " failed\n  ! ssl_init returned -0x%x\n\n", -ret );
//        goto exit;
//    }
//
//    printf( " ok\n" );
//
//    if( opt.debug_level > 0 )
//        ssl_set_verify( &ssl, my_verify, NULL );
//
//    ssl_set_endpoint( &ssl, SSL_IS_CLIENT );
//    ssl_set_authmode( &ssl, SSL_VERIFY_OPTIONAL );
//
//    ssl_set_rng( &ssl, ctr_drbg_random, &ctr_drbg );
//    ssl_set_dbg( &ssl, my_debug, stdout );
//    ssl_set_bio( &ssl, net_recv, &server_fd,
//                       net_send, &server_fd );
//
//    if( opt.force_ciphersuite[0] != DFL_FORCE_CIPHER )
//        ssl_set_ciphersuites( &ssl, opt.force_ciphersuite );
//
//    ssl_set_renegotiation( &ssl, opt.renegotiation );
//    ssl_legacy_renegotiation( &ssl, opt.allow_legacy );
//
//    ssl_set_ca_chain( &ssl, &cacert, NULL, opt.server_name );
//    ssl_set_own_cert( &ssl, &clicert, &rsa );
//
//    ssl_set_hostname( &ssl, opt.server_name );
//
//    if( opt.min_version != -1 )
//        ssl_set_min_version( &ssl, SSL_MAJOR_VERSION_3, opt.min_version );
//    if( opt.max_version != -1 )
//        ssl_set_max_version( &ssl, SSL_MAJOR_VERSION_3, opt.max_version );
//
//    /*
//     * 4. Handshake
//     */
//    printf( "  . Performing the SSL/TLS handshake..." );
//    fflush( stdout );
//
//    while( ( ret = ssl_handshake( &ssl ) ) != 0 )
//    {
//        if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
//        {
//            printf( " failed\n  ! ssl_handshake returned -0x%x\n\n", -ret );
//            goto exit;
//        }
//    }
//
//    printf( " ok\n    [ Ciphersuite is %s ]\n",
//            ssl_get_ciphersuite( &ssl ) );
//
//    /*
//     * 5. Verify the server certificate
//     */
//    printf( "  . Verifying peer X.509 certificate..." );
//
//    if( ( ret = ssl_get_verify_result( &ssl ) ) != 0 )
//    {
//        printf( " failed\n" );
//
//        if( ( ret & BADCERT_EXPIRED ) != 0 )
//            printf( "  ! server certificate has expired\n" );
//
//        if( ( ret & BADCERT_REVOKED ) != 0 )
//            printf( "  ! server certificate has been revoked\n" );
//
//        if( ( ret & BADCERT_CN_MISMATCH ) != 0 )
//            printf( "  ! CN mismatch (expected CN=%s)\n", opt.server_name );
//
//        if( ( ret & BADCERT_NOT_TRUSTED ) != 0 )
//            printf( "  ! self-signed or not signed by a trusted CA\n" );
//
//        printf( "\n" );
//    }
//    else
//        printf( " ok\n" );
//
//    printf( "  . Peer certificate information    ...\n" );
//    x509parse_cert_info( (char *) buf, sizeof( buf ) - 1, "      ",
//                         ssl_get_peer_cert( &ssl ) );
//    printf( "%s\n", buf );
//
//    /*
//     * 6. Write the GET request
//     */
//    printf( "  > Write to server:" );
//    fflush( stdout );
//
//    len = sprintf( (char *) buf, GET_REQUEST, opt.request_page );
//
//    while( ( ret = ssl_write( &ssl, buf, len ) ) <= 0 )
//    {
//        if( ret != POLARSSL_ERR_NET_WANT_READ && ret != POLARSSL_ERR_NET_WANT_WRITE )
//        {
//            printf( " failed\n  ! ssl_write returned -0x%x\n\n", -ret );
//            goto exit;
//        }
//    }
//
//    len = ret;
//    printf( " %d bytes written\n\n%s", len, (char *) buf );
//
//    /*
//     * 7. Read the HTTP response
//     */
//    printf( "  < Read from server:" );
//    fflush( stdout );
//
////    //-------------add by hb
////    FILE *fp = NULL;
////    fp = fopen(pcFileName, "w+");
////    //-------------add by hb
//
//    do
//    {
//        len = sizeof( buf ) - 1;
//        memset( buf, 0, sizeof( buf ) );
//        ret = ssl_read( &ssl, buf, len );
//
////        //-------------add by hb
////        fwrite(buf,len,1,fp);
////        //-------------add by hb
//
//        if( ret == POLARSSL_ERR_NET_WANT_READ || ret == POLARSSL_ERR_NET_WANT_WRITE )
//            continue;
//
//        if( ret == POLARSSL_ERR_SSL_PEER_CLOSE_NOTIFY )
//            break;
//
//        if( ret < 0 )
//        {
//            printf( "failed\n  ! ssl_read returned -0x%x\n\n", -ret );
//            break;
//        }
//
//        if( ret == 0 )
//        {
//            printf("\n\nEOF\n\n");
//            break;
//        }
//
//        len = ret;
//        printf( " %d bytes read\n\n%s", len, (char *) buf );
//    }
//    while( 1 );
//
//    ssl_close_notify( &ssl );
//
//exit:
////	//-------------add by hb
////	if(fp != NULL)
////		fclose(fp);
////	//-------------add by hb
//#ifdef POLARSSL_ERROR_C
//    if( ret != 0 )
//    {
//        char error_buf[100];
//        error_strerror( ret, error_buf, 100 );
//        printf("Last error was: -0x%X - %s\n\n", -ret, error_buf );
//    }
//#endif
//
//    if( server_fd )
//        net_close( server_fd );
//    x509_free( &clicert );
//    x509_free( &cacert );
//    rsa_free( &rsa );
//    ssl_free( &ssl );
//
//    memset( &ssl, 0, sizeof( ssl ) );
//
//#if defined(_WIN32)
//    printf( "  + Press Enter to exit this program.\n" );
//    fflush( stdout ); getchar();
//#endif
//
//    return( ret );
//}



//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <errno.h>
//#include <unistd.h>
//#include <netinet/in.h>
//#include <limits.h>
//#include <netdb.h>
//#include <arpa/inet.h>
//#include <ctype.h>
//#include <openssl/crypto.h>
//#include <openssl/ssl.h>
//#include <openssl/err.h>
//#include <openssl/rand.h>
//
//#define DEBUG 0
//
//
//int GetFromHttps(const char *pcConfigUrl,const char* pcUserName, const char* pcPasswd,const char* pcFileName)
//{
//    int sockfd, ret;
//    char buffer[1024];
//    struct sockaddr_in server_addr;
//    struct hostent *host;
//    int portnumber, nbytes;
//    char host_addr[256];
//    char host_file[1024];
//    char local_file[256];
//    FILE *fp;
//    char request[1024];
//    int send, totalsend;
//    int i;
//    char *pt;
//    SSL *ssl;
//    SSL_CTX *ctx;
//
//    if (DEBUG)
//        printf("parameter.1 is: %s\n", "https://192.168.0.9:8543/imsms-acs/download/405");
//
//    GetHost(pcConfigUrl, host_addr, host_file, &portnumber);        /*分析网址、端口、文件名等 */
//    if (DEBUG)
//        printf("webhost:%s\n", host_addr);
//    if (DEBUG)
//        printf("hostfile:%s\n", host_file);
//    if (DEBUG)
//        printf("portnumber:%d\n\n", portnumber);
//
//    if ((host = gethostbyname(host_addr)) == NULL) {        /*取得主机IP地址 */
//        if (DEBUG)
//            fprintf(stderr, "Gethostname error, %s\n", strerror(errno));
//        return 1;
//    }
//
//    /* 客户程序开始建立 sockfd描述符 */
//    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {        /*建立SOCKET连接 */
//        if (DEBUG)
//            fprintf(stderr, "Socket Error:%s\a\n", strerror(errno));
//        return 1;
//    }
//
//    /* 客户程序填充服务端的资料 */
//    bzero(&server_addr, sizeof(server_addr));
//    server_addr.sin_family = AF_INET;
//    server_addr.sin_port = htons(portnumber);
//    server_addr.sin_addr = *((struct in_addr *) host->h_addr);
//
//    /* 客户程序发起连接请求 */
//    if (connect(sockfd, (struct sockaddr *) (&server_addr), sizeof(struct sockaddr)) == -1) {        /*连接网站 */
//        if (DEBUG)
//            fprintf(stderr, "Connect Error:%s\a\n", strerror(errno));
//        return 1;
//    }
//
//    /* SSL初始化 */
//    SSL_library_init();
//    SSL_load_error_strings();
//    ctx = SSL_CTX_new(SSLv3_client_method());
//    if (ctx == NULL) {
//        ERR_print_errors_fp(stderr);
//        return 1;
//    }
//
//    ssl = SSL_new(ctx);
//    if (ssl == NULL) {
//        ERR_print_errors_fp(stderr);
//        return 1;
//    }
//
//    /* 把socket和SSL关联 */
//    ret = SSL_set_fd(ssl, sockfd);
//    if (ret == 0) {
//        ERR_print_errors_fp(stderr);
//        return 1;
//    }
//
//    RAND_poll();
//    while (RAND_status() == 0) {
//        unsigned short rand_ret = rand() % 65536;
//        RAND_seed(&rand_ret, sizeof(rand_ret));
//    }
//
//    ret = SSL_connect(ssl);
//    if (ret != 1) {
//        ERR_print_errors_fp(stderr);
//        return 1;
//    }
//
//    sprintf(request, "GET /%s HTTP/1.1\r\nAccept: */*\r\nAccept-Language: zh-cn\r\n\
//User-Agent: Mozilla/4.0 (compatible; MSIE 5.01; Windows NT 5.0)\r\n\
//Host: %s:%d\r\nConnection: Close\r\n\r\n", host_file, host_addr,
//            portnumber);
//    if (DEBUG)
//        printf("%s", request);        /*准备request，将要发送给主机 */
//
//    /*取得真实的文件名 */
//    if (host_file && *host_file)
//        pt = Rstrchr(host_file, '/');
//    else
//        pt = 0;
//
//    memset(local_file, 0, sizeof(local_file));
//    if(pcFileName != NULL)
//    {
//    	strcpy(local_file, pcFileName);
//    }
////    if (pt && *pt) {
////        if ((pt + 1) && *(pt + 1))
////            strcpy(local_file, pt + 1);
////        else
////            memcpy(local_file, host_file, strlen(host_file) - 1);
////    } else if (host_file && *host_file)
////        strcpy(local_file, host_file);
////    else
////        strcpy(local_file, "index.html");
//    if (DEBUG)
//        printf("local filename to write:%s\n\n", local_file);
//
//    /*发送https请求request */
//    send = 0;
//    totalsend = 0;
//    nbytes = strlen(request);
//    while (totalsend < nbytes) {
//        send = SSL_write(ssl, request + totalsend, nbytes - totalsend);
//        if (send == -1) {
//            if (DEBUG)
//                ERR_print_errors_fp(stderr);
//            exit(0);
//        }
//        totalsend += send;
//        if (DEBUG)
//            printf("%d bytes send OK!\n", totalsend);
//    }
//
//    fp = fopen(local_file, "w+");
//    if (!fp) {
//        if (DEBUG)
//            printf("create file error! %s\n", strerror(errno));
//        return 0;
//    }
//    if (DEBUG)
//        printf("\nThe following is the response header:\n");
//    i = 0;
//    /* 连接成功了，接收https响应，response */
//    while ((nbytes = SSL_read(ssl, buffer, 1)) == 1) {
//        if (i < 4) {
//            if (buffer[0] == '\r' || buffer[0] == '\n')
//                i++;
//            else
//                i = 0;
//            if (DEBUG)
//                printf("%c", buffer[0]);        /*把https头信息打印在屏幕上 */
//        } else {
//            fwrite(buffer, 1, 1, fp);        /*将https主体信息写入文件 */
//            i++;
//            if (i % 1024 == 0)
//                fflush(fp);        /*每1K时存盘一次 */
//        }
//    }
//    fclose(fp);
//    /* 结束通讯 */
//    ret = SSL_shutdown(ssl);
//    if (ret != 1) {
//        ERR_print_errors_fp(stderr);
//        return 1;
//    }
//    close(sockfd);
//    SSL_free(ssl);
//    SSL_CTX_free(ctx);
//    ERR_free_strings();
//    return 0;
//}
//
