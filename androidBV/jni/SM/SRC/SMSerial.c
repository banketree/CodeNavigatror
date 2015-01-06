#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "../INC/SMSerial.h"

#define TIMEOUT_SEC(buflen, baud)       	2
#define TIMEOUT_USEC    					0
#define READ_TRY_AGAIN_TIMES  				3
#define WRITE_TRY_AGAIN_TIMES 				3

static struct termios termios_new;

int BAUDRATE(int iBaudrate)
{
	switch (iBaudrate)
	{
	case 1200:
		return B1200;
	case 2400:
		return B2400;
	case 4800:
		return B4800;
	case 9600:
		return B9600;
	case 19200:
		return B19200;
	case 38400:
		return B38400;
	case 57600:
		return B57600;
	case 115200:
		return B115200;
	default:
		return B9600;
	}
}

int _BAUDRATE(int iBaudrate)
{
	/* reverse iBaudrate */
	switch (iBaudrate)
	{
	case B1200:
		return 1200;
	case B2400:
		return 2400;
	case B4800:
		return 4800;
	case B9600:
		return 9600;
	case B19200:
		return 19200;
	case B38400:
		return 38400;
	case B57600:
		return 57600;
	case B115200:
		return 115200;
	default:
		return (9600);
	}
}

/* get serial port iBaudrate */
int SMGetBaudrate()
{
	return _BAUDRATE(cfgetospeed(&termios_new));
}

/* set serial port iBaudrate by use of file descriptor iFd */
void SMSetBaudrate(int iBaudrate)
{
	termios_new.c_cflag = BAUDRATE(iBaudrate); /* set iBaudrate */
}

void SMSetDataBit(int iDatabit)
{
	termios_new.c_cflag &= ~CSIZE;
	switch (iDatabit)
	{
	case 8:
		termios_new.c_cflag |= CS8;
		break;
	case 7:
		termios_new.c_cflag |= CS7;
		break;
	case 6:
		termios_new.c_cflag |= CS6;
		break;
	case 5:
		termios_new.c_cflag |= CS5;
		break;
	default:
		termios_new.c_cflag |= CS8;
		break;
	}
}

void SMSetStopBit(const char *pcStopbit)
{
	if (0 == strcmp(pcStopbit, "1"))
	{
		termios_new.c_cflag &= ~CSTOPB; /* 1 stop bit */
	}
	else if (0 == strcmp(pcStopbit, "1.5"))
	{
		termios_new.c_cflag &= ~CSTOPB; /* 1.5 stop bits */
	}
	else if (0 == strcmp(pcStopbit, "2"))
	{
		termios_new.c_cflag |= CSTOPB; /* 2 stop bits */
	}
	else
	{
		termios_new.c_cflag &= ~CSTOPB; /* 1 stop bit */
	}
}

void SMSetParityCheck(char cParity)
{
	switch (cParity)
	{
	case 'N': /* no cParity check */
		termios_new.c_cflag &= ~PARENB;
		break;
	case 'E': /* even */
		termios_new.c_cflag |= PARENB;
		termios_new.c_cflag &= ~PARODD;
		break;
	case 'O': /* odd */
		termios_new.c_cflag |= PARENB;
		termios_new.c_cflag |= ~PARODD;
		break;
	default: /* no cParity check */
		termios_new.c_cflag &= ~PARENB;
		break;
	}
}

int SMSetPortAttr(int iFd, int iBaudrate, int iDatabit, const char *pcStopbit,
		char cParity)
{
	bzero(&termios_new, sizeof(termios_new));

	cfmakeraw(&termios_new);

	SMSetBaudrate(iBaudrate);
	termios_new.c_cflag |= CLOCAL | CREAD;
	SMSetDataBit(iDatabit);
	SMSetParityCheck(cParity);
	SMSetStopBit(pcStopbit);

	termios_new.c_oflag = 0;
	termios_new.c_lflag |= 0;
	termios_new.c_oflag &= ~OPOST;
	termios_new.c_cc[VTIME] = 1; /* unit: 1/10 second. */
	termios_new.c_cc[VMIN] = 1; /* minimal characters for reading */
	tcflush(iFd, TCIFLUSH);

	// 清空发送接收缓冲区
	tcflush(iFd, TCIOFLUSH);

	return tcsetattr(iFd, TCSANOW, &termios_new);
}

int SMOpenUsbPort(unsigned char ucComPort, int iBaudrate, int iDatabit,
		const char *pcStopbit, char cParity)
{
	char *pcComPort;
	int iFd;
	int iRet;

	switch (ucComPort)
	{
	case 0:
		pcComPort = (char *) "/dev/ttyUSB0";
		break;
	case 1:
		pcComPort = (char *) "/dev/ttyUSB1";
		break;
	case 2:
		pcComPort = (char *) "/dev/ttyUSB2";
		break;
	case 3:
		pcComPort = (char *) "/dev/ttyUSB3";
		break;
	default:
		pcComPort = (char *) "/dev/ttyUSB0";
		break;
	}

	iFd = open(pcComPort, O_RDWR);
	if (-1 == iFd)
		return -1;

	/* 0 on success, -1 on failure */
	iRet = SMSetPortAttr(iFd, iBaudrate, iDatabit, pcStopbit, cParity);
	if (-1 == iRet)
		return -1;

	return iFd;
}

int SMOpenComPort(unsigned char ucComPort, int iBaudrate, int iDatabit,
		const char *pcStopbit, char cParity)
{
	char *pcComPort;
	int iFd; //File descriptor for the port
	int iRet;

	switch (ucComPort)
	{
	case 0:
#ifdef _YK_MN2440_AA_
		pcComPort = (char *) "/dev/ttySAC1";
#elif _YK_XT8126_AV_
		pcComPort = (char *) "/dev/ttyS3";
#elif _YK_S5PV210_
		pcComPort = (char *) "/dev/s3c2410_serial2";
printf("pcComPort = %s \n", pcComPort);
#endif
		break;
	case 1:
#ifdef _YK_MN2440_AA_
		pcComPort = (char *) "/dev/ttySAC1";
#elif _YK_XT8126_AV_
		pcComPort = (char *) "/dev/ttyS2";
#elif _YK_IMX27_AV_

		pcComPort = (char *) "/dev/ttymxc1"; //南向通讯串口
		
#endif
		break;
	case 2:
	pcComPort = (char *) "/dev/ttymxc2";  //DTMF检测串口
	break;

	default:
		break;
	}

	iFd = open(pcComPort, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (-1 == iFd)
		return (-1);

	/* 0 on success, -1 on failure */
	iRet = SMSetPortAttr(iFd, iBaudrate, iDatabit, pcStopbit, cParity);
	if (-1 == iRet)
		return -1;

	return iFd;
}

void SMCloseComPort(int iFd)
{
	/* flush output pvData before close and restore old attribute */
	//tcsetattr (iFd, TCSADRAIN, &termios_old);
	close(iFd);
}

int SMReadComPort(int iFd, void *pvData, int iDataLen)
{
	fd_set fs_read;
	struct timeval tv_timeout;
	int iRet = 0;

	FD_ZERO(&fs_read);
	FD_SET(iFd, &fs_read);
	tv_timeout.tv_sec = 0; //TIMEOUT_SEC (iDataLen, SMGetBaudrate ());
	tv_timeout.tv_usec = 1; //TIMEOUT_USEC;

	iRet = select(iFd + 1, &fs_read, NULL, NULL, &tv_timeout);
	if (iRet)
		return read(iFd, pvData, iDataLen);
	else
		return 0;
}

int SMWriteComPort(int iFd, char * pvData, int iDataLen)
{
	fd_set fs_write;
	struct timeval tv_timeout;
	int iRet, iLen, iTotalLen;

	FD_ZERO(&fs_write);
	FD_SET(iFd, &fs_write);
	tv_timeout.tv_sec = 0; //TIMEOUT_SEC (iDataLen, SMGetBaudrate ());
	tv_timeout.tv_usec = 1; //TIMEOUT_USEC;

	for (iTotalLen = 0, iLen = 0; iTotalLen < iDataLen;)
	{
		iRet = select(iFd + 1, NULL, &fs_write, NULL, &tv_timeout);
		if (iRet)
		{
			iLen = write(iFd, &pvData[iTotalLen], iDataLen - iTotalLen);
			if (iLen > 0)
				iTotalLen += iLen;
		}
		else
		{
			tcflush(iFd, TCOFLUSH); /* flush all output pvData */
			break;
		}
	}

	printf("[SM_MESSAGE] -SMSend[%d] : %s\n", iDataLen, pvData);
	return iTotalLen;
}

