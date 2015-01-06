#include <stdio.h>
#include   <time.h>
#include <stdlib.h>
#include <math.h>
#include <semaphore.h>       //sem_t

#define CommonH
#include "common.h"

void SaveToFlash(int savetype);    //向Flash中存储文件

void DisplaySetupWindow(void);    //显示设置窗口（一级）
void OperateSetupWindow(short wintype, int currwindow);  //设置窗口操作（一级）

void DisplaySetupMainWindow(void);    //显示设置主界面窗口（二级）
void OperateSetupMainWindow(short wintype, int currwindow);  //设置主界面操作（二级）

void DisplaySetupInfoWindow(void);    //显示查询本机信息窗口
void OperateSetupInfoWindow(short wintype, int currwindow);  //查询本机信息操作

void DisplayAddressWindow(void);    //显示本机地址变更窗口
void OperateAddressWindow(short wintype, int currwindow);  //本机地址变更操作

void DisplayModiPassWindow(int passtype);   //显示修改密码窗口（三级、四级） 0 管理密码 1 开锁密码
void OperateModiPassWindow(short wintype, int currwindow);  //修改密码操作（三级、四级）

void DisplayMacWindow(void);    //显示网卡地址更窗口
void OperateMacWindow(short wintype, int currwindow);  //网卡地址变更操作

void DisplayIPWindow(void);    //显示IP地址设置窗口（三级）
void OperateIPWindow(short wintype, int currwindow);  //修改IP地址设置操作（三级）

void DisplayIPMaskWindow(void);    //显示子网掩码窗口（三级）
void OperateIPMaskWindow(short wintype, int currwindow);  //修改子网掩码操作（三级）

void DisplayIPGateWindow(void);    //显示网关地址窗口（三级）
void OperateIPGateWindow(short wintype, int currwindow);  //修改网关地址操作（三级）

void DisplayIPServerWindow(void);    //显示服务器地址窗口（三级）
void OperateIPServerWindow(short wintype, int currwindow);  //修改服务器地址操作（三级）

void DisplayCalibrateWindow(void);    //校准触摸屏窗口（二级）
void OperateCalibrateWindow(short wintype, int currwindow);  //校准触摸屏操作（二级）
void DisplayCalibrateTipWindow(int ltype);    //显示校准触摸屏完成窗口（三级，弹出）
void OperateCalibrateTipWindow(short wintype);  //校准触摸屏完成窗口操作（三级，弹出）
//画十字函数   t_Flag 0--清除  1--写
void WriteCross(int xLeft, int yTop, int t_Flag, int PageNo);
int CrossPosX[4]={50, 750, 50, 750};
int CrossPosY[4]={50, 50, 430, 430};
int Calib_X[4];
int Calib_Y[4];

void PlayPassErrorWav(void);
void PlayModiSuccWav(void);
void PlayModiFailWav(void);
//---------------------------------------------------------------------------
void DisplaySetupWindow(void)    //显示设置窗口（一级）
{
  int i;
  char vername[30];

  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 6)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 6;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setupback_image, 0);


  setup_pass_edit.BoxLen = 0;
  setup_pass_edit.Text[0] = '\0';
  CurrBox = 0;
  //打开光标
  WriteCursor(&setup_pass_edit, 1, 1, 0);
  DisplayEdit(&setup_pass_edit, 0);

//  sprintf(vername, "%s %s\0", SOFTWAREVER, SERIALNUM);
//  outxy16(ip_image[1].xLeft + 15,
//                              ip_image[1].yTop + 202 , 1, cBlack, 1, 1, vername, 0, 0);    
  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateSetupWindow(short wintype, int currwindow)  //设置窗口操作（一级）
{
  int i, j;
  char str[5];
  int isOk;

  //停止光标,并清一下
  WriteCursor(&setup_pass_edit, 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(setup_pass_edit.BoxLen < setup_pass_edit.MaxLen)
            {
             str[0] = '0' + wintype;
             str[1] = '\0';
             strcat(setup_pass_edit.Text, str);
             str[0] = '*';
             str[1] = '\0';
             #if 0
               outxy24(setup_pass_edit.xLeft + setup_pass_edit.CursorX + setup_pass_edit.BoxLen*setup_pass_edit.fWidth,
                       setup_pass_edit.yTop + setup_pass_edit.CursorY - 2, 2, cBlack, 1, 1, str, 0, 0);
             #else
               outxy_pic_num(setup_pass_edit.xLeft + setup_pass_edit.CursorX + setup_pass_edit.BoxLen*num_yk[0].width,
                             setup_pass_edit.yTop +setup_pass_edit.CursorY, str, 0);
             #endif
             setup_pass_edit.BoxLen ++;
            }
           break;
    case 11: //确认
          printf("setup_pass_edit.BoxLen = %d, setup_pass_edit.Text = %s, LocalCfg.EngineerPass = %s\n",
                  setup_pass_edit.BoxLen, setup_pass_edit.Text, LocalCfg.EngineerPass);
          setup_pass_edit.Text[setup_pass_edit.BoxLen] = '\0';
          LocalCfg.EngineerPass[6] = '\0';
          if(strcmp(setup_pass_edit.Text, LocalCfg.EngineerPass) == 0)
           {
             DisplaySetupMainWindow();
           }
          else
           {
            PlayPassErrorWav();

            //清除输入框
            setup_pass_edit.Text[0] = 0;
            setup_pass_edit.BoxLen = 0;
            DisplayEdit(&setup_pass_edit, 0);
           }
          break;
    case 10: //返回
          if(setup_pass_edit.BoxLen != 0)
           {
            //清除输入框
            setup_pass_edit.Text[0] = 0;
            setup_pass_edit.BoxLen = 0;
            DisplayEdit(&setup_pass_edit, 0);
           }
          else
        	  printf("***OperateSetupWindow()******DisplayMainWindow(0)***\n");
            DisplayMainWindow(0);
          break;
   }
  if(Local.CurrentWindow == 6)
    //打开光标
    WriteCursor(&setup_pass_edit, 1, 1, 0);
}
//---------------------------------------------------------------------------
void DisplaySetupMainWindow(void)    //显示设置主界面窗口（二级）
{
  int i;
  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 60)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 60;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setupmain_image, 0);

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateSetupMainWindow(short wintype, int currwindow)  //设置界面操作（二级）
{
  int i, j;

  printf("wintype = %d\n", wintype);

  switch(wintype)
   {
    case 1: //查询本机信息
           DisplaySetupInfoWindow();
           break;
    case 2: //本机地址变更
           DisplayAddressWindow();
           break;
    case 3: //管理密码
           DisplayModiPassWindow(0);
           break;
    case 4: //开锁密码
           DisplayModiPassWindow(1);
           break;
    case 5: //网卡地址
           DisplayMacWindow();
           break;
    case 6: //IP地址
           DisplayIPWindow();
           break;
    case 7: //子网掩码
           DisplayIPMaskWindow();
           break;
    case 8: //网关地址
           DisplayIPGateWindow();
           break;
    case 9: //服务器地址
           DisplayIPServerWindow();
           break;
    case 10:  //首页
    	printf("***OperateSetupMainWindow()******DisplayMainWindow(0)***\n");
           DisplayMainWindow(0);
           break;
   }
}
//---------------------------------------------------------------------------
void PlayPassErrorWav(void)
{
  WaitAudioUnuse(200); //等待声卡空闲
  sprintf(wavFile, "%s/passerror.wav\0", wavPath);
  StartPlayWav(wavFile, 0);
}
//---------------------------------------------------------------------------
void PlayModiSuccWav(void)
{
  WaitAudioUnuse(200); //等待声卡空闲
  sprintf(wavFile, "%s/modisucc.wav\0", wavPath);
  StartPlayWav(wavFile, 0);
}
//---------------------------------------------------------------------------
void PlayModiFailWav(void)
{
  WaitAudioUnuse(200); //等待声卡空闲
  sprintf(wavFile, "%s/modifail.wav\0", wavPath);
  StartPlayWav(wavFile, 0);
}
//---------------------------------------------------------------------------
void DisplaySetupInfoWindow(void)    //显示查询本机信息窗口
{
  int i;
  char tmp_text[20];
  int xLeft[2][6] = {
                     {288, 288, 288, 288, 288, 288},
                     {115, 115, 115, 115, 115, 115}
                    };
  int yTop[2][6] = {
                     {18, 75, 136, 197, 258, 317},
                     {9+7, 38+8, 68+7, 99+7, 129+6, 159+6}
/*                     {9, 38, 68, 99, 129, 159}*/
                    };
  int nPos;

  nPos = 0;

  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 61)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 61;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setup_info_image, 0);

  //本机地址
  memcpy(tmp_text, LocalCfg.Addr + 1, Local.AddrLen - 1);
  tmp_text[Local.AddrLen - 1] = '\0';

  outxy_pic_num1(xLeft[nPos][0], yTop[nPos][0], tmp_text, 0);

  //网卡地址
  sprintf(tmp_text, "%02X:%02X:%02X\0", LocalCfg.Mac_Addr[3], LocalCfg.Mac_Addr[4], LocalCfg.Mac_Addr[5]);
  outxy_pic_num1(xLeft[nPos][1], yTop[nPos][1], tmp_text, 0);

  //IP地址
  sprintf(tmp_text, "%03d.%03d.%03d.%03d\0", LocalCfg.IP[0], LocalCfg.IP[1],
                    LocalCfg.IP[2], LocalCfg.IP[3]);
  outxy_pic_num1(xLeft[nPos][2], yTop[nPos][2], tmp_text, 0);

  //子网掩码
  sprintf(tmp_text, "%03d.%03d.%03d.%03d\0", LocalCfg.IP_Mask[0], LocalCfg.IP_Mask[1],
                    LocalCfg.IP_Mask[2], LocalCfg.IP_Mask[3]);

  outxy_pic_num1(xLeft[nPos][3], yTop[nPos][3], tmp_text, 0);

  //网关
  sprintf(tmp_text, "%03d.%03d.%03d.%03d\0", LocalCfg.IP_Gate[0], LocalCfg.IP_Gate[1],
                    LocalCfg.IP_Gate[2], LocalCfg.IP_Gate[3]);
  outxy_pic_num1(xLeft[nPos][4], yTop[nPos][4], tmp_text, 0);

  //服务器地址
  sprintf(tmp_text, "%03d.%03d.%03d.%03d\0", LocalCfg.IP_Server[0], LocalCfg.IP_Server[1],
                    LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
  outxy_pic_num1(xLeft[nPos][5], yTop[nPos][5], tmp_text, 0);

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateSetupInfoWindow(short wintype, int currwindow)  //查询本机信息操作
{
  int i, j;

  printf("wintype = %d\n", wintype);

  switch(wintype)
   {
    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
}
//---------------------------------------------------------------------------
void DisplayAddressWindow(void)    //显示房号设置窗口（三级）
{
  int i;
  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 62)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 62;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setup_modify_image, 0);
  DisplayImage(&addr_image[0], 0);


  addr_edit[0].Text[0] = '\0';
  addr_edit[0].BoxLen = 0;         //文本框当前输入长度
  addr_edit[0].MaxLen = 7;         //文本框最大输入长度
  addr_edit[0].Focus = 0;        //显示输入光标
  addr_edit[0].Visible = 0;

  DisplayEdit(&addr_edit[0], 0);

  CurrBox = 0;
  //打开光标
  WriteCursor(&addr_edit[0], 1, 1, 0);

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateAddressWindow(short wintype, int currwindow)  //房号设置操作（三级）
{
  int i;
  char str[10];
  int AddrOK;

  //停止光标,并清一下
  WriteCursor(&addr_edit[CurrBox], 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(addr_edit[CurrBox].BoxLen < addr_edit[CurrBox].MaxLen)
            {
             str[0] = '0' + wintype;
             str[1] = '\0';
            // strcat(ip_edit.Text, str);
             memcpy(addr_edit[CurrBox].Text + addr_edit[CurrBox].BoxLen, str, 1);
             #if 0
               outxy16(addr_edit[CurrBox].xLeft + addr_edit[CurrBox].CursorX + addr_edit[CurrBox].BoxLen*addr_edit[CurrBox].fWidth,
                       addr_edit[CurrBox].yTop  + addr_edit[CurrBox].CursorY - 2, 1, cBlack, 1, 1, str, 0, 0);
             #else
               outxy_pic_num1(addr_edit[CurrBox].xLeft + addr_edit[CurrBox].CursorX + addr_edit[CurrBox].BoxLen*num_image[0].width,
                             addr_edit[CurrBox].yTop +addr_edit[CurrBox].CursorY, str, 0);
             #endif

             addr_edit[CurrBox].BoxLen ++;
            }
           break;
    case 11: //确定   K
          AddrOK = 0;
          if((addr_edit[0].BoxLen == 4)||(addr_edit[0].BoxLen == 7))
           {
            AddrOK = 1;
           }
          if(AddrOK == 0)
           {
            PlayModiFailWav();
            break;
           }
          if(addr_edit[0].BoxLen == 7)
           {
            LocalCfg.Addr[0] = 'M';
            memcpy(LocalCfg.Addr + 1, addr_edit[0].Text, 7);
            memcpy(LocalCfg.Addr + 8, "0000", 4);
           }
          else
           {
            LocalCfg.Addr[0] = 'W';
            memcpy(LocalCfg.Addr + 1, addr_edit[0].Text, 4);
            memcpy(LocalCfg.Addr + 5, "0000000", 7);
           }
          LocalCfg.Addr[12] == '\0';

          SaveToFlash(4);    //向Flash中存储文件

          if(LocalCfg.Addr[0] == 'M')
           {
            Local.AddrLen = 8;  //地址长度  S 12  B 6 M 8 H 6
           }
          if(LocalCfg.Addr[0] == 'W')
           {
            Local.AddrLen = 5;  //地址长度  S 12  B 6 M 8 H 6
           }

          PlayModiSuccWav();

          DisplaySetupMainWindow();
          break;
    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
  if(Local.CurrentWindow == 62)
    //显示光标
    WriteCursor(&addr_edit[CurrBox], 1, 1, 0);
}
//---------------------------------------------------------------------------
void DisplayModiPassWindow(int passtype)   //显示修改密码窗口（三级、四级） 0 管理密码 1 开锁密码
{
  int i;

  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == (63 + passtype))
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = (63 + passtype);
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  switch(passtype)
   {
    case 0:
           pass_win_no = 0;
           DisplayImage(&setup_modify_image, 0);
           DisplayImage(&engineer_pass_image[0], 0);
           break;
    case 1:
           pass_win_no = 1;
           DisplayImage(&setup_modify_image, 0);
           DisplayImage(&lock_pass_image[0], 0);
           break;
   }
  for(i=0; i<2; i++)
   {
    modi_engi_edit[i].Text[0] = '\0';
    modi_engi_edit[i].BoxLen = 0;         //文本框当前输入长度
    modi_engi_edit[i].MaxLen = 6;         //文本框最大输入长度
    modi_engi_edit[i].Focus = 0;        //显示输入光标
    modi_engi_edit[i].Visible = 0;
   } 

  DisplayEdit(&modi_engi_edit[0], 0);

  Modi_Pass_Input_Num = 0;  //修改密码输入次数
  CurrBox = 0;
  //打开光标
  WriteCursor(&modi_engi_edit[0], 1, 1, 0);

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateModiPassWindow(short wintype, int currwindow)  //修改密码操作（三级、四级）
{
  int i;
  char str[10];
  int isOK;

  //停止光标,并清一下
  WriteCursor(&modi_engi_edit[CurrBox], 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(modi_engi_edit[CurrBox].BoxLen < modi_engi_edit[CurrBox].MaxLen)
            {
             str[0] = '0' + wintype;
             str[1] = '\0';
             strcat(modi_engi_edit[CurrBox].Text, str);

             str[0] = '*';
             str[1] = '\0';

             #if 0
               outxy16(modi_engi_edit[CurrBox].xLeft + modi_engi_edit[CurrBox].CursorX + modi_engi_edit[CurrBox].BoxLen*modi_engi_edit[CurrBox].fWidth,
                       modi_engi_edit[CurrBox].yTop  + modi_engi_edit[0].CursorY - 2, 3, cBlack, 1, 1, str, 0, 0);
             #else
               outxy_pic_num1(modi_engi_edit[CurrBox].xLeft + modi_engi_edit[CurrBox].CursorX + modi_engi_edit[CurrBox].BoxLen*num_image[0].width,
                             modi_engi_edit[CurrBox].yTop + modi_engi_edit[CurrBox].CursorY, str, 0);
             #endif

             modi_engi_edit[CurrBox].BoxLen ++;
            }
           break;
    case 11: //确定   K
          isOK = 1;
          if(modi_engi_edit[CurrBox].BoxLen < modi_engi_edit[CurrBox].MaxLen)
           {
            isOK = 0;
            break;
           }

          if(isOK == 1)
           {
            if(CurrBox == 0)
             {
              CurrBox ++;
              if(Local.CurrentWindow == 63)  //管理密码
                DisplayImage(&engineer_pass_image[CurrBox], 0);
              else                           //开锁密码
                DisplayImage(&lock_pass_image[CurrBox], 0);
              modi_engi_edit[CurrBox].Text[0] = '\0';
              modi_engi_edit[CurrBox].BoxLen = 0;
              DisplayEdit(&modi_engi_edit[CurrBox], 0);
              break;
             }
            modi_engi_edit[0].Text[modi_engi_edit[0].BoxLen] = '\0';
            modi_engi_edit[1].Text[modi_engi_edit[1].BoxLen] = '\0';
            if(strcmp(modi_engi_edit[0].Text, modi_engi_edit[1].Text) == 0)
             {
              if(Local.CurrentWindow == 63)  //管理密码
                strcpy(LocalCfg.EngineerPass, modi_engi_edit[0].Text);
              else                           //开锁密码
                strcpy(LocalCfg.OpenLockPass, modi_engi_edit[0].Text);
              SaveToFlash(4);    //向Flash中存储文
              isOK = 1;
             }
            else
             isOK = 0;
           }
          if(isOK == 0)
           {
             PlayModiFailWav();
           }
          else
           {
             PlayModiSuccWav();
           }

          //清除输入框
          modi_engi_edit[0].Text[0] = 0;
          modi_engi_edit[0].BoxLen = 0;
          DisplayEdit(&modi_engi_edit[0], 0);

          modi_engi_edit[1].Text[0] = 0;
          modi_engi_edit[1].BoxLen = 0;
          DisplayEdit(&modi_engi_edit[1], 0);
          CurrBox = 0;

          DisplaySetupMainWindow();
          break;
    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
  if((Local.CurrentWindow == 63)||(Local.CurrentWindow == 64))
    //显示光标
    WriteCursor(&modi_engi_edit[CurrBox], 1, 1, 0);
}
//---------------------------------------------------------------------------
void DisplayMacWindow(void)    //显示网卡地址更窗口
{
  int i;
  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 71)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 71;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setup_modify_image, 0);
  DisplayImage(&mac_image[0], 0);

  mac_edit[0].Text[0] = '\0';
  mac_edit[0].BoxLen = 0;         //文本框当前输入长度
  mac_edit[0].MaxLen = 8;         //文本框最大输入长度
  mac_edit[0].Focus = 0;        //显示输入光标
  mac_edit[0].Visible = 0;

  //网卡地址
  DisplayEdit(&mac_edit[0], 0);
  strcpy(mac_edit[0].Text, "  :  :  \0");
  #if 0
    outxy16(mac_edit[0].xLeft + mac_edit[0].CursorX + mac_edit[0].BoxLen*mac_edit[0].fWidth,
                      mac_edit[0].yTop  + mac_edit[0].CursorY - 2, 0, cBlack, 1, 1, mac_edit[0].Text, 0, 0);
  #else
    outxy_pic_num1(mac_edit[0].xLeft + mac_edit[0].CursorX + mac_edit[0].BoxLen*num_image[0].width,
                  mac_edit[0].yTop + mac_edit[0].CursorY, mac_edit[0].Text, 0);
  #endif
  mac_edit[0].BoxLen = 0;

  CurrBox = 0;

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateMacWindow(short wintype, int currwindow)  //网卡地址变更操作
{
  int i,j;
  char str[10];
  int hex_data[6];
  char TmpText[5][5];
  int input_ok;

  //停止光标,并清一下
  WriteCursor(&mac_edit[CurrBox], 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(mac_edit[CurrBox].BoxLen < mac_edit[CurrBox].MaxLen)
            {
             if(wintype < 10)
               str[0] = '0' + wintype;
             else
               str[0] = 'A' + wintype - 10;
             str[1] = '\0';
             memcpy(mac_edit[CurrBox].Text + mac_edit[CurrBox].BoxLen, str, 1);
             #if 0
               outxy16(mac_edit[CurrBox].xLeft + mac_edit[CurrBox].CursorX + mac_edit[CurrBox].BoxLen*mac_edit[CurrBox].fWidth,
                       mac_edit[CurrBox].yTop  + mac_edit[0].CursorY - 2, 1, cBlack, 1, 1, str, 0, 0);
             #else
               outxy_pic_num1(mac_edit[CurrBox].xLeft + mac_edit[CurrBox].CursorX + mac_edit[CurrBox].BoxLen*num_image[0].width,
                             mac_edit[CurrBox].yTop + mac_edit[CurrBox].CursorY, str, 0);
             #endif
             mac_edit[CurrBox].BoxLen ++;
             if(mac_edit[CurrBox].Text[mac_edit[CurrBox].BoxLen] == ':')
             mac_edit[CurrBox].BoxLen ++;
            }
           break;
    case 11: //确定   K
          input_ok = 1;
          if(mac_edit[0].BoxLen < (mac_edit[0].MaxLen - 1))
           {
            PlayModiFailWav();
            input_ok = 0;
            break;
           }

          if(input_ok == 1)
           {
            memcpy(TmpText[0], mac_edit[0].Text, 2);
            TmpText[0][2] = '\0';
            memcpy(TmpText[1], mac_edit[0].Text + 3, 2);
            TmpText[1][2] = '\0';
            memcpy(TmpText[2], mac_edit[0].Text + 6, 2);
            TmpText[2][2] = '\0';

            LocalCfg.Mac_Addr[0] = 0x00;
            LocalCfg.Mac_Addr[1] = 0x50;
            LocalCfg.Mac_Addr[2] = 0x2A;
            for(i=0; i<3; i++)
             if(strlen(TmpText[i]) != 2)
              {
               strcpy(str, TmpText[i]);
               strcpy(TmpText[i], "0");
               strcat(TmpText[i], str);
               TmpText[i][2] = '\0';
              }
            for(i=0; i<3; i++)
             {
              sscanf(TmpText[i], "%x", &hex_data[i]);
              LocalCfg.Mac_Addr[i+3] = hex_data[i];
             }

            SaveToFlash(4);    //向Flash中存储文件

            RefreshNetSetup(1); //刷新网络设置

            PlayModiSuccWav();

            //发送免费ARP
            SendFreeArp();

            DisplaySetupMainWindow();
           }
          break;

    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
  if(Local.CurrentWindow == 71)
    //显示光标
    WriteCursor(&mac_edit[CurrBox], 1, 1, 0);
}
//---------------------------------------------------------------------------
void DisplayIPWindow(void)    //显示IP地址设置窗口（三级）
{
  int i;

  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 72)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 72;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setup_modify_image, 0);
  DisplayImage(&ip_image[0], 0);

  ip_edit[0].Text[0] = '\0';
  ip_edit[0].BoxLen = 0;         //文本框当前输入长度
  ip_edit[0].MaxLen = 15;         //文本框最大输入长度
  ip_edit[0].Focus = 0;        //显示输入光标
  ip_edit[0].Visible = 0;

  //IP地址
  DisplayEdit(&ip_edit[0], 0);
  strcpy(ip_edit[0].Text, "   .   .   .   \0");
  outxy_pic_num1(ip_edit[0].xLeft + ip_edit[0].CursorX + ip_edit[0].BoxLen*num_image[0].width,
                  ip_edit[0].yTop + ip_edit[0].CursorY, ip_edit[0].Text, 0);

  ip_edit[0].BoxLen = 0;

  CurrBox = 0;
  //打开光标
  WriteCursor(&ip_edit[0], 1, 1, 0);

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateIPWindow(short wintype, int currwindow)  //IP地址设置操作（三级）
{
  int i,j;
  char str[10];
  int hex_data[6];
  char TmpText[5][5];
  int input_ok;

  //停止光标,并清一下
  WriteCursor(&ip_edit[CurrBox], 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(ip_edit[CurrBox].BoxLen < ip_edit[CurrBox].MaxLen)
            {
             str[0] = '0' + wintype;
             str[1] = '\0';
             memcpy(ip_edit[CurrBox].Text + ip_edit[CurrBox].BoxLen, str, 1);
             outxy_pic_num1(ip_edit[CurrBox].xLeft + ip_edit[CurrBox].CursorX + ip_edit[CurrBox].BoxLen*num_image[0].width,
                             ip_edit[CurrBox].yTop + ip_edit[CurrBox].CursorY, str, 0);

             ip_edit[CurrBox].BoxLen ++;
             if(ip_edit[CurrBox].Text[ip_edit[CurrBox].BoxLen] == '.')
               ip_edit[CurrBox].BoxLen ++;
            }
           break;
    case 11: //确定   K
          input_ok = 1;
          if(input_ok == 1)
            if(ip_edit[0].BoxLen < (ip_edit[0].MaxLen - 2))
             {
              PlayModiFailWav();
              input_ok = 0;
              break;
             }

          if(input_ok == 1)
           {
            memcpy(TmpText[0], ip_edit[0].Text, 3);
            TmpText[0][3] = '\0';
            memcpy(TmpText[1], ip_edit[0].Text + 4, 3);
            TmpText[1][3] = '\0';
            memcpy(TmpText[2], ip_edit[0].Text + 8, 3);
            TmpText[2][3] = '\0';
            memcpy(TmpText[3], ip_edit[0].Text + 12, 3);
            TmpText[3][3] = '\0';

            for(i=0; i<4; i++)
              LocalCfg.IP[i] = atoi(TmpText[i]);

            //广播地址
            for(i=0; i<4; i++)
             {
              if(LocalCfg.IP_Mask[i] != 0)
                LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
              else
                LocalCfg.IP_Broadcast[i] = 0xFF;
             }
            printf("eth0 %d.%d.%d.%d \0",
                             LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3]);
            SaveToFlash(4);    //向Flash中存储文件

            RefreshNetSetup(1); //刷新网络设置

            PlayModiSuccWav();

            //发送免费ARP
            SendFreeArp();
            DisplaySetupMainWindow();
           }

          break;
    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
  if(Local.CurrentWindow == 72)
    //显示光标
    WriteCursor(&ip_edit[CurrBox], 1, 1, 0);
}
//---------------------------------------------------------------------------
void DisplayIPMaskWindow(void)    //显示子网掩码窗口（三级）
{
  int i;
  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 73)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 73;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setup_modify_image, 0);
  DisplayImage(&ipmask_image[0], 0);

  ipmask_edit[0].Text[0] = '\0';
  ipmask_edit[0].BoxLen = 0;         //文本框当前输入长度
  ipmask_edit[0].MaxLen = 15;         //文本框最大输入长度
  ipmask_edit[0].Focus = 0;        //显示输入光标
  ipmask_edit[0].Visible = 0;

  //子网掩码
  DisplayEdit(&ipmask_edit[0], 0);
  strcpy(ipmask_edit[0].Text, "   .   .   .   \0");
  outxy_pic_num1(ipmask_edit[0].xLeft + ipmask_edit[0].CursorX + ipmask_edit[0].BoxLen*num_image[0].width,
                  ipmask_edit[0].yTop + ipmask_edit[0].CursorY, ipmask_edit[0].Text, 0);

  ipmask_edit[0].BoxLen = 0;

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateIPMaskWindow(short wintype, int currwindow)  //修改子网掩码操作（三级）
{
  int i,j;
  char str[10];
  int hex_data[6];
  char TmpText[5][5];
  int input_ok;

  //停止光标,并清一下
  WriteCursor(&ipmask_edit[CurrBox], 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(ipmask_edit[CurrBox].BoxLen < ipmask_edit[CurrBox].MaxLen)
            {
             str[0] = '0' + wintype;
             str[1] = '\0';
             memcpy(ipmask_edit[CurrBox].Text + ipmask_edit[CurrBox].BoxLen, str, 1);
             outxy_pic_num1(ipmask_edit[CurrBox].xLeft + ipmask_edit[CurrBox].CursorX + ipmask_edit[CurrBox].BoxLen*num_image[0].width,
                             ipmask_edit[CurrBox].yTop + ipmask_edit[CurrBox].CursorY, str, 0);

             ipmask_edit[CurrBox].BoxLen ++;
             if(ipmask_edit[CurrBox].Text[ipmask_edit[CurrBox].BoxLen] == '.')
               ipmask_edit[CurrBox].BoxLen ++;
            }
           break;
    case 11: //确定   K
          input_ok = 1;

          if(input_ok == 1)
           if(ipmask_edit[CurrBox].BoxLen < (ipmask_edit[CurrBox].MaxLen - 2))
             {
              PlayModiFailWav();
              input_ok = 0;
              break;
             }

          if(input_ok == 1)
           {
            memcpy(TmpText[0], ipmask_edit[0].Text, 3);
            TmpText[0][3] = '\0';
            memcpy(TmpText[1], ipmask_edit[0].Text + 4, 3);
            TmpText[1][3] = '\0';
            memcpy(TmpText[2], ipmask_edit[0].Text + 8, 3);
            TmpText[2][3] = '\0';
            memcpy(TmpText[3], ipmask_edit[0].Text + 12, 3);
            TmpText[3][3] = '\0';

            for(i=0; i<4; i++)
              LocalCfg.IP_Mask[i] = atoi(TmpText[i]);

            //广播地址
            for(i=0; i<4; i++)
             {
              if(LocalCfg.IP_Mask[i] != 0)
                LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
              else
                LocalCfg.IP_Broadcast[i] = 0xFF;
             }
            SaveToFlash(4);    //向Flash中存储文件

            RefreshNetSetup(1); //刷新网络设置

            PlayModiSuccWav();

            //发送免费ARP
            SendFreeArp();
            DisplaySetupMainWindow();
           }
          break;
    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
  if(Local.CurrentWindow == 73)
    //显示光标
    WriteCursor(&ip_edit[CurrBox], 1, 1, 0);
}
//---------------------------------------------------------------------------
void DisplayIPGateWindow(void)    //显示网关地址窗口（三级）
{
  int i;
  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 74)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 74;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setup_modify_image, 0);
  DisplayImage(&ipgate_image[0], 0);

  ipgate_edit[0].Text[0] = '\0';
  ipgate_edit[0].BoxLen = 0;         //文本框当前输入长度
  ipgate_edit[0].MaxLen = 15;         //文本框最大输入长度
  ipgate_edit[0].Focus = 0;        //显示输入光标
  ipgate_edit[0].Visible = 0;

  //网关地址
  DisplayEdit(&ipgate_edit[0], 0);
  strcpy(ipgate_edit[0].Text, "   .   .   .   \0");
  outxy_pic_num1(ipgate_edit[0].xLeft + ipgate_edit[0].CursorX + ipgate_edit[0].BoxLen*num_image[0].width,
                  ipgate_edit[0].yTop + ipgate_edit[0].CursorY, ipgate_edit[0].Text, 0);

  ipgate_edit[0].BoxLen = 0;

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateIPGateWindow(short wintype, int currwindow)  //修改网关地址操作（三级）
{
  int i,j;
  char str[10];
  int hex_data[6];
  char TmpText[5][5];
  int input_ok;

  //停止光标,并清一下
  WriteCursor(&ipgate_edit[CurrBox], 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(ipgate_edit[CurrBox].BoxLen < ipgate_edit[CurrBox].MaxLen)
            {
             str[0] = '0' + wintype;
             str[1] = '\0';
             memcpy(ipgate_edit[CurrBox].Text + ipgate_edit[CurrBox].BoxLen, str, 1);
             outxy_pic_num1(ipgate_edit[CurrBox].xLeft + ipgate_edit[CurrBox].CursorX + ipgate_edit[CurrBox].BoxLen*num_image[0].width,
                             ipgate_edit[CurrBox].yTop + ipgate_edit[CurrBox].CursorY, str, 0);

             ipgate_edit[CurrBox].BoxLen ++;
             if(ipgate_edit[CurrBox].Text[ipgate_edit[CurrBox].BoxLen] == '.')
               ipgate_edit[CurrBox].BoxLen ++;
            }
           break;
    case 11: //确定   K
          input_ok = 1;

          if(input_ok == 1)
           if(ipgate_edit[CurrBox].BoxLen < (ipgate_edit[CurrBox].MaxLen - 2))
             {
              PlayModiFailWav();
              input_ok = 0;
              break;
             }

          if(input_ok == 1)
           {
            memcpy(TmpText[0], ipgate_edit[0].Text, 3);
            TmpText[0][3] = '\0';
            memcpy(TmpText[1], ipgate_edit[0].Text + 4, 3);
            TmpText[1][3] = '\0';
            memcpy(TmpText[2], ipgate_edit[0].Text + 8, 3);
            TmpText[2][3] = '\0';
            memcpy(TmpText[3], ipgate_edit[0].Text + 12, 3);
            TmpText[3][3] = '\0';

            for(i=0; i<4; i++)
              LocalCfg.IP_Gate[i] = atoi(TmpText[i]);

            //广播地址
            for(i=0; i<4; i++)
             {
              if(LocalCfg.IP_Mask[i] != 0)
                LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
              else
                LocalCfg.IP_Broadcast[i] = 0xFF;
             }
            SaveToFlash(4);    //向Flash中存储文件

            RefreshNetSetup(1); //刷新网络设置

            PlayModiSuccWav();

            //发送免费ARP
            SendFreeArp();
            DisplaySetupMainWindow();
           }
          break;
    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
  if(Local.CurrentWindow == 74)
    //显示光标
    WriteCursor(&ipgate_edit[CurrBox], 1, 1, 0);
}
//---------------------------------------------------------------------------
void DisplayIPServerWindow(void)    //显示服务器地址窗口（三级）
{
  int i;
  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 75)
    return;
  Local.isDisplayWindowFlag = 1;

  Local.PrevWindow = Local.CurrentWindow;
  Local.CurrentWindow = 75;
  CheckMainFbPage(); //检查是否在framebuffer 的第0页

  DisplayImage(&setup_modify_image, 0);
  DisplayImage(&ipserver_image[0], 0);

  ipserver_edit[0].Text[0] = '\0';
  ipserver_edit[0].BoxLen = 0;         //文本框当前输入长度
  ipserver_edit[0].MaxLen = 15;         //文本框最大输入长度
  ipserver_edit[0].Focus = 0;        //显示输入光标
  ipserver_edit[0].Visible = 0;

  //服务器地址
  DisplayEdit(&ipserver_edit[0], 0);
  strcpy(ipserver_edit[0].Text, "   .   .   .   \0");
  outxy_pic_num1(ipserver_edit[0].xLeft + ipserver_edit[0].CursorX + ipserver_edit[0].BoxLen*num_image[0].width,
                  ipserver_edit[0].yTop + ipserver_edit[0].CursorY, ipserver_edit[0].Text, 0);

  ipserver_edit[0].BoxLen = 0;

  Local.isDisplayWindowFlag = 0; //窗口正在显示中
}
//---------------------------------------------------------------------------
void OperateIPServerWindow(short wintype, int currwindow)  //修改服务器地址操作（三级）
{
  int i,j;
  char str[10];
  int hex_data[6];
  char TmpText[5][5];
  int input_ok;

  //停止光标,并清一下
  WriteCursor(&ipserver_edit[CurrBox], 0, 0, 0);
  switch(wintype)
   {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
           if(ipserver_edit[CurrBox].BoxLen < ipserver_edit[CurrBox].MaxLen)
            {
             str[0] = '0' + wintype;
             str[1] = '\0';
             memcpy(ipserver_edit[CurrBox].Text + ipserver_edit[CurrBox].BoxLen, str, 1);
             outxy_pic_num1(ipserver_edit[CurrBox].xLeft + ipserver_edit[CurrBox].CursorX + ipserver_edit[CurrBox].BoxLen*num_image[0].width,
                             ipserver_edit[CurrBox].yTop + ipserver_edit[CurrBox].CursorY, str, 0);

             ipserver_edit[CurrBox].BoxLen ++;
             if(ipserver_edit[CurrBox].Text[ipserver_edit[CurrBox].BoxLen] == '.')
               ipserver_edit[CurrBox].BoxLen ++;
            }
           break;
    case 11: //确定   K
          input_ok = 1;

          if(input_ok == 1)
           if(ipserver_edit[CurrBox].BoxLen < (ipserver_edit[CurrBox].MaxLen - 2))
             {
              PlayModiFailWav();
              input_ok = 0;
              break;
             }

          if(input_ok == 1)
           {
            memcpy(TmpText[0], ipserver_edit[0].Text, 3);
            TmpText[0][3] = '\0';
            memcpy(TmpText[1], ipserver_edit[0].Text + 4, 3);
            TmpText[1][3] = '\0';
            memcpy(TmpText[2], ipserver_edit[0].Text + 8, 3);
            TmpText[2][3] = '\0';
            memcpy(TmpText[3], ipserver_edit[0].Text + 12, 3);
            TmpText[3][3] = '\0';

            for(i=0; i<4; i++)
              LocalCfg.IP_Server[i] = atoi(TmpText[i]);

            SaveToFlash(4);    //向Flash中存储文件

            PlayModiSuccWav();

            DisplaySetupMainWindow();
           }
          break;
    case 10:  //返回
           DisplaySetupMainWindow();
           break;
   }
  if(Local.CurrentWindow == 75)
    //显示光标
    WriteCursor(&ipgate_edit[CurrBox], 1, 1, 0);
}
//---------------------------------------------------------------------------
void SaveToFlash(int savetype)    //向Flash中存储文件
{
  int i;
  //写入文件
  //锁定互斥锁
  pthread_mutex_lock (&Local.save_lock);
  printf("to save thread\n");
  //查找可用存储缓冲并填空
  for(i=0; i<SAVEMAX; i++)
   if(Save_File_Buff[i].isValid == 0)
    {
     Save_File_Buff[i].Type = savetype;      //存储本地设置
     Save_File_Buff[i].isValid = 1;
     sem_post(&save_file_sem);     
     break;
    }
  //打开互斥锁
  pthread_mutex_unlock (&Local.save_lock);

}
//---------------------------------------------------------------------------
void DisplayCalibrateWindow(void)    //校准触摸屏窗口（二级）
{
  int i,j;
  char str[10];
  char jpgfilename[80];
  while(Local.isDisplayWindowFlag == 1) //窗口正在显示中
    usleep(10);
  if(Local.CurrentWindow == 190)
    return;
  Local.isDisplayWindowFlag = 1;

  memset(fbmem + f_data.buf_len*3, 235, f_data.uv_offset);
  memset(fbmem + f_data.buf_len*3 + f_data.uv_offset, 128, f_data.uv_offset/4);
  memset(fbmem + f_data.buf_len*3 + f_data.uv_offset*5/4, 128, f_data.uv_offset/4);

  outxy24(200, 200, 3, cBlack, 1, 1, "请点击十字交叉点", 0, 3);
  Local.CalibratePos = 0;
  if((fb_width == 800)&&(fb_height == 600))
   {
    CrossPosY[2] = 550;
    CrossPosY[3] = 550;
   }

  WriteCross(CrossPosX[Local.CalibratePos], CrossPosY[Local.CalibratePos], 1, 3);
  if(Local.CurrentWindow != 191)
    Local.TmpWindow = Local.CurrentWindow;
  Local.CurrentWindow = 190;
//  Local.CalibrateNum ++;  //校准次数
  //设置FB页面
  if(Local.CurrFbPage != 3)
   {
    Local.CurrFbPage = 3;
    fb_setpage(fbdev, Local.CurrFbPage);
   }

  Local.isDisplayWindowFlag = 0; //窗口正在显示中   
}
//---------------------------------------------------------------------------
void OperateCalibrateWindow(short wintype, int currwindow)  //校准触摸屏操作（二级）
{
  int i;
  int X0,Y0,XA,YA,XB,YB,XC,YC,XD,YD,deltaX,deltaY;
  int Stand_X0, Stand_Y0, Stand_deltaX, Stand_deltaY;
  int RefCalib_X[4], RefCalib_Y[4];
  Stand_X0 = 491;
  Stand_Y0 = 499;
  Stand_deltaX = -830;
  Stand_deltaY = 683;
/*  RefCalib_X[0] = (0x00*256+0x16);
  RefCalib_Y[0] = (0x00*256+0x53);
  RefCalib_X[1] = (0x0E*256+0x61);
  RefCalib_Y[1] = (0x01*256+0x6C);
  RefCalib_X[2] = (0x00*256+0x25);
  RefCalib_Y[2] = (0x0F*256+0x15);
  RefCalib_X[3] = (0x0F*256+0x1A);
  RefCalib_Y[3] = (0x0E*256+0x70); */

  WriteCross(CrossPosX[Local.CalibratePos], CrossPosY[Local.CalibratePos], 0, 3);
  Local.CalibratePos ++;
  if(Local.CalibratePos > 3)
   {
    XA = Calib_X[0];
    XB = Calib_X[1];
    XC = Calib_X[2];
    XD = Calib_X[3];
    YA = Calib_Y[0];
    YB = Calib_Y[1];
    YC = Calib_Y[2];
    YD = Calib_Y[3];
    X0=(XA + XB + XC +XD)/4;
    Y0=(YA + YB + YC +YD)/4;
    deltaX = (XB - XA + XD - XC)/2;
 //   deltaX = (XA - XC + XB - XD)/2;
    deltaX = deltaX *fb_width/(fb_width - 100);    //由于校准点不在原点
 //   deltaY = (YB - YA + YD - YC)/2;
    deltaY = (YC - YA + YD - YB)/2;
    deltaY = deltaY *fb_height/(fb_height - 100);
    printf("X0 = %d, Y0 = %d, deltaX = %d, deltaY = %d, 1 = %f,  2 = %f, delta1 = %f, delta2 = %f,\n",  \
           X0, Y0, deltaX, deltaY, fabs((X0 - Stand_X0)*1.0/(Stand_X0*1.0)),  fabs((Y0 - Stand_Y0)*1.0/(Stand_Y0*1.0)), \
           fabs((deltaX - Stand_deltaX)*1.0/(Stand_deltaX*1.0)),  fabs((deltaY - Stand_deltaY)*1.0/(Stand_deltaY*1.0)));
    //如与标准值偏差太大
/*    if((fabs((X0 - Stand_X0)*1.0/(Stand_X0*1.0))<0.2)
       &&(fabs((Y0 - Stand_Y0)*1.0/(Stand_Y0*1.0))<0.2)
       &&(fabs((deltaX - Stand_deltaX)*1.0/(Stand_deltaX*1.0))<0.2)
       &&(fabs((deltaY - Stand_deltaY)*1.0/(Stand_deltaY*1.0))<0.2)) */
       {                                         
        LocalCfg.Ts_X0 = X0;
        LocalCfg.Ts_Y0 = Y0;
        LocalCfg.Ts_deltaX = deltaX;
        LocalCfg.Ts_deltaY = deltaY;

        SaveToFlash(4);    //向Flash中存储文件

        Local.CalibrateSucc = 1; 
             //设置FB页面
             if(Local.CurrFbPage != 0)
              {
               Local.CurrFbPage = 0;
               fb_setpage(fbdev, Local.CurrFbPage);
              }
             Local.CurrentWindow = Local.TmpWindow;
     //        Local.CalibrateNum = 0;
       }
  /*    else
       {
        Local.CalibrateSucc = 0; 
             //设置FB页面
             if(Local.CurrFbPage != 0)
              {
               Local.CurrFbPage = 0;
               fb_setpage(fbdev, Local.CurrFbPage);
              }
             Local.CurrentWindow = Local.TmpWindow;
     //        Local.CalibrateNum = 0;
       }       */

   }
  else
    WriteCross(CrossPosX[Local.CalibratePos], CrossPosY[Local.CalibratePos], 1, 3);
}
//---------------------------------------------------------------------------
//画十字函数   t_Flag 0--清除  1--写
void WriteCross(int xLeft, int yTop, int t_Flag, int PageNo)
{
 if(t_Flag == 1)
  {
   fb_line(xLeft - 20, yTop, xLeft + 20, yTop, cBlack, PageNo);
   fb_line(xLeft - 20, yTop + 1, xLeft + 20, yTop + 1, cBlack, PageNo);
   fb_line(xLeft, yTop - 20, xLeft, yTop + 20, cBlack, PageNo);
   fb_line(xLeft + 1, yTop - 20, xLeft + 1, yTop + 20, cBlack, PageNo);
  }
  else
  {
   fb_line(xLeft - 20, yTop, xLeft + 20, yTop, cWhite, PageNo);
   fb_line(xLeft - 20, yTop + 1, xLeft + 20, yTop + 1, cWhite, PageNo);
   fb_line(xLeft, yTop - 20, xLeft, yTop + 20, cWhite, PageNo);
   fb_line(xLeft + 1, yTop - 20, xLeft + 1, yTop + 20, cWhite, PageNo);
  }
}
//---------------------------------------------------------------------------


