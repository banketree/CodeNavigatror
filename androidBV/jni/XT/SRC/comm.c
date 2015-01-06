#include     <stdio.h>      /*��׼�����������*/
#include   <time.h>
#include     <stdlib.h>     /*��׼�����ⶨ��*/
#include     <unistd.h>     /*Unix ��׼��������*/
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include     <fcntl.h>      /*�ļ����ƶ���*/
#include     <termios.h>    /*PPSIX �ն˿��ƶ���*/
#include     <errno.h>      /*����Ŷ���*/

#define CommonH
#include "common.h"
#include "TMInterface.h"

#define FALSE  -1
#define TRUE   0

 struct TImage sip_ok;
 struct TImage sip_error;

int OpenDev(char *Dev);  //�򿪴���
int set_speed(int fd, int speed);  //���ò�����
int set_Parity(int fd,int databits,int stopbits,int parity);  //���ò���

int OpenComm(int CommPort,int BautSpeed,int databits,int stopbits,int parity);  //�򿪴���

static int g_iOpendoorFlg = 0;

short CommRecvFlag=1;
pthread_t comm2_rcvid;
pthread_t comm3_rcvid;
pthread_t comm4_rcvid;
void CreateComm2_RcvThread(int fd);
void CreateComm3_RcvThread(int fd);
void CreateComm4_RcvThread(int fd);
void Comm2_RcvThread(int fd);  //Comm2�����̺߳���
void Comm3_RcvThread(int fd);  //Comm3�����̺߳���
void Comm4_RcvThread(int fd);  //Comm4�����̺߳���
int CommSendBuff(int fd,unsigned char buf[1024],int nlength);
void CloseComm();

int CheckTouchDelayTime(void);  //����������ʱ����ӳ�

//20110222 xu �����ſڻ���RFID_SIM
void Comm_Data_Deal(unsigned char *buff, int len);

void M8Input_Func(unsigned char *inputbuff); //M8����
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
 void IDCard_Func(unsigned char *inputbuff, int ID_Card_Type); //�û�ˢID��   1  M8  2  RFID-SIM
 void CheckBrushAndSend(void);  //���ˢ����¼�������������
 void SendIDCardAck(unsigned char *inputbuff, unsigned char result, int nLength);//����ID��ˢ��Ӧ��
#endif 
//�洢����
void Save_Setup(void);
//���ͱ�����Ϣ
void SendAlarm(void);
void SendOneOrder(unsigned char norder, char doorno, char *param);
//---------------------------------------------------------------------------
int OpenDev(char *Dev)                                                                               
{
  int	fd = open( Dev, O_RDWR|O_NONBLOCK );         //| O_NOCTTY | O_NDELAY
  if (FALSE == fd)
   {
    perror("Can't Open Serial Port");
    return FALSE;
   }
  else
    return fd;
}
//---------------------------------------------------------------------------
/**
*@brief  ���ô���ͨ������
*@param  fd     ���� int  �򿪴��ڵ��ļ����
*@param  speed  ���� int  �����ٶ�
*@return  void
*/
int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
					B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,  
					19200,  9600, 4800, 2400, 1200,  300, };
int set_speed(int fd, int speed)
{
  int   i;
  int   status;
  struct termios   Opt;
  tcgetattr(fd, &Opt);
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
   {
    if  (speed == name_arr[i])
     {
      tcflush(fd, TCIOFLUSH);
      cfsetispeed(&Opt, speed_arr[i]);
      cfsetospeed(&Opt, speed_arr[i]);
      status = tcsetattr(fd, TCSANOW, &Opt);
      if  (status != 0)
       {
        perror("tcsetattr fd1");
        return FALSE;
       }
      tcflush(fd,TCIOFLUSH);
     }
   }
  return TRUE;
}
//---------------------------------------------------------------------------
/**
*@brief   ���ô�������λ��ֹͣλ��Ч��λ
*@param  fd     ����  int  �򿪵Ĵ����ļ����
*@param  databits ����  int ����λ   ȡֵ Ϊ 7 ����8
*@param  stopbits ����  int ֹͣλ   ȡֵΪ 1 ����2
*@param  parity  ����  int  Ч������ ȡֵΪN,E,O,,S
*/
int set_Parity(int fd,int databits,int stopbits,int parity)
{
  struct termios options;
  if  ( tcgetattr( fd,&options)  !=  0)
   {
    perror("SetupSerial 1");
    return(FALSE);
   }
  options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
//  options.c_oflag  &= ~OPOST;   /*Output*/

  options.c_cflag &= ~CSIZE;
  switch (databits) /*��������λ��*/
   {
    case 7:
           options.c_cflag |= CS7;
           break;
    case 8:
           options.c_cflag |= CS8;
           break;
    default:
           fprintf(stderr,"Unsupported data size\n"); return (FALSE);
   }
  switch (parity)
   {
    case 'n':
    case 'N':
		options.c_cflag &= ~PARENB;   /* Clear parity enable */
		options.c_iflag &= ~INPCK;     /* Enable parity checking */ 
		break;  
    case 'o':
    case 'O':
		options.c_cflag |= (PARODD | PARENB); /* ����Ϊ��Ч��*/
		options.c_iflag |= INPCK;             /* Disnable parity checking */ 
		break;  
    case 'e':
    case 'E':
		options.c_cflag |= PARENB;     /* Enable parity */    
		options.c_cflag &= ~PARODD;   /* ת��ΪżЧ��*/
		options.c_iflag |= INPCK;       /* Disnable parity checking */
		break;
    case 'S':
    case 's':  /*as no parity*/
       	        options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB;break;  
    default:
		fprintf(stderr,"Unsupported parity\n");    
		return (FALSE);  
	}  
/* ����ֹͣλ*/
  switch (stopbits)
   {
	case 1:    
		options.c_cflag &= ~CSTOPB;  
		break;  
	case 2:    
		options.c_cflag |= CSTOPB;  
	   break;
	default:    
		 fprintf(stderr,"Unsupported stop bits\n");  
		 return (FALSE); 
   }
/* Set input parity option */ 
  if (parity != 'n')
    options.c_iflag |= INPCK;

  //һЩ��������
  options.c_cflag |= CLOCAL | CREAD;
  options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  options.c_oflag &= ~OPOST;
  options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);


  tcflush(fd,TCIFLUSH);
  options.c_cc[VTIME] = 10; /* ���ó�ʱ10 seconds*/
  options.c_cc[VMIN] = 1;//0; /* Update the options and do it NOW */
  if (tcsetattr(fd,TCSANOW,&options) != 0)
   {
     perror("SetupSerial 3");
     return (FALSE);
   }
  return (TRUE);
}
//---------------------------------------------------------------------------
int OpenComm(int CommPort,int BautSpeed,int databits,int stopbits,int parity)
{
  char dev[20]  = "/dev/ttyS"; //���ڶ�
  char   tmp[3];
  int Commfd;
  sprintf(tmp,   "%ld", CommPort-1 );
  strcat(dev,tmp);
  printf("Comm is %s\n",dev);
  Commfd = OpenDev(dev);
  if(Commfd == FALSE)
    return FALSE;
  if(set_speed(Commfd,BautSpeed) == FALSE)
   {
    P_Debug("Set Baut Error\n");
    return FALSE;
   }
  if (set_Parity(Commfd,databits,stopbits,parity) == FALSE)
   {
    P_Debug("Set Parity Error\n");
    return FALSE;
   }
  switch(CommPort)
   {
    case 2:
           CreateComm2_RcvThread(Commfd);
           break;
    case 3:
           CreateComm3_RcvThread(Commfd);
           break;
    case 4:
           CreateComm4_RcvThread(Commfd);
           break;
   }
  return Commfd;
}
//---------------------------------------------------------------------------
void CreateComm2_RcvThread(int fd)
{
  int i,ret;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  ret=pthread_create(&comm2_rcvid, &attr, (void *)Comm2_RcvThread, (void *)fd);
  pthread_attr_destroy(&attr);
  P_Debug ("Create comm2 pthread!\n");
  if(ret!=0){
    P_Debug ("Create comm2 pthread error!\n");
    exit (1);
  }
}
//---------------------------------------------------------------------------
void CreateComm3_RcvThread(int fd)
{
  int ret;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  ret=pthread_create(&comm3_rcvid, &attr, (void *)Comm3_RcvThread, (void *)fd);
  pthread_attr_destroy(&attr);
  P_Debug ("Create comm3 pthread!\n");
  if(ret!=0){
    P_Debug ("Create comm3 pthread error!\n");
    exit (1);
  }
}
//---------------------------------------------------------------------------
void CreateComm4_RcvThread(int fd)
{
  int ret;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  ret=pthread_create(&comm4_rcvid, &attr, (void *)Comm4_RcvThread, (void *)fd);
  pthread_attr_destroy(&attr);
  P_Debug ("Create comm4 pthread!\n");
  if(ret!=0){
    P_Debug ("Create comm4 pthread error!\n");
    exit (1);
  }
}
//---------------------------------------------------------------------------
uint32_t prev_comm_time;
void Comm2_RcvThread(int fd)  //Comm2�����̺߳���
{
  struct timeval tv;
  int len;
  int i;
  char buff[2048];

  P_Debug("This is comm2 pthread.\n");
  //��һ�δ������ݽ���ʱ��
  gettimeofday(&tv, NULL);
  prev_comm_time = tv.tv_sec *1000 + tv.tv_usec/1000;  
  while (CommRecvFlag == 1)     //ѭ����ȡ����
   {
    //ϵͳ��ʼ����־,��û�г�ʼ�������ȴ�
     //while(InitSuccFlag == 0)
     //  usleep(10000);
     while((len = read(fd, buff, 1024))>0)
      {
       sprintf(Local.DebugInfo, "Len %d Comm2\n", len);
       P_Debug(Local.DebugInfo);
       buff[len] = '\0';      
       Comm_Data_Deal(buff, len);
      }
   }
}
//---------------------------------------------------------------------------
//20110222 xu �����ſڻ���RFID_SIM
void Comm_Data_Deal(unsigned char *buff, int len)
{
  int i;
  struct timeval tv;
  uint32_t nowtime;
  unsigned char m_crc;
  struct commbuf1 commbuf;
  unsigned char validbuff[20];
  int validlen;
  int DataIsOK;
       gettimeofday(&tv, NULL);
       nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;
       //����һ�ν��ճ���50ms,���ж�Ϊ��ʱ
       if((nowtime - prev_comm_time) >= 100)
        {
         commbuf.iget = 0;
         commbuf.iput = 0;
         commbuf.n = 0;
        }
       prev_comm_time = nowtime;

       //printf("nowtime = %d\n", nowtime);

      /* for(i=0; i<len; i++)
         printf("0x%X, ", buff[i]);
       printf("\n");            */

       memcpy(commbuf.buffer + commbuf.iput, buff, len);
       commbuf.iput += len;
       if(commbuf.iput >= COMMMAX)
         commbuf.iput = 0;
       commbuf.n += len;
       //printf("commbuf.n = %d\n", commbuf.n);
       DataIsOK = 0;
       while(commbuf.n >= 11)
        {
         //RFID-SIM
         if((commbuf.buffer[commbuf.iget] == 0xB0)&&(commbuf.buffer[commbuf.iget+1] == 0x09))
          {
           DataIsOK = 1;
           validlen = 11;
           memcpy(validbuff, commbuf.buffer + commbuf.iget, validlen);
           commbuf.iget += validlen;
           if(commbuf.iget >= COMMMAX)
             commbuf.iget = 0;
           commbuf.n -= validlen;

           IDCard_Func(validbuff, 2);
          }
         //M8
         if(commbuf.buffer[commbuf.iget] == 0x7E)
          {
           DataIsOK = 1;
           validlen = 18;
           if(commbuf.n >= validlen)
            {
             //printf("dddddd\n");
             memcpy(validbuff, commbuf.buffer + commbuf.iget, validlen);
             commbuf.iget += validlen;
             if(commbuf.iget >= COMMMAX)
               commbuf.iget = 0;
             commbuf.n -= validlen;
             if((validbuff[0] == 0x7E)&&(validbuff[17] == 0x0D))
              {
               m_crc = 0;
               for(i=1; i<=15; i++)
                 m_crc += validbuff[i];
               if((m_crc == 0x7E)||(m_crc == 0x0D))
                 m_crc -= 1;
               //printf("m_crc = 0x%X, validbuff[16] = 0x%X\n", m_crc, validbuff[16]);
               if(m_crc == validbuff[16])
                {
                 M8Input_Func(validbuff); //M8����
                }
              }
            }
           else
             break;
          }
         if(DataIsOK == 0)
          {
           commbuf.iget ++;
           if(commbuf.iget >= COMMMAX)
             commbuf.iget = 0;
           commbuf.n --;
          }
        }
}
//---------------------------------------------------------------------------
//�洢����
void Save_Setup(void)
{
  int i;
  //д���ļ�
  //����������
  pthread_mutex_lock (&Local.save_lock);
  //���ҿ��ô洢���岢���
  for(i=0; i<SAVEMAX; i++)
   if(Save_File_Buff[i].isValid == 0)
    {
     Save_File_Buff[i].Type = 4;      //�洢��������
     Save_File_Buff[i].isValid = 1;
     sem_post(&save_file_sem);
     break;
    }
  printf("save local setup i = %d\n", i);
  //�򿪻�����
  pthread_mutex_unlock (&Local.save_lock);
}
//---------------------------------------------------------------------------
void Comm3_RcvThread(int fd)  //Comm3�����̺߳���
{
#if 1
  struct timeval tv;
  int len;
  long int i = 0;
  char buff[2048];

  P_Debug("This is comm3 pthread.\n");
  //��һ�δ������ݽ���ʱ��
  gettimeofday(&tv, NULL);
  prev_comm_time = tv.tv_sec *1000 + tv.tv_usec/1000;
  while (CommRecvFlag == 1)     //ѭ����ȡ����
   {
     while((len = read(fd, buff, 1024))>0)
      {
       //sprintf(Local.DebugInfo, "Len %d Comm3\n", len);
       //P_Debug(Local.DebugInfo);
       buff[len] = '\0';      
       Comm_Data_Deal(buff, len);
      }
     //added by lchx
     //��ֹ�߳�����������ݿڻ���ס����
     if(i > 999999) //
     {
//    	 printf("XT: feed dog!\n");
    	 SendOneOrder(_M8_WATCHDOG_START, 0, NULL);
    	 i = 0;
     }
     i++;
   }
#endif
}
//---------------------------------------------------------------------------
void Comm4_RcvThread(int fd)  //Comm4�����̺߳���
{
  struct timeval tv;
  int len;
  int i;
  char buff[2048];

  P_Debug("This is comm4 pthread.\n");
  //��һ�δ������ݽ���ʱ��
  gettimeofday(&tv, NULL);
  prev_comm_time = tv.tv_sec *1000 + tv.tv_usec/1000;
  while (CommRecvFlag == 1)     //ѭ����ȡ����
   {
     while((len = read(fd, buff, 1024))>0)
      {
       sprintf(Local.DebugInfo, "Len %d Comm4\n", len);
       P_Debug(Local.DebugInfo);
       buff[len] = '\0';      
       Comm_Data_Deal(buff, len);
      }
   }
}
//---------------------------------------------------------------------------
void CloseComm()
{
  //Comm2���ݽ����߳�
  CommRecvFlag = 0;
  usleep(40*1000);
  //pthread_cancel(comm2_rcvid);
  pthread_cancel(comm3_rcvid);
  //pthread_cancel(comm4_rcvid);
  //close(Comm2fd);
  close(Comm3fd);
  //close(Comm4fd);
}
//---------------------------------------------------------------------------
int CommSendBuff(int fd,unsigned char buf[1024],int nlength)
{
  int i;
  int nByte = write(fd, buf, nlength);
  //printf("fd = %d\n", fd);
  //for(i=0; i<nlength; i++)
  //  printf("0x%X, ", buf[i]);
  //printf("\n");
  return nByte;
}
//---------------------------------------------------------------------------
void M8Input_Func(unsigned char *inputbuff) //M8����
{
  int i;

  unsigned char tmpvalue;

  switch(inputbuff[1])
   {
    case 0x52:  //ID����
             //SendAck(inputbuff, 18);
             #ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
              IDCard_Func(inputbuff, 1);
             #endif 
             break;
    case 0x90: //M8���Ͱ���ֵ
             tmpvalue = inputbuff[4];
             printf("tmpvalue = 0x%X\n", tmpvalue);
             switch(tmpvalue)
              {
                   case 0:  //0
                   case 1:  //1
                   case 2:  //2
                   case 3:  //3
                   case 4:  //4
                   case 5:  //5
                   case 6:  //6
                   case 7:  //7
                   case 8:  //8
                   case 9:  //9
                   case 10:  //*
                   case 11:  //#
                          KeyAndTouchFunc('a' + tmpvalue);

                         /**
                          * add by chensq 20120331 �������߻��ѹ���
                          */
                  		if (Local.LcdLightFlag == 0) {
                  			OpenLCD();
                  			printf("*****press key: open lcd*****\n");
                  		}
                  		Local.LcdLightFlag = 1;
                  		Local.LcdLightTime = 0;

                          break;
              }
             break;
   }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef _BRUSHIDCARD_SUPPORT   //ˢ������֧��
void IDCard_Func(unsigned char *inputbuff, int ID_Card_Type) //�û�ˢID��   1  M8  2  RFID-SIM
{
  int i;
  int IDCardOk;
  int tmpnum;
  char mac_text[5];
  int hex_data[6];
  //ʱ��
  time_t t;
  struct tm *tm_t;
  char wavFile[80];
  int tmp_pos;
  if(ID_Card_Type == 1) //M8
   {
    tmp_pos = 4;
    printf("Card data send from M8\n");
    sprintf(Local.DebugInfo, "IDcard[0] = %c%c, IDcard[1] = %c%c, IDcard[2] = %c%c, IDcard[3] = %c%c\n",
           inputbuff[tmp_pos], inputbuff[tmp_pos+1], inputbuff[tmp_pos+2], inputbuff[tmp_pos+3], inputbuff[tmp_pos+4], inputbuff[tmp_pos+5], inputbuff[tmp_pos+6], inputbuff[tmp_pos+7]);
    P_Debug(Local.DebugInfo);

    for(i=0; i<4; i++)
      hex_data[i] = 0;
    for(i=0; i<8; i+=2)
     {
      mac_text[0] = inputbuff[tmp_pos+i];
      mac_text[1] = inputbuff[tmp_pos + 1 + i];
      mac_text[2] = '\0';
      sscanf(mac_text, "%x", &hex_data[i/2]);
     }
   }
  else
   {
    tmp_pos = 3;
    printf("Card data send from RFID-SIM\n");
    for(i=0; i<4; i++)
      hex_data[i] = inputbuff[7+i];
   }


  //printf("hex_data[0] = 0x%X, hex_data[1] = 0x%X, hex_data[2] = 0x%X, hex_data[3] = 0x%X\n",
  //       hex_data[0], hex_data[1], hex_data[2], hex_data[3]);
  IDCardOk = 0;
  for(i=0; i<IDCardNo.Num; i++)
    if((IDCardNo.SN[i*BYTEPERSN+0] == hex_data[0])&&
       (IDCardNo.SN[i*BYTEPERSN+1] == hex_data[1])&&
       (IDCardNo.SN[i*BYTEPERSN+2] == hex_data[2])&&
       (IDCardNo.SN[i*BYTEPERSN+3] == hex_data[3]))
      {
       IDCardOk = 1;
       break;
      }
#if 0
  printf("-------------------------------\n");
  for(i=0; i<IDCardNo.Num; i++)
    printf("No.%d, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", i, IDCardNo.SN[i*BYTEPERSN+0], IDCardNo.SN[i*BYTEPERSN+1], IDCardNo.SN[i*BYTEPERSN+2], IDCardNo.SN[i*BYTEPERSN+3]);
  printf("-------------------------------\n");
#endif

  sprintf(Local.DebugInfo, "IDCardOk = %d, IDCardNo.Num = %d, 0x%02X-0x%02X-0x%02X-0x%02X\n", IDCardOk, IDCardNo.Num, hex_data[0], hex_data[1], hex_data[2], hex_data[3]);
  P_Debug(Local.DebugInfo);
  SendIDCardAck(inputbuff, IDCardOk, 14);//����ID��ˢ��Ӧ��
  if(g_iOpendoorFlg == 1)
   {
     OpenLock_Func();  //��������
     //ˢ����¼
     if(BrushIDCard.Num < IDCARDBRUSHNUM)
      {
       time(&t);
       tm_t=localtime(&t);
       //ID����
       tmpnum = BrushIDCard.Num*11;
       //����������
       pthread_mutex_lock (&Local.idcard_lock);
       //memcpy(BrushIDCard.Info + tmpnum, hex_data, 4);
       BrushIDCard.Info[tmpnum] = hex_data[0];
       BrushIDCard.Info[tmpnum + 1] = hex_data[1];
       BrushIDCard.Info[tmpnum + 2] = hex_data[2];
       BrushIDCard.Info[tmpnum + 3] = hex_data[3];
       BrushIDCard.Info[tmpnum + 4] = (tm_t->tm_year + 1900)/256;
       BrushIDCard.Info[tmpnum + 5] = (tm_t->tm_year + 1900)%256;
       BrushIDCard.Info[tmpnum + 6] = tm_t->tm_mon+1;
       BrushIDCard.Info[tmpnum + 7] = tm_t->tm_mday;
       BrushIDCard.Info[tmpnum + 8] =  tm_t->tm_hour;
       BrushIDCard.Info[tmpnum + 9] =  tm_t->tm_min;
       BrushIDCard.Info[tmpnum + 10] =  tm_t->tm_sec;
       //printf("%d %d %d %d %d %d %d\n", BrushIDCard.Info[4], BrushIDCard.Info[5], BrushIDCard.Info[6], BrushIDCard.Info[7], BrushIDCard.Info[8],
       //       BrushIDCard.Info[9], BrushIDCard.Info[10]);
       BrushIDCard.Num ++;
       //�򿪻�����
       pthread_mutex_unlock (&Local.idcard_lock);
       //���ˢ����¼�������������
       CheckBrushAndSend();
      }
   }
}
//---------------------------------------------------------------------------
//���ˢ����¼�������������
void CheckBrushAndSend(void)
{
  int i;
  int tmpnum;
  if(BrushIDCard.Num > 0)
   {
            //����������
            pthread_mutex_lock (&Local.udp_lock);
            //���ҿ��÷��ͻ��岢���
            for(i=0; i<UDPSENDMAX; i++)
             if(Multi_Udp_Buff[i].isValid == 0)
              {
               Multi_Udp_Buff[i].SendNum = 0;
               Multi_Udp_Buff[i].m_Socket = m_DataSocket;
               Multi_Udp_Buff[i].m_Socket = RemoteDataPort;
             //  Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
               sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",LocalCfg.IP_Server[0],
                       LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
               //ͷ��
               memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
               //����
               Multi_Udp_Buff[i].buf[6] = BRUSHIDCARD;
               Multi_Udp_Buff[i].buf[7] = ASK;    //����

               memcpy(Multi_Udp_Buff[i].buf+8,LocalCfg.Addr,20);

               //����
               if(BrushIDCard.Num > 120)
                 tmpnum = 120;
               else
                 tmpnum = BrushIDCard.Num;
               Multi_Udp_Buff[i].buf[28] = (tmpnum & 0x00FF);
               Multi_Udp_Buff[i].buf[29] = (tmpnum & 0xFF00) >> 8;

               memcpy( Multi_Udp_Buff[i].buf + 30, BrushIDCard.Info, 11*tmpnum);

               Multi_Udp_Buff[i].nlength = 30 + 11*tmpnum;
               Multi_Udp_Buff[i].DelayTime = 100;
               Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
               Multi_Udp_Buff[i].isValid = 1;
               sem_post(&multi_send_sem);
               break;
              }
            //�򿪻�����
            pthread_mutex_unlock (&Local.udp_lock);
   }
}
//---------------------------------------------------------------------------
void SendIDCardAck(unsigned char *inputbuff, unsigned char result, int nLength)//����ID��ˢ��Ӧ��
{
 char wavFile[80];
 int iRfCardExist = FALSE;
 char acCardNum[32] = { 0 };

//if(0 == strcmp(inputbuff+4, "00297571") || 0 == strcmp(inputbuff+4, "C87A1DAE") ||
//	 0 == strcmp(inputbuff+4, "000162E1") || 0 == strcmp(inputbuff+4, "981692DE")
//	 ||  0 == strcmp(inputbuff+4, "00981692"))

   strcpy(acCardNum, inputbuff+4);
   if(TMSMIsICOpenDoorEnabled() == 1)
   {
	   iRfCardExist = TMSMIsValidICIDCard(acCardNum);
	   if(iRfCardExist == 1)
	   {
		 printf("ˢ���ɹ�\n");
		 sprintf(wavFile, "%sdooropen.wav\0", wavPath);
		 StartPlayWav(wavFile, 0);
		 g_iOpendoorFlg = 1;
		 //==============ADD BY HB==================
		 TMSetICIDOpenDoor(acCardNum);
		 //==============ADD BY HB==================
	   }
	  else
	   {
		 printf("ˢ��ʧ��\n");
		 sprintf(wavFile, "%scard_fail.wav\0", wavPath);
		 StartPlayWav(wavFile, 0);
		 g_iOpendoorFlg = 0;
	   }
   }
   else
   {
	   printf("ˢ�����ܱ�����\n");
   }
 #if 0
  unsigned char send_b[30];
  int i;
  memcpy(send_b, inputbuff, nLength);
  send_b[2] = 0x32; //Ӧ��
  send_b[15] = result;   //1 �ɹ�  0  ʧ��
  send_b[16] = 0;
  for(i=1; i<=15; i++)    //crc
    send_b[16] += send_b[i];
  if((send_b[16] == 0x7E)||(send_b[16] == 0x0D))
    send_b[16] --;
  send_b[17] = 0x0D;
  CommSendBuff(Comm2fd, send_b, nLength);
 #endif
}
#endif
//----------------------------------------------------------------------------
//���ͱ�����Ϣ
void SendAlarm(void)
{
  int i;
            //����������
            pthread_mutex_lock (&Local.udp_lock);
            //���ҿ��÷��ͻ��岢���
            for(i=0; i<UDPSENDMAX; i++)
             if(Multi_Udp_Buff[i].isValid == 0)
              {
               Multi_Udp_Buff[i].SendNum = 0;
               Multi_Udp_Buff[i].m_Socket = m_DataSocket;
               Multi_Udp_Buff[i].RemotePort = RemoteDataPort;
             //  Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
               sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",LocalCfg.IP_Server[0],
                       LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
               //ͷ��
               memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
               //����
               Multi_Udp_Buff[i].buf[6] = ALARM;
               Multi_Udp_Buff[i].buf[7] = ASK;    //����

               memcpy(Multi_Udp_Buff[i].buf+8,LocalCfg.Addr,20);

               Multi_Udp_Buff[i].buf[28] = 0x01;

               Multi_Udp_Buff[i].nlength = 29;
               Multi_Udp_Buff[i].DelayTime = 100;
               Multi_Udp_Buff[i].SendDelayTime = 0; //���͵ȴ�ʱ�����  20101112
               Multi_Udp_Buff[i].isValid = 1;
               sem_post(&multi_send_sem);
               break;
              }
            //�򿪻�����
            pthread_mutex_unlock (&Local.udp_lock);
}
//---------------------------------------------------------------------------
int CheckTouchDelayTime(void)  //����������ʱ����ӳ�
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  Local.newtouchtime = tv.tv_sec *1000 + tv.tv_usec/1000;
  if(((Local.newtouchtime - Local.oldtouchtime) < TOUCHMAXTIME)&&(Local.newtouchtime > Local.oldtouchtime))
    return 0;
  else
    Local.oldtouchtime = Local.newtouchtime;
  return 1;
}
//---------------------------------------------------------------------------
void SendOneOrder(unsigned char norder, char doorno, char *param)
{
  int i,j;
  int sendlength;
  unsigned char comm_buf[50];

  //20100420  ���ſ�, ��ǰΪģ�ⷽʽ���ָ�Ϊ���ַ�ʽ, ����ֻ��һ��
  doorno = 0x30;

  comm_buf[0] = 0x7E;   //��ʼ�ַ�
  comm_buf[1] = norder;   //����
  comm_buf[2] = '1';   // ��������
  comm_buf[3] = '2';
  comm_buf[4] = '3';
  comm_buf[5] = '4';

  if(param == NULL)
   {
    for(j=6; j<=15; j++)
      comm_buf[j] = 0;
   }
  else
    memcpy(comm_buf + 4, param, 12);
  comm_buf[16] = 0;
  for(j=1; j<=15; j++)    //crc
    comm_buf[16] += comm_buf[j];
  if((comm_buf[16] == 0x7E)||(comm_buf[16] == 0x0D))
    comm_buf[16] --;
  comm_buf[17] = 0x0D;

  sendlength = 18;
  CommSendBuff(Comm3fd, comm_buf, sendlength);
}
//---------------------------------------------------------------------------
