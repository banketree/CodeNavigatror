#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>

#define CommonH
#include "common.h"

struct TImage talkover_image; //���н���

//���̺ʹ�����������
void KeyAndTouchFunc(int pressed);

extern int ShowCursor; //�Ƿ���ʾ���
//������ʾ����
void DisplayMainWindow(short wintype); //����

//��ǰ����
//          0 -- ��������
//          1 -- ���н���
//          2 -- ����������(ס��������)
//          3 -- ����ͨ����
//          4 -- ���ڼ�����
//          5 -- ���뿪������
//          6 -- �������ý���
//          7--���н���

//          53 -- ��Ƭ���Ŵ���
//          60 -- ����������

//          61 -- ������Ϣ����
//          62 -- �豸��ַ�������
//          63 -- ����Ա����������
//          64 -- ��������������

//          71 -- MAC��ַ�������
//          72 -- IP��ַ�������
//          73 -- ��������������
//          74 -- ���ص�ַ�������
//          75 -- ��������ַ�������

void InterfaceInit(void); //�����ʼ��
void MainPageInit(void); //��ҳ��ʼ��
void EditInit(void); //�ı����ʼ��
void EditUnInit(void); //�ı����ͷ�
void ImageInit(void); //Image��ʼ��
void ImageUnInit(void); //Image�ͷ�
void LabelInit(void); //Label��ʼ��
void LabelUnInit(void); //Label�ͷ�

//void ShowLabel(struct TLabel *t_label, int refreshflag);
//��ʾClock
void ShowClock(struct TLabel *t_label, int refreshflag);

void PlaySoundTip(void); //������ʾ��

//��ʾͼ������
void outxy_pic_num(int xLeft, int yTop, char *str, int PageNo);
//---------------------------------------------------------------------------
//��ʾͼ������
void outxy_pic_num(int xLeft, int yTop, char *str, int PageNo)
{
	int i, j;
	int len;
	char cText[11] = "0123456789*";
	len = strlen(str);
	for (i = 0; i < len; i++)
	{
		for (j = 0; j < 11; j++)
			if (cText[j] == str[i])
			{
				num_yk[j].xLeft = xLeft + num_yk[j].width * i;
				num_yk[j].yTop = yTop;
				DisplayImage(&num_yk[j], PageNo);
				//str[len] = '\0';
				//printf("j = %d, str = %s\n", j, str);
				break;
			}
	}
}

//��ʾͼ������(С)
void outxy_pic_num1(int xLeft, int yTop, char *str, int PageNo)
{
	int i, j;
	int len;
	char cText[14] = "0123456789.:*";
	len = strlen(str);
	for (i = 0; i < len; i++)
	{
		for (j = 0; j < 13; j++)
			if (cText[j] == str[i])
			{
				num_image[j].xLeft = xLeft + num_image[j].width * i;
				num_image[j].yTop = yTop;
				DisplayImage(&num_image[j], PageNo);
				//str[len] = '\0';
				//printf("j = %d, str = %s\n", j, str);
				break;
			}
	}
}
//---------------------------------------------------------------------------
void DisplayMainWindow(short wintype)
{
	int i, j;
	//�ȹرչ��
	if (ShowCursor == 1)
		ShowCursor = 0;

	switch (wintype)
	{
	case 0: //������
		DisplayWelcomeWindow(); //��ʾ�Խ�����
		break;
	case 1: //�Խ����ڣ�һ����
		if (Local.CurrentWindow != wintype)
			DisplayTalkWindow();
		break;
	case 5: //���ô��ڣ�һ����
		if (Local.CurrentWindow != wintype)
			DisplaySetupWindow();
		break;
	case 12: //����ͨ�����ڣ�������
		break;
	case 13: //���Ӵ��ڣ�������
		if (Local.CurrentWindow != wintype)
			DisplayWatchWindow();
		break;
	case 16: //��ʾͼ��������ڣ�������,�к���ʱ�Զ���ʾ
		break;
	default:
		break;
	}

}
//---------------------------------------------------------------------------
/*void key_press_thread_func(void)
 {
 int i,j;

 #ifdef _DEBUG
 printf("�������������̣߳�\n" );
 printf("key_press_flag=%d\n",key_press_flag);
 #endif
 while(key_press_flag == 1)
 {
 //�ȴ��а������µ��ź�
 sem_wait(&key_press_sem);
 //ϵͳ��ʼ����־,��û�г�ʼ�������ȴ�
 while(InitSuccFlag == 0)
 usleep(10000);
 //    printf("Local.CurrentWindow = %d", Local.CurrentWindow);
 if(Local.Key_Press_Run_Flag == 0)
 Local.Key_Press_Run_Flag = 1;
 else
 {
 //���̺ʹ�����������
 KeyAndTouchFunc(key_press);
 }
 }
 }                                     */
//---------------------------------------------------------------------------
void PlaySoundTip(void) //������ʾ��
{
	printf("Local.Status:%d\n", Local.Status);
	if (Local.Status == 0)
	{
		WaitAudioUnuse(200); //�ȴ���������
		sprintf(wavFile, "%s/sound1.wav\0", wavPath);
		StartPlayWav(wavFile, 0);
	}
	else
		printf("Local.Status = %d, cannot playsoundtip\n", Local.Status);
}
//---------------------------------------------------------------------------
//���̺ʹ�����������
void KeyAndTouchFunc(int pressed)
{
	printf("pressed = %d\n", pressed);

	if (CheckTouchDelayTime() == 0) //����������ʱ����ӳ�
	{
		printf("touch is too quick XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n");
		return;
	}
	PlaySoundTip(); //������ʾ��

	PicStatBuf.KeyPressTime = 0;
	Local.LcdLightTime = 0;

	if(Local.LcdLightFlag == 0)
	{
		DisplayWelcomeWindow();
		return ;
	}

	if(Local.CurrentWindow == 0 && pressed != 107)
	{
		DisplayTalkWindow();
	}


//��Զ������
#if 0
	if(Local.LcdLightFlag == 0)
	{
		//����LCD����
		OpenLCD();
		Local.LcdLightFlag = 1;//LCD�����־
		Local.LcdLightTime = 0;//ʱ��
		return;
	}
#endif   

	switch (pressed)
	{
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':
	case 'l':
	case 'm':
	case 'n':
	case 'o':
	case 'p': //14
	case 'q':
	case 'r':
	case 's':
	case 't':
	case 'u':
	case 'v':
	case 'w':
	case 'x':
	case 'y':
	case 'z': //���������µİ�ť
	case 'a' + 30:
	case 'a' + 31:
	case 'a' + 32:
	case 'a' + 33:
	case 'a' + 34:
	case 'a' + 35:
		printf("*****KeyAndTouchFunc()***Local.CurrentWindow=%d***press key is %c***\n",
				Local.CurrentWindow, pressed);
		if ((Local.CurrentWindow >= 0) && (Local.CurrentWindow <= 300))
			switch (Local.CurrentWindow)
			{
			case 1: //�Խ����������һ����
				OperateTalkWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 2: //�Խ� �������Ӵ��ڲ���
				OperateTalkConnectWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 3: //�Խ� ͨ�����ڲ���
				OperateTalkStartWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 4: //���ӽ������(������
				OperateWatchWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 5: //���뿪�����������һ����
				OperateOpenLockWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 6: //�������ô��ڲ�����һ����
				OperateSetupWindow(pressed - 'a', Local.CurrentWindow);
				break;
				//*******************add by xuqd*******************************
			case 7: // ���н����Ĵ��ڲ���
				OperateTalkOverWindow(pressed - 'a', Local.CurrentWindow);
				break;

#ifdef _DOWNLOAD_PHOTO  //����ͼƬ������֧��
				case 53: //��Ƭ���Ŵ��ڲ�����һ����
				OperatePlayPhotoWindow(pressed - 'a', Local.CurrentWindow);
				break;
#endif
			case 60: ////���������������������
				OperateSetupMainWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 61: //��ѯ������Ϣ����
				OperateSetupInfoWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 62: //�豸��ַ�������
				OperateAddressWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 63: //����Ա����������
			case 64: //��������������
				OperateModiPassWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 71: //MAC��ַ�������
				OperateMacWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 72: //IP��ַ���ò�����������
				OperateIPWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 73: //��������������
				OperateIPMaskWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 74: //���ص�ַ�������
				OperateIPGateWindow(pressed - 'a', Local.CurrentWindow);
				break;
			case 75: //��������ַ�������
				OperateIPServerWindow(pressed - 'a', Local.CurrentWindow);
				break;
			}
		break;
	}
}
//---------------------------------------------------------------------------
void InterfaceInit(void) //�����ʼ��
{
	MainPageInit(); //��ҳ��ʼ��
	//��ʼ��Image
	ImageInit();

	//��ʼ��Edit
	EditInit();

	printf("InterfaceInit 11\n");
}
//---------------------------------------------------------------------------
void MainPageInit(void) //��ҳ��ʼ��
{
	strcpy(currpath, sPath);
}
//---------------------------------------------------------------------------
#ifdef YK_XT8126_BV_FKT
void EditInit(void) //�ı����ʼ��
{
	int i;
	char jpgfilename[80];

	/*����ͨ������***************************************/
	//�ı���1
	FreeEdit(&r2r_edit[0]);
	//���
	if (r2r_edit[0].Cursor_H != NULL)
	{
		free(r2r_edit[0].Cursor_H);
		r2r_edit[0].Cursor_H = NULL;
	}

	sprintf(jpgfilename, "%s/talk_edit.jpg", currpath);
	EditLoadFromFile(jpgfilename, &r2r_edit[0]);

	r2r_edit[0].CursorX = (r2r_edit[0].width - 11 * num_yk[0].width) / 2;
	r2r_edit[0].CursorY = (r2r_edit[0].height - num_yk[0].height) / 2;
	r2r_edit[0].CursorHeight = 24;
	r2r_edit[0].CursorCorlor = cBlack;
	r2r_edit[0].fWidth = num_yk[0].width;

	r2r_edit[0].xLeft = 46;
	r2r_edit[0].yTop = 118;

	r2r_edit[0].Text[0] = '\0';
	r2r_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	r2r_edit[0].MaxLen = 11; //�ı���������볤��
	r2r_edit[0].Focus = 0; //��ʾ������
	r2r_edit[0].Visible = 0;


	//���뿪������
	openlock_edit = r2r_edit[0];

	openlock_edit.xLeft = 46;
	openlock_edit.yTop = 118;
	openlock_edit.BoxLen = 0; //�ı���ǰ���볤��
	openlock_edit.MaxLen = 11; //�ı���������볤��
	openlock_edit.Focus = 0; //��ʾ������
	openlock_edit.Visible = 0;


	//���ô���
	setup_pass_edit = r2r_edit[0];

	setup_pass_edit.xLeft = 46;
	setup_pass_edit.yTop = 118;
	setup_pass_edit.BoxLen = 0; //�ı���ǰ���볤��
	setup_pass_edit.MaxLen = 11; //�ı���������볤��
	setup_pass_edit.Focus = 0; //��ʾ������
	setup_pass_edit.Visible = 0;

	//�������ô����ı���
	if (addr_edit[0].image != NULL)
		FreeImage(&addr_edit[0]);
	sprintf(jpgfilename, "%s/talk_edit1.jpg", currpath);
	EditLoadFromFile(jpgfilename, &addr_edit[0]);

	addr_edit[0].xLeft = 132;
	addr_edit[0].yTop = 166;
	addr_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	addr_edit[0].MaxLen = 7; //�ı���������볤��
	addr_edit[0].Focus = 0; //��ʾ������
	addr_edit[0].Visible = 0;

	//MAC��ַ���ô����ı���
	mac_edit[0] = addr_edit[0];

	mac_edit[0].xLeft = 132;
	mac_edit[0].yTop = 166;
	mac_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	mac_edit[0].MaxLen = 8; //�ı���������볤��
	mac_edit[0].Focus = 0; //��ʾ������
	mac_edit[0].Visible = 0;

	//IP��ַ���ô����ı���
	ip_edit[0] = addr_edit[0];

	ip_edit[0].CursorX = (ip_edit[0].width - 15 * num_image[0].width) / 2;
	ip_edit[0].CursorY = (ip_edit[0].height - num_image[0].height) / 2;
	ip_edit[0].fWidth = num_image[0].width;

	ip_edit[0].xLeft = 132;
	ip_edit[0].yTop = 166;
	ip_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ip_edit[0].MaxLen = 15; //�ı���������볤��
	ip_edit[0].Focus = 0; //��ʾ������
	ip_edit[0].Visible = 0;

	/*�����������ô����ı���***************************************/
	ipmask_edit[0] = addr_edit[0];

	ipmask_edit[0].xLeft = 132;
	ipmask_edit[0].yTop = 166;
	ipmask_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ipmask_edit[0].MaxLen = 15; //�ı���������볤��
	ipmask_edit[0].Focus = 0; //��ʾ������
	ipmask_edit[0].Visible = 0;

	/*���ص�ַ���ô����ı���***************************************/
	ipgate_edit[0] = addr_edit[0];

	ipgate_edit[0].xLeft = 132;
	ipgate_edit[0].yTop = 166;
	ipgate_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ipgate_edit[0].MaxLen = 15; //�ı���������볤��
	ipgate_edit[0].Focus = 0; //��ʾ������
	ipgate_edit[0].Visible = 0;

	/*��������ַ���ô����ı���***************************************/
	ipserver_edit[0] = addr_edit[0];

	ipserver_edit[0].xLeft = 132;
	ipserver_edit[0].yTop = 166;
	ipserver_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ipserver_edit[0].MaxLen = 15; //�ı���������볤��
	ipserver_edit[0].Focus = 0; //��ʾ������
	ipserver_edit[0].Visible = 0;

	/*�޸Ĺ������봰���ı���***************************************/
	modi_engi_edit[0] = addr_edit[0];

	modi_engi_edit[0].xLeft = 132;
	modi_engi_edit[0].yTop = 166;
	modi_engi_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	modi_engi_edit[0].MaxLen = 6; //�ı���������볤��
	modi_engi_edit[0].Focus = 0; //��ʾ������
	modi_engi_edit[0].Visible = 0;

	modi_engi_edit[1] = addr_edit[0];

	modi_engi_edit[1].xLeft = 132;
	modi_engi_edit[1].yTop = 166;
	modi_engi_edit[1].BoxLen = 0; //�ı���ǰ���볤��
	modi_engi_edit[1].MaxLen = 6; //�ı���������볤��
	modi_engi_edit[1].Focus = 0; //��ʾ������
	modi_engi_edit[1].Visible = 0;
}
#else
void EditInit(void) //�ı����ʼ��
{
	int i;
	char jpgfilename[80];

	/*����ͨ������***************************************/
	//�ı���1
	FreeEdit(&r2r_edit[0]);
	//���
	if (r2r_edit[0].Cursor_H != NULL)
	{
		free(r2r_edit[0].Cursor_H);
		r2r_edit[0].Cursor_H = NULL;
	}

	sprintf(jpgfilename, "%s/talk_edit.jpg", currpath);
	EditLoadFromFile(jpgfilename, &r2r_edit[0]);

	r2r_edit[0].CursorX = (r2r_edit[0].width - 6 * num_yk[0].width) / 2;
	r2r_edit[0].CursorY = (r2r_edit[0].height - num_yk[0].height) / 2;
	r2r_edit[0].CursorHeight = 24;
	r2r_edit[0].CursorCorlor = cBlack;
	r2r_edit[0].fWidth = num_yk[0].width;

	r2r_edit[0].xLeft = 120;
	r2r_edit[0].yTop = 108;

	r2r_edit[0].Text[0] = '\0';
	r2r_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	r2r_edit[0].MaxLen = 6; //�ı���������볤��
	r2r_edit[0].Focus = 0; //��ʾ������
	r2r_edit[0].Visible = 0;


	//���뿪������
	openlock_edit = r2r_edit[0];

	openlock_edit.xLeft = 120;
	openlock_edit.yTop = 108;
	openlock_edit.BoxLen = 0; //�ı���ǰ���볤��
	openlock_edit.MaxLen = 6; //�ı���������볤��
	openlock_edit.Focus = 0; //��ʾ������
	openlock_edit.Visible = 0;


	//���ô���
	setup_pass_edit = r2r_edit[0];

	setup_pass_edit.xLeft = 120;
	setup_pass_edit.yTop = 108;
	setup_pass_edit.BoxLen = 0; //�ı���ǰ���볤��
	setup_pass_edit.MaxLen = 6; //�ı���������볤��
	setup_pass_edit.Focus = 0; //��ʾ������
	setup_pass_edit.Visible = 0;

	//�������ô����ı���
	if (addr_edit[0].image != NULL)
		FreeImage(&addr_edit[0]);
	sprintf(jpgfilename, "%s/talk_edit1.jpg", currpath);
	EditLoadFromFile(jpgfilename, &addr_edit[0]);

	addr_edit[0].xLeft = 132;
	addr_edit[0].yTop = 166;
	addr_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	addr_edit[0].MaxLen = 7; //�ı���������볤��
	addr_edit[0].Focus = 0; //��ʾ������
	addr_edit[0].Visible = 0;

	//MAC��ַ���ô����ı���
	mac_edit[0] = addr_edit[0];

	mac_edit[0].xLeft = 132;
	mac_edit[0].yTop = 166;
	mac_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	mac_edit[0].MaxLen = 8; //�ı���������볤��
	mac_edit[0].Focus = 0; //��ʾ������
	mac_edit[0].Visible = 0;

	//IP��ַ���ô����ı���
	ip_edit[0] = addr_edit[0];

	ip_edit[0].CursorX = (ip_edit[0].width - 15 * num_image[0].width) / 2;
	ip_edit[0].CursorY = (ip_edit[0].height - num_image[0].height) / 2;
	ip_edit[0].fWidth = num_image[0].width;

	ip_edit[0].xLeft = 132;
	ip_edit[0].yTop = 166;
	ip_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ip_edit[0].MaxLen = 15; //�ı���������볤��
	ip_edit[0].Focus = 0; //��ʾ������
	ip_edit[0].Visible = 0;

	/*�����������ô����ı���***************************************/
	ipmask_edit[0] = addr_edit[0];

	ipmask_edit[0].xLeft = 132;
	ipmask_edit[0].yTop = 166;
	ipmask_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ipmask_edit[0].MaxLen = 15; //�ı���������볤��
	ipmask_edit[0].Focus = 0; //��ʾ������
	ipmask_edit[0].Visible = 0;

	/*���ص�ַ���ô����ı���***************************************/
	ipgate_edit[0] = addr_edit[0];

	ipgate_edit[0].xLeft = 132;
	ipgate_edit[0].yTop = 166;
	ipgate_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ipgate_edit[0].MaxLen = 15; //�ı���������볤��
	ipgate_edit[0].Focus = 0; //��ʾ������
	ipgate_edit[0].Visible = 0;

	/*��������ַ���ô����ı���***************************************/
	ipserver_edit[0] = addr_edit[0];

	ipserver_edit[0].xLeft = 132;
	ipserver_edit[0].yTop = 166;
	ipserver_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	ipserver_edit[0].MaxLen = 15; //�ı���������볤��
	ipserver_edit[0].Focus = 0; //��ʾ������
	ipserver_edit[0].Visible = 0;

	/*�޸Ĺ������봰���ı���***************************************/
	modi_engi_edit[0] = addr_edit[0];

	modi_engi_edit[0].xLeft = 132;
	modi_engi_edit[0].yTop = 166;
	modi_engi_edit[0].BoxLen = 0; //�ı���ǰ���볤��
	modi_engi_edit[0].MaxLen = 6; //�ı���������볤��
	modi_engi_edit[0].Focus = 0; //��ʾ������
	modi_engi_edit[0].Visible = 0;

	modi_engi_edit[1] = addr_edit[0];

	modi_engi_edit[1].xLeft = 132;
	modi_engi_edit[1].yTop = 166;
	modi_engi_edit[1].BoxLen = 0; //�ı���ǰ���볤��
	modi_engi_edit[1].MaxLen = 6; //�ı���������볤��
	modi_engi_edit[1].Focus = 0; //��ʾ������
	modi_engi_edit[1].Visible = 0;
}
#endif
//---------------------------------------------------------------------------
void EditUnInit(void) //�ı����ͷ�
{
	int i;

	/*����ͨ������***************************************/
	FreeEdit(&r2r_edit[0]);
	//���
	if (r2r_edit[0].Cursor_H != NULL)
	{
		free(r2r_edit[0].Cursor_H);
		r2r_edit[0].Cursor_H = NULL;
	}
}
//---------------------------------------------------------------------------
#ifdef YK_XT8126_BV_FKT
void ImageInit(void) //Image��ʼ��
{
	char jpgfilename[80];
	int i;
	int addr_xLeft, addr_yTop;

	addr_xLeft = 132;
	addr_yTop = 81;
	printf("image 1\n");

	if (num_back_image.image != NULL
		)
		FreeImage(&num_back_image);
	sprintf(jpgfilename, "%s/num.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_back_image);

	for (i = 0; i < 13; i++)
	{
		num_image[i].width = 28;
		num_image[i].height = 48;

		if (num_image[i].image == NULL)
			num_image[i].image = (unsigned char *) malloc(num_image[i].width * num_image[i].height * 2);
		SavePicS_D_Func(i * num_image[i].width, 0, num_image[i].width,
				num_image[i].height, num_image[i].image, num_back_image.width,
				num_back_image.height, num_back_image.image); //���
	}

	if (num_back_image.image != NULL)
		FreeImage(&num_back_image);

	for(i=0; i<10; i++)
	{
		if(num_yk[i].image != NULL)
		{
			FreeImage(&num_yk[i]);
		}
		sprintf(jpgfilename, "%s/num_0%d.jpg\0", currpath, i);
		ImageLoadFromFile(jpgfilename, &num_yk[i]);
		num_yk[i].width = 50;
		num_yk[i].height = 96;
	}

	if(num_yk[10].image != NULL)
	{
		FreeImage(&num_yk[10]);
	}
	sprintf(jpgfilename, "%s/num_10.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_yk[10]);
	num_yk[10].width = 50;
	num_yk[10].height = 96;

	//��ʾ��ӭ����
	if (talk_image.image != NULL)
		FreeImage(&talk_image);
	sprintf(jpgfilename, "%s/welcome.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_image);
	talk_image.xLeft = 0;
	talk_image.yTop = 0;

	//���ӶԽ�����ͼ��  ��Ԫ
	if (talk1_image.image != NULL)
		FreeImage(&talk1_image);
	sprintf(jpgfilename, "%s/talk1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk1_image);
	talk1_image.xLeft = 0;
	talk1_image.yTop = 0;

	//���ӶԽ�����ͼ��  Χǽ
	if (talk2_image.image != NULL)
		FreeImage(&talk2_image);
	sprintf(jpgfilename, "%s/talk2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk2_image);
	talk2_image.xLeft = 0;
	talk2_image.yTop = 0;

	//���ӶԽ� �������ӱ���ͼ��
	if (talk_connect_image.image != NULL)
		FreeImage(&talk_connect_image);
	sprintf(jpgfilename, "%s/talk_connect.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_connect_image);
	talk_connect_image.xLeft = 0;
	talk_connect_image.yTop = 0;

	//���ӶԽ� ͨ������ͼ��
	if (talk_start_image.image != NULL)
		FreeImage(&talk_start_image);
	sprintf(jpgfilename, "%s/talk_start.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_start_image);
	talk_start_image.xLeft = 0;
	talk_start_image.yTop = 0;

	//���뿪������ͼ��
	if (openlock_image.image != NULL)
		FreeImage(&openlock_image);
	sprintf(jpgfilename, "%s/openlock.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &openlock_image);
	openlock_image.xLeft = 0;
	openlock_image.yTop = 0;

	//�����Ѵ�ͼ��
	if (door_openlock.image != NULL)
		FreeImage(&door_openlock);
	sprintf(jpgfilename, "%s/dooropen.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &door_openlock);
	door_openlock.xLeft = 286;
	door_openlock.yTop = 358;

	if (sip_ok.image != NULL)
		FreeImage(&sip_ok);
	sprintf(jpgfilename, "%s/sip_ok.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_ok);
	sip_ok.xLeft = 630;
	sip_ok.yTop = 400;

	if (sip_error.image != NULL)
		FreeImage(&sip_error);
	sprintf(jpgfilename, "%s/sip_error.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_error);
	sip_error.xLeft = 630;
	sip_error.yTop = 400;

	//���ӱ���ͼ��
	if (watch_image.image != NULL)
		FreeImage(&watch_image);
	sprintf(jpgfilename, "%s/watch.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &watch_image);
	watch_image.xLeft = 0;
	watch_image.yTop = 0;

	//���ô��ڱ���ͼ��
	if (setupback_image.image != NULL)
		FreeImage(&setupback_image);
	sprintf(jpgfilename, "%s/setup.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupback_image);
	setupback_image.xLeft = 0;
	setupback_image.yTop = 0;

	//���������ڱ���ͼ��
	if (setupmain_image.image != NULL)
		FreeImage(&setupmain_image);
	sprintf(jpgfilename, "%s/setupmain.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupmain_image);
	setupmain_image.xLeft = 0;
	setupmain_image.yTop = 0;

	//��ѯ������Ϣ���� ����
	if (setup_info_image.image != NULL)
		FreeImage(&setup_info_image);
	sprintf(jpgfilename, "%s/setup_info.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_info_image);
	setup_info_image.xLeft = 0;
	setup_info_image.yTop = 0;

	//�޸Ĵ󴰿� ����
	if (setup_modify_image.image != NULL)
		FreeImage(&setup_modify_image);
	sprintf(jpgfilename, "%s/setup_modify_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_modify_image);
	setup_modify_image.xLeft = 0;
	setup_modify_image.yTop = 0;

	//������ַ���ô���Image
	if (addr_image[0].image != NULL)
		FreeImage(&addr_image[0]);
	sprintf(jpgfilename, "%s/addr_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &addr_image[0]);
	addr_image[0].xLeft = addr_xLeft;
	addr_image[0].yTop = addr_yTop;

	//MAC��ַ���ô���Image
	if (mac_image[0].image != NULL)
		FreeImage(&mac_image[0]);
	sprintf(jpgfilename, "%s/mac_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &mac_image[0]);
	mac_image[0].xLeft = addr_xLeft;
	mac_image[0].yTop = addr_yTop;

	//IP���ô���Image
	if (ip_image[0].image != NULL)
		FreeImage(&ip_image[0]);
	sprintf(jpgfilename, "%s/ip_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ip_image[0]);
	ip_image[0].xLeft = addr_xLeft;
	ip_image[0].yTop = addr_yTop;

	//�����������ô���Image
	if (ipmask_image[0].image != NULL)
		FreeImage(&ipmask_image[0]);
	sprintf(jpgfilename, "%s/ipmask_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipmask_image[0]);
	ipmask_image[0].xLeft = addr_xLeft;
	ipmask_image[0].yTop = addr_yTop;

	//�������ô���Image
	if (ipgate_image[0].image != NULL)
		FreeImage(&ipgate_image[0]);
	sprintf(jpgfilename, "%s/ipgate_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipgate_image[0]);
	ipgate_image[0].xLeft = addr_xLeft;
	ipgate_image[0].yTop = addr_yTop;

	//��������ַ���ô���Image
	if (ipserver_image[0].image != NULL
		)
		FreeImage(&ipserver_image[0]);
	sprintf(jpgfilename, "%s/ipserver_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipserver_image[0]);
	ipserver_image[0].xLeft = addr_xLeft;
	ipserver_image[0].yTop = addr_yTop;

	//�����������ô���Image
	if (engineer_pass_image[0].image != NULL)
		FreeImage(&engineer_pass_image[0]);
	sprintf(jpgfilename, "%s/engineer_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[0]);
	engineer_pass_image[0].xLeft = addr_xLeft;
	engineer_pass_image[0].yTop = addr_yTop;

	if (engineer_pass_image[1].image != NULL
		)
		FreeImage(&engineer_pass_image[1]);
	sprintf(jpgfilename, "%s/engineer_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[1]);
	engineer_pass_image[1].xLeft = addr_xLeft;
	engineer_pass_image[1].yTop = addr_yTop;

	//�����������ô���Image
	if (lock_pass_image[0].image != NULL)
		FreeImage(&lock_pass_image[0]);
	sprintf(jpgfilename, "%s/lock_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[0]);
	lock_pass_image[0].xLeft = addr_xLeft;
	lock_pass_image[0].yTop = addr_yTop;

	if (lock_pass_image[1].image != NULL
		)
		FreeImage(&lock_pass_image[1]);
	sprintf(jpgfilename, "%s/lock_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[1]);
	lock_pass_image[1].xLeft = addr_xLeft;
	lock_pass_image[1].yTop = addr_yTop;

	//���н���
	if (talkover_image.image != NULL)
		FreeImage(&talkover_image);
	sprintf(jpgfilename, "%s/talk_over.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talkover_image);
	talkover_image.xLeft = 0;
	talkover_image.yTop = 0;

}
#else
void ImageInit(void) //Image��ʼ��
{
	char jpgfilename[80];
	int i;
	int addr_xLeft, addr_yTop;

	addr_xLeft = 132;
	addr_yTop = 81;
	printf("image 1\n");

	if (num_back_image.image != NULL
		)
		FreeImage(&num_back_image);
	sprintf(jpgfilename, "%s/num.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_back_image);

	for (i = 0; i < 13; i++)
	{
		num_image[i].width = 28;
		num_image[i].height = 48;

		if (num_image[i].image == NULL)
			num_image[i].image = (unsigned char *) malloc(num_image[i].width * num_image[i].height * 2);
		SavePicS_D_Func(i * num_image[i].width, 0, num_image[i].width,
				num_image[i].height, num_image[i].image, num_back_image.width,
				num_back_image.height, num_back_image.image); //���
	}

	if (num_back_image.image != NULL)
		FreeImage(&num_back_image);

	for(i=0; i<10; i++)
	{
		if(num_yk[i].image != NULL)
		{
			FreeImage(&num_yk[i]);
		}
		sprintf(jpgfilename, "%s/num_0%d.jpg\0", currpath, i);
		ImageLoadFromFile(jpgfilename, &num_yk[i]);
		num_yk[i].width = 66;
		num_yk[i].height = 96;
	}

	if(num_yk[10].image != NULL)
	{
		FreeImage(&num_yk[10]);
	}
	sprintf(jpgfilename, "%s/num_10.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &num_yk[10]);
	num_yk[10].width = 66;
	num_yk[10].height = 96;

	//��ʾ��ӭ����
	if (talk_image.image != NULL)
		FreeImage(&talk_image);
	sprintf(jpgfilename, "%s/welcome.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_image);
	talk_image.xLeft = 0;
	talk_image.yTop = 0;

	//���ӶԽ�����ͼ��  ��Ԫ
	if (talk1_image.image != NULL)
		FreeImage(&talk1_image);
	sprintf(jpgfilename, "%s/talk1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk1_image);
	talk1_image.xLeft = 0;
	talk1_image.yTop = 0;

	//���ӶԽ�����ͼ��  Χǽ
	if (talk2_image.image != NULL)
		FreeImage(&talk2_image);
	sprintf(jpgfilename, "%s/talk2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk2_image);
	talk2_image.xLeft = 0;
	talk2_image.yTop = 0;

	//���ӶԽ� �������ӱ���ͼ��
	if (talk_connect_image.image != NULL)
		FreeImage(&talk_connect_image);
	sprintf(jpgfilename, "%s/talk_connect.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_connect_image);
	talk_connect_image.xLeft = 0;
	talk_connect_image.yTop = 0;

	//���ӶԽ� ͨ������ͼ��
	if (talk_start_image.image != NULL)
		FreeImage(&talk_start_image);
	sprintf(jpgfilename, "%s/talk_start.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talk_start_image);
	talk_start_image.xLeft = 0;
	talk_start_image.yTop = 0;

	//���뿪������ͼ��
	if (openlock_image.image != NULL)
		FreeImage(&openlock_image);
	sprintf(jpgfilename, "%s/openlock.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &openlock_image);
	openlock_image.xLeft = 0;
	openlock_image.yTop = 0;

	//�����Ѵ�ͼ��
	if (door_openlock.image != NULL)
		FreeImage(&door_openlock);
	sprintf(jpgfilename, "%s/dooropen.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &door_openlock);
	door_openlock.xLeft = 258;
	door_openlock.yTop = 300;

	if (sip_ok.image != NULL)
		FreeImage(&sip_ok);
	sprintf(jpgfilename, "%s/sip_ok.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_ok);
	sip_ok.xLeft = 630;
	sip_ok.yTop = 400;

	if (sip_error.image != NULL)
		FreeImage(&sip_error);
	sprintf(jpgfilename, "%s/sip_error.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &sip_error);
	sip_error.xLeft = 630;
	sip_error.yTop = 400;

	//���ӱ���ͼ��
	if (watch_image.image != NULL)
		FreeImage(&watch_image);
	sprintf(jpgfilename, "%s/watch.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &watch_image);
	watch_image.xLeft = 0;
	watch_image.yTop = 0;

	//���ô��ڱ���ͼ��
	if (setupback_image.image != NULL)
		FreeImage(&setupback_image);
	sprintf(jpgfilename, "%s/setup.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupback_image);
	setupback_image.xLeft = 0;
	setupback_image.yTop = 0;

	//���������ڱ���ͼ��
	if (setupmain_image.image != NULL)
		FreeImage(&setupmain_image);
	sprintf(jpgfilename, "%s/setupmain.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setupmain_image);
	setupmain_image.xLeft = 0;
	setupmain_image.yTop = 0;

	//��ѯ������Ϣ���� ����
	if (setup_info_image.image != NULL)
		FreeImage(&setup_info_image);
	sprintf(jpgfilename, "%s/setup_info.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_info_image);
	setup_info_image.xLeft = 0;
	setup_info_image.yTop = 0;

	//�޸Ĵ󴰿� ����
	if (setup_modify_image.image != NULL)
		FreeImage(&setup_modify_image);
	sprintf(jpgfilename, "%s/setup_modify_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &setup_modify_image);
	setup_modify_image.xLeft = 0;
	setup_modify_image.yTop = 0;

	//������ַ���ô���Image
	if (addr_image[0].image != NULL)
		FreeImage(&addr_image[0]);
	sprintf(jpgfilename, "%s/addr_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &addr_image[0]);
	addr_image[0].xLeft = addr_xLeft;
	addr_image[0].yTop = addr_yTop;

	//MAC��ַ���ô���Image
	if (mac_image[0].image != NULL)
		FreeImage(&mac_image[0]);
	sprintf(jpgfilename, "%s/mac_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &mac_image[0]);
	mac_image[0].xLeft = addr_xLeft;
	mac_image[0].yTop = addr_yTop;

	//IP���ô���Image
	if (ip_image[0].image != NULL)
		FreeImage(&ip_image[0]);
	sprintf(jpgfilename, "%s/ip_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ip_image[0]);
	ip_image[0].xLeft = addr_xLeft;
	ip_image[0].yTop = addr_yTop;

	//�����������ô���Image
	if (ipmask_image[0].image != NULL)
		FreeImage(&ipmask_image[0]);
	sprintf(jpgfilename, "%s/ipmask_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipmask_image[0]);
	ipmask_image[0].xLeft = addr_xLeft;
	ipmask_image[0].yTop = addr_yTop;

	//�������ô���Image
	if (ipgate_image[0].image != NULL)
		FreeImage(&ipgate_image[0]);
	sprintf(jpgfilename, "%s/ipgate_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipgate_image[0]);
	ipgate_image[0].xLeft = addr_xLeft;
	ipgate_image[0].yTop = addr_yTop;

	//��������ַ���ô���Image
	if (ipserver_image[0].image != NULL
		)
		FreeImage(&ipserver_image[0]);
	sprintf(jpgfilename, "%s/ipserver_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &ipserver_image[0]);
	ipserver_image[0].xLeft = addr_xLeft;
	ipserver_image[0].yTop = addr_yTop;

	//�����������ô���Image
	if (engineer_pass_image[0].image != NULL)
		FreeImage(&engineer_pass_image[0]);
	sprintf(jpgfilename, "%s/engineer_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[0]);
	engineer_pass_image[0].xLeft = addr_xLeft;
	engineer_pass_image[0].yTop = addr_yTop;

	if (engineer_pass_image[1].image != NULL
		)
		FreeImage(&engineer_pass_image[1]);
	sprintf(jpgfilename, "%s/engineer_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &engineer_pass_image[1]);
	engineer_pass_image[1].xLeft = addr_xLeft;
	engineer_pass_image[1].yTop = addr_yTop;

	//�����������ô���Image
	if (lock_pass_image[0].image != NULL)
		FreeImage(&lock_pass_image[0]);
	sprintf(jpgfilename, "%s/lock_pass_image1.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[0]);
	lock_pass_image[0].xLeft = addr_xLeft;
	lock_pass_image[0].yTop = addr_yTop;

	if (lock_pass_image[1].image != NULL
		)
		FreeImage(&lock_pass_image[1]);
	sprintf(jpgfilename, "%s/lock_pass_image2.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &lock_pass_image[1]);
	lock_pass_image[1].xLeft = addr_xLeft;
	lock_pass_image[1].yTop = addr_yTop;

	//���н���
	if (talkover_image.image != NULL)
		FreeImage(&talkover_image);
	sprintf(jpgfilename, "%s/talk_over.jpg\0", currpath);
	ImageLoadFromFile(jpgfilename, &talkover_image);
	talkover_image.xLeft = 0;
	talkover_image.yTop = 0;

}
#endif
//---------------------------------------------------------------------------
void ImageUnInit(void) //Image�ͷ�
{
	int i;
	//��ҳ����ͼ��
	printf("image 0\n");
	for (i = 0; i < 13; i++)
		if (num_image[i].image != NULL)
			FreeImage(&num_image[i]);

	printf("image 1\n");
	//���ӶԽ�����ͼ��  ��Ԫ
	if (talk1_image.image != NULL)
		FreeImage(&talk1_image);

	//���ӶԽ�����ͼ��  Χǽ
	if (talk2_image.image != NULL)
		FreeImage(&talk2_image);

	//���ӶԽ� �������ӱ���ͼ��
	if (talk_connect_image.image != NULL)
		FreeImage(&talk_connect_image);

	//���ӶԽ� ͨ������ͼ��
	if (talk_start_image.image != NULL)
		FreeImage(&talk_start_image);

	//���뿪������ͼ��
	if (openlock_image.image != NULL)
		FreeImage(&openlock_image);

	//�����Ѵ�ͼ��
	if (door_openlock.image != NULL)
		FreeImage(&door_openlock);

	printf("image 2\n");

	if (sip_ok.image != NULL)
		FreeImage(&sip_ok);

	if (sip_error.image != NULL)
		FreeImage(&sip_error);

	//���ӱ���ͼ��
	if (watch_image.image != NULL)
		FreeImage(&watch_image);
	printf("image 3\n");
	//���ô��ڱ���ͼ��
	if (setupback_image.image != NULL)
		FreeImage(&setupback_image);
	printf("image 4\n");
	//�������ô���Image
	for (i = 0; i < 1; i++)
		FreeImage(&ip_image[i]);
	printf("image 5\n");
	//�������ô���Image
	for (i = 0; i < 1; i++)
		FreeImage(&addr_image[i]);
	printf("image 6\n");
	//�����������ô���Image
	for (i = 0; i < 2; i++)
		FreeImage(&engineer_pass_image[i]);

	//�����������ô���Image
	for (i = 0; i < 2; i++)
		FreeImage(&lock_pass_image[i]);

	//���ӶԽ� ͨ������ ����Label
	if (label_talk_start.image != NULL)
		free(label_talk_start.image);

	// sip���н���
	if (talkover_image.image != NULL)
		FreeImage(&talkover_image);
}
//---------------------------------------------------------------------------
void LabelInit(void) //Label��ʼ��
{
}
//---------------------------------------------------------------------------
void LabelUnInit(void) //Label�ͷ�
{
}
//---------------------------------------------------------------------------

//��ʾLabel
/*void ShowLabel(struct TLabel *t_label, int refreshflag)
 {
 if(refreshflag == REFRESH)
 RestorePicBuf_Func(t_label->xLeft - 2, t_label->yTop - 2, t_label->width, t_label->height,
 t_label->image, 0);
 if(refreshflag == HILIGHT)
 {
 printf("t_label->xLeft=%d,t_label->yTop=%d,t_label->width=%d,t_label->height=%d \n" ,
 t_label->xLeft, t_label->yTop, t_label->width, t_label->height);
 RestorePicBuf_Func(t_label->xLeft - 2, t_label->yTop -2, t_label->width, t_label->height,
 t_label->image_h, 0);
 }
 outxy24(t_label->xLeft, t_label->yTop, 3, cWhite, 1, 1, t_label->Text, 0, 0);
 }                  */
//---------------------------------------------------------------------------
//��ʾClock
char OldClock[20] = "####################"; //2004-33-33 33:33:33
void ShowClock(struct TLabel *t_label, int refreshflag)
{
	if (refreshflag == REFRESH)
		RestorePicBuf_Func(t_label->xLeft, t_label->yTop, t_label->width,
				t_label->height, t_label->image, 0);
	outxy16(t_label->xLeft + 15, t_label->yTop + 2, 1, cBlack, 1, 1,
			t_label->Text, 0, 0);
}
//---------------------------------------------------------------------------
void BuffUninit(void)
{
}
//---------------------------------------------------------------------------
