

#include <inttypes.h>
#include <signal.h>
#include <semaphore.h>       //sem_t
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "sndtools.h"
#include "image.h"

//#define _REMOTECALLTEST  //远程呼叫测试

//#define cfg_name "./door26cfg"
#define cfg_name "/data/data/com.fsti.ladder/door26cfg"
#define info_name "/mnt/mtd/config/info"
#define wavini_name "/mnt/sddisk/homewav/wavini"
#define movieini_name "/mnt/sddisk/homemovie/movieini"
#define mtdexe_name "/mnt/mtd/door26"
#define mtdimage_name "/mdoor26pImage"

#define FLAGTEXT "hikdsdkkkkdfdsIMAGE"

//#define _MULTI_MACHINE_SUPPORT  //多分机支持

//#define _DEBUG           //调试模式

//#define _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
//#define _BRUSHIDCARD_SUPPORT   //刷卡功能支持

//测试服务器解析模式
//测试视频中转模式
//#define _TESTTRANSMODE

//#define _MESSAGE_SUPPORT  //消息机制

#define HARDWAREVER "M-HW VER 3.1"    //硬件版本
#define SOFTWAREVER "M-SW VER 3.2"    //软件版本
#define SERIALNUM "20110917"    //产品序列号
#define WAVFILEMAX 10

#define ZOOMMAXTIME 2000   //放大缩小延迟处理时间
#define TOUCHMAXTIME 300   //触摸屏处理延迟处理时间

#define INTRTIME 50       //线程50ms
#define INTRPERSEC 20       //每秒20次线程

#define NSMULTIADDR  "238.9.9.1"  //NS组播地址
#define MULTITTL   5      //多播TTL值

#define COMMMAX 1024     //串口缓冲区最大值
#define SCRWIDTH  800
#define SCRHEIGHT  480
#define D1_W  720
#define D1_H  480
#define CIF_W 352
#define CIF_H 240
#define REFRESH  1
#define NOREFRESH 0
#define SHOW  0
#define HIDE  1
#define UMOUNT  0
#define MOUNT   1
#define HILIGHT  2
#define IMAGEUP  0
#define IMAGEDOWN  1

#define DIRECTCALLTIME  600                 //直接呼叫每次时间
#define TRANSCALLTIME  800                  //中转呼叫每次时间
#define WATCHTIMEOUT  60*INTRPERSEC//30*20    //监视最长时间
#define CALLTIMEOUT  30*INTRPERSEC     //呼叫最长时间
//#define CALLTIMEOUT  300*INTRPERSEC     //呼叫最长时间
#define TALKTIMEOUT  240*INTRPERSEC//30*20     //通话最长时间
#define PREPARETIMEOUT  10*INTRPERSEC     //留影留言预备最长时间
#define RECORDTIMEOUT  30*INTRPERSEC     //留影留言最长时间


//命令 管理中心
#define ALARM  1
#define SENDMESSAGE   3   //发送信息
#define REPORTSTATUS  4   //设备定时报告状态
#define QUERYSTATUS   5   //管理中心查询设备状态
#define SENDCOMMSRV   7   //发送物业服务
#define SEARCHNOTLOGIN   9   //搜索未登录服务器的设备
#define WRITEADDRESS   40   //写地址设置
#define READADDRESS    41   //读地址设置
#define WRITECOMMMENU   42   //写物业服务菜单
#define READCOMMMENU   43   //读物业服务菜单
#define WRITEHELP   50   //写帮助信息
#define READHELP    51   //读帮助信息
#define WRITESETUP     52   //写设置信息
#define READSETUP      53   //读设置信息

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
 #define _IDCARDHAVEADDR      //ID卡带地址
 #define IDCARDMAXNUM  100000  //ID卡最大数量
 #ifdef _IDCARDHAVEADDR      //ID卡带地址
  #define CARDPERPACK  50    //每个数据包内ID卡最大数量
  #define BYTEPERSN    24    //每个SN 占用字节数
 #else
  #define CARDPERPACK  350  //每个数据包内ID卡最大数量
  #define BYTEPERSN    4     //每个SN 占用字节数
 #endif
 #define IDCARDBRUSHNUM 10000 //最多离线刷卡条数
 
 #define WRITEIDCARD    54   //写ID卡信息
 #define READIDCARD     55   //读ID卡信息
 #define BRUSHIDCARD     56   //刷ID卡信息，向服务器发送
#endif

#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
 #define tmp_pic_file  "/tmp/capture_pic.jpg"
 #define MAX_CAPTUREPIC_NUM   20
 #define CAPTUREPIC_SEND_START     60   //单元门口机、围墙机、二次门口机发送呼叫照片->发送开始
 #define CAPTUREPIC_SEND_DATA      61   //单元门口机、围墙机、二次门口机发送呼叫照片->发送数据
 #define CAPTUREPIC_SEND_SUCC      62   //单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（成功）
 #define CAPTUREPIC_SEND_FAIL      63   //单元门口机、围墙机、二次门口机发送呼叫照片->发送结束（失败）
#endif

//对讲
#define VIDEOTALK      150 //局域网可视对讲
#define VIDEOTALKTRANS 151 //局域网可视对讲中转服务
#define VIDEOWATCH     152 //局域网监控
#define VIDEOWATCHTRANS   153 //局域网监控中转服务
#define NSORDER        154 //主机名解析（子网内广播）
#define NSSERVERORDER  155 //主机名解析(NS服务器)


#define ASKALLOCSERVER  160 //请求分配视频服务器(向管理中心)
#define BEGINTRANS      161 //开始视频中转(向视频服务器)
#define ENDTRANS        162 //结束视频中转(向视频服务器)

#define DOWNLOADFILE  224    //下载应用程序
#define DOWNLOADIMAGE  225    //下载系统映像
#define STARTDOWN  1       //开始下载
#define DOWN       2       //下载
#define DOWNFINISHONE       3  //下载完成一个
#define STOPDOWN       10      //停止下载
#define DOWNFINISHALL       20 //全部完成下载
#define DOWNFAIL         21 //下载失败  设备－》管理中心

#define REMOTEDEBUGINFO      253   //发送远程调试信息

#ifdef _REMOTECALLTEST  //远程呼叫测试
 #define REMOTETEST      200   //发送远程呼叫测试
 #define STARTTEST  1    //开始
#endif

#define ERASEFLASH  31    //正在删除Flash
#define WRITEFLASH  32    //正在写Flash
#define CHECKFLASH  33    //正在校验Flash
#define ENDFLASH  34      //完成写Image
#define ERRORFLASH  35      //操作Image失败

#define ASK              1     //命令类型 主叫
#define REPLY            2     //命令类型 应答

#define CALL             1     //呼叫
#define LINEUSE          2     //占线
#define QUERYFAIL        3      //通信失败
#define CALLANSWER       4     //呼叫应答
#define CALLSTART        6     //开始通话

#define CALLUP           7     //通话数据1（主叫方->被叫方）
#define CALLDOWN         8     //通话数据2（被叫方->主叫方）
#define CALLCONFIRM      9     //通话在线确认（接收方发送，以便发送方确认在线）
#define REMOTEOPENLOCK   10     //远程开锁
#define FORCEIFRAME      11     //强制I帧请求
#define ZOOMOUT          15     //放大(720*480)
#define ZOOMIN           16     //缩小(352*288)

#define PREPARERECORD    20     //开始留影预备（被叫方->主叫方，主叫方应答
#define RECORD           21     //开始留影（主叫方->被叫方，被叫方应答）
#define JOINGROUP        22     //加入组播组（主叫方->被叫方，被叫方应答）
#define LEAVEGROUP       23     //退出组播组（主叫方->被叫方，被叫方应答）
#define TURNTALK         24     //转接（被叫方->主叫方，主叫方应答）

#define CALLEND          30     //通话结束

#define SAVEMAX  10     //FLASH存储缓冲最大值
#define UDPSENDMAX  50  //UDP多次发送缓冲最大值
#define COMMSENDMAX  20  //COMM多次发送缓冲最大值
#define DISPARTMAX  10   //拆分包发送缓冲
#define MAXCOMMSENDNUM  3
#define MAXSENDNUM  6  //最大发送次数

#define SEARCHALLEQUIP  252  //搜索所有设备（管理中心－＞设备）
#define WRITEEQUIPADDR      254  //写设备地址（管理中心－＞设备）

#define TALK_IO            15   //呼叫键
#define CENTER_IO            14   //呼叫中心键
#define OPENLOCK_IO          92   //开锁     GPIO2 28

#define WATCHDOG_IO_EN       93       //Watchdog 置0 启用    置1禁用   GPIO2 29
#define WATCHDOG_IO_CLEAR    9        //定时器反转，清WatchDog         GPIO0  9

#define LCD_LIGHT_IO               2        //PWM GPIO0  2
#define PWM_IO               3        //PWM GPIO0  3

#define _M8_WATCHDOG_START  0x82
#define _M8_WATCHDOG_END  0x83

//按钮压下时间
#define DELAYTIME  300


#define USER_DEV_NAME  "/dev/user_apply"

#define _INIT_GPIO  0
#define _SET_GPIO_OUTPUT_HIGH  1
#define _SET_GPIO_OUTPUT_LOW   2
#define _SET_GPIO_HIGH  3
#define _SET_GPIO_LOW   4
#define _SET_GPIO_INPUT  5
#define _GET_GPIO_VALUE  6


//视频常量
#define cWhite  1
#define cYellow 2
#define cCyan   3
#define cGreen  4
#define cMagenta  5
#define cRed      6
#define cBlue     7
#define cBlack    8
#define	FB_DEV	"/dev/fb0"
#define MAXPIXELS (1280*1024)  /* Maximum size of final image */
#define VIDEO_PICTURE_QUEUE_SIZE_MAX 20

#define CONFLICTARP  0x8950
#define FLCD_GET_DATA_SEP   0x46db
#define FLCD_GET_DATA       0x46dc
#define FLCD_SET_FB_NUM     0x46dd
#define FLCD_SWITCH_MODE    0x46de
#define FLCD_CLOSE_PANEL    0x46df
#define FLCD_BYPASS    0x46e0
#define FLCD_OPEN	0x46fa
#define FLCD_CLOSE	0x46fb


//20071210 检查SD卡插入和写保护状态
#define DEVICE_SD                "/dev/cpesda"
#define CHECK_SD_STATUS  0x2222
//20080401 设置PMU，关闭不用的时钟
#define CLOSE_PMU1  0x2255
#define CLOSE_PMU2  0x2256

#define NMAX (512*48*2)  //音频环形缓冲区大小
#define G711NUM  (48*512*2)/AUDIOBLK         //音频接收缓冲区个数 未解码   10

#define VIDEOMAX 720*576
#define VNUM  3         //视频采集缓冲区大小
#define VPLAYNUM  10         //视频播放缓冲区大小         6
#define MP4VNUM  10         //视频接收缓冲区个数 未解码   10
#define PACKDATALEN  1200   //数据包大小
#define MAXPACKNUM  100     //帧最大数据包数量

#define MAXRECWAV  30   //最长留言时间
struct TimeStamp1{
    unsigned int OldCurrVideo;     //上一次当前视频时间
    unsigned int CurrVideo;
    unsigned int OldCurrAudio;     //上一次当前音频时间
    unsigned int CurrAudio;
   };
//视频采集缓冲
struct videobuf1
 {
  int iput; // 环形缓冲区的当前放入位置
  int iget; // 缓冲区的当前取出位置
  int n; // 环形缓冲区中的元素总数量
  uint32_t timestamp[VNUM]; //时间戳
  uint32_t frameno[VNUM];   //帧序号
  unsigned char *buffer_y[VNUM];//[VIDEOMAX];
  unsigned char *buffer_u[VNUM];//[VIDEOMAX/4];
  unsigned char *buffer_v[VNUM];//[VIDEOMAX/4];
 };
//视频接收缓冲  未解码
struct tempvideobuf1
 {
//  int iput;                     // 环形缓冲区的当前放入位置
//  int iget;                     // 缓冲区的当前取出位置
//  int n;                        // 环形缓冲区中的元素总数量
  uint32_t timestamp;  //时间戳
  uint32_t frameno;       //帧序号
  short TotalPackage;     //总包数
  uint8_t CurrPackage[MAXPACKNUM]; //当前包   1 已接收  0 未接收
  int Len;                //帧数据长度
  uint8_t isFull;                  //该帧已接收完全
  unsigned char buffer[VIDEOMAX];
  unsigned char frame_flag;             //帧标志 音频帧 I帧 P帧
 };                            //     [MP4VNUM]
//视频播放缓冲
struct videoplaybuf1
 {
  uint8_t isUse;     //该帧已解码未播放,缓冲区不可用
  uint32_t timestamp; //时间戳
  uint32_t frameno;   //帧序号
  unsigned char *buffer;//[VIDEOMAX];
  unsigned char frame_flag;             //帧标志 音频帧 I帧 P帧
 };
//同步播放结构
struct _SYNC
 {
  pthread_cond_t cond;       //同步线程条件变量
  pthread_condattr_t cond_attr;
  pthread_mutex_t lock;      //互斥锁
  pthread_mutex_t audio_rec_lock;//[VPLAYNUM];//音频录制互斥锁
  pthread_mutex_t audio_play_lock;//[VPLAYNUM];//音频播放互斥锁
  pthread_mutex_t video_rec_lock;//[VPLAYNUM];//视频录制互斥锁
  pthread_mutex_t video_play_lock;//[VPLAYNUM];//视频播放互斥锁
  pthread_mutex_t audio_lock;
  unsigned int count;        //计数
  uint8_t isDecodeVideo;     //视频已解码一帧  解码线程-->同步线程
  uint8_t isPlayVideo;       //视频已播放一帧  播放线程-->同步线程
  uint8_t isDecodeAudio;     //音频已解码一帧  解码线程-->同步线程
  uint8_t isPlayAudio;       //音频已播放一帧  播放线程-->同步线程
 };

//加缓冲锁?
struct audiobuf1
 {
  int iput; // 环形缓冲区的当前放入位置
  int iget; // 缓冲区的当前取出位置
  int i_echo;  //回声消除位置
  int n; // 环形缓冲区中的元素总数量
  uint32_t timestamp[NMAX/AUDIOBLK]; //时间戳
  uint32_t frameno[NMAX/AUDIOBLK];   //帧序号
  unsigned char buffer[NMAX];
  #ifdef _ECHO_STATE_SUPPORT  //回声消除支持
   uint32_t echo_timestamp[NMAX/AUDIOBLK]; //时间戳
  #endif
 };
struct WavFileBuf1 {
     int isValid;
     char wname[80];
     int PlayFlag;     //0 单次播放  1 循环播放
     };
/*struct tempbuf1
 {
  int iput; // 环形缓冲区的当前放入位置
  int iget; // 缓冲区的当前取出位置
  int n; // 环形缓冲区中的元素总数量
  uint32_t timestamp[NMAX/AUDIOBLK]; //时间戳
  uint32_t frameno[NMAX/AUDIOBLK];   //帧序号
  unsigned char buffer[NMAX];
 }; */
//音频接收缓冲  未解码
struct tempaudiobuf1
 {
  uint32_t timestamp;  //时间戳
  uint32_t frameno;       //帧序号
  short TotalPackage;     //总包数
  uint8_t CurrPackage[MAXPACKNUM]; //当前包   1 已接收  0 未接收
  int Len;                //帧数据长度
  uint8_t isFull;                  //该帧已接收完全
  unsigned char *buffer;//[AUDIOBLK];
  unsigned char frame_flag;             //帧标志 音频帧 I帧 P帧
 };                            //     [MP4VNUM]
//音频接收缓冲 链表
typedef struct node3{
               struct tempaudiobuf1 Content;
               struct node3 *llink, *rlink;
}TempAudioNode1; 

//音频播放缓冲
struct audioplaybuf1
 {
  uint8_t isUse;     //该帧已解码未播放,缓冲区不可用
  uint32_t timestamp; //时间戳
  uint32_t frameno;   //帧序号
  unsigned char *buffer;//[VIDEOMAX];
 };

//家庭留言缓冲
struct wavbuf1
 {
  int iput; // 环形缓冲区的当前放入位置
  int iget; // 缓冲区的当前取出位置
  int n; // 环形缓冲区中的元素总数量
  unsigned char *buffer;
 };

struct WaveFileHeader
{
 char chRIFF[4];
 uint32_t dwRIFFLen;
 char chWAVE[4];

 char chFMT[4];
 uint32_t dwFMTLen;
 uint16_t wFormatTag;
 uint16_t nChannels;
 uint32_t nSamplesPerSec;
 uint32_t nAvgBytesPerSec;
 uint16_t nBlockAlign;
 uint16_t wBitsPerSample;

 char chFACT[4];
 uint32_t dwFACTLen;

 char chDATA[4];
 uint32_t dwDATALen;
};


typedef struct _parameters {
//  char *jpegformatstr;
  char jpegformatstr[10];
  uint32_t begin;       /* the video frame start */
  int32_t numframes;   /* -1 means: take all frames */
//  y4m_ratio_t framerate;
//  y4m_ratio_t aspect_ratio;
  int interlace;   /* will the YUV4MPEG stream be interlaced? */
  int interleave;  /* are the JPEG frames field-interleaved? */
  int verbose; /* the verbosity of the program (see mjpeg_logging.h) */

  int width;
  int height;
  int colorspace;
  int loop;
  int rescale_YUV;
} parameters_t;

/*typedef */struct fcap_frame_buff
{
    unsigned int phyAddr;
    unsigned int mmapAddr;   //length per dma buffer
    unsigned int frame_no;
};

struct Local1{
               int _Door_Disp_Type;     //显示类型  1 -- 800x480   2 -- 320x240
               int Status;
               int NSReplyFlag;  //NS应答处理标志
               //状态 0 空闲 1 主叫对讲  2 被叫对讲  3 监视  4 被监视  5 主叫通话
               //6 被叫通话  7 主叫留影留言预备  8 被叫留影留言预备 9 主叫留影留言
               //10 被叫留影留言   11 播放留影   30 IP 监控

               int LoginServer;     //是否登录服务器，<<从未登录到登录是即时，从登录到未登录则判断3次>>
                              
               int CallConfirmFlag; //在线标志
               int Timer1Num;  //定时器1计数
               int OnlineFlag; //需检查在线确认
               int OnlineNum;  //在线确认序号
               int TimeOut;    //监视超时,  通话超时,  呼叫超时，无人接听
               int RecPicSize;  //视频大小  1  352*288   2  720*480
               int PlayPicSize;  //视频大小  1  352*288   2  720*480
               pthread_mutex_t save_lock;//互斥锁
               pthread_mutex_t udp_lock;//互斥锁
               pthread_mutex_t comm_lock;//互斥锁
               #ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
                pthread_mutex_t idcard_lock;//互斥锁
                pthread_mutex_t dispart_send_lock;//互斥锁
               #endif
               int isSDInsert;  //SD卡是否存在
               int isSDProtect; //SD卡写保护
               int isSDMounted;  //SD卡Mount成功
               int SD_Status;  //SD卡安装 0 -- 无  1--正常  2--写保护  3--安全拨出  4--非安全拨出
               int DefenceDelayFlag;    //布防延时标志
               int DefenceDelayTime;   //计数
               int PassLen;            //密码长度
               int SingleFortifyFlag;    //单防区布防延时标志
               int SingleFortifyTime;   //单防区布防计数
               int ForceIFrame;    //1 强制I帧
               int LMovieTime;     //留影文件长度 秒
               int CalibratePos;   //校准触摸屏十字位置 0 1 2 3
               int CalibrateSucc;  //校准成功
               int CurrFbPage; //当前Fb页
               char ClockText[20];  //当前时钟
               int InputMax;       //输入最大长度  围墙机  10位  单元门口机  4位
               int ModiPassInputNum; //修改密码输入次数
               char FirstPass[20];   //第一次输入的密码
               unsigned char IP_Group[4];  //组播地址
               unsigned char Weather[3];   //天气预报

               #ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
                int RecordPic;
                int IFrameCount; //帧计数
                int IFrameNo;    //留第几个I帧
                int Pic_Size;                
                int Pic_Width;
                int Pic_Height;
                unsigned char yuv[D1_W*D1_H];
                int HavePicRecorded;  //有照片已录制
                struct tm *recpic_tm_t; //留照片时间
                char RemoteAddr[21];  //远端地址
                int SendToCetnering;  //正在传给中心
                int ConnectCentered;   //与中心连接状态
               #endif               

               int AddrLen;          //地址长度  S 12  B 6 M 8 H 6

               int isHost;           //'0' 主机 '1' 副机 '2' ...
               int DenNum;             //目标数量  副机
               unsigned char DenIP[10][4];    //副机IP
               char DenAddr[10][21];         //副机Addr

               unsigned char IP_VideoServer[4];  //视频服务器地址

               int NetStatus;   //网络状态 1 断开  0 接通
               int OldNetSpeed;  //网络速度

               int ReportSend;  //设备定时报告状态已发送
               int RandReportTime; //设备定时报告状态随机时间
               int ReportTimeNum;  //计时

               int LcdLightFlag; //LCD背光标志
               int LcdLightTime; //时间

                                 //在GPIO线程中查询各线程是否运行
               int Key_Press_Run_Flag;
               int Save_File_Run_Flag;
               #ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
                int Dispart_Send_Run_Flag;
               #endif 
               int Multi_Send_Run_Flag;
               int Multi_Comm_Send_Run_Flag;

               int PlayLMovieTipFlag; //播放留言提示标志

               int ResetPlayRingFlag;  //复位Audio Play flag

               int nowvideoframeno;   //当前视频帧编号
               int nowaudioframeno;   //当前音频帧编号

               int ForceEndWatch;  //有呼叫时，强制关监视
               int ZoomInOutFlag;  //正在放大缩小中
               uint32_t newzoomtime;
               uint32_t oldzoomtime;
               uint32_t newtouchtime;
               uint32_t oldtouchtime;    //上一次触摸屏处理时间

               int _TESTNSSERVER;        //测试服务器解析模式
               int _TESTTRANS;           //测试视频中转模式

               int SearchAllNo;  //中心机搜索所有设备序号
               int DownProgramOK; //下载应用程序完成
               int download_image_flag; //下载系统映像

               char DebugInfo[1024];
               char DebugIP[20];
               int RemoteDebugInfo;  //发送远程调试信息

               int CaptureVideoStartFlag;  //捕获视频开始标志     20101109  xu
               int CaptureVideoStartTime;  //捕获视频开始计数

               char cLockFlg;								//开锁保护（防止连续开锁）	add by lcx
              };
struct LocalCfg1{
               char Addr[20];             //地址编码
               unsigned char Mac_Addr[6]; //网卡地址
               unsigned char IP[4];       //IP地址
               unsigned char IP_Mask[4];  //子网掩码
               unsigned char IP_Gate[4];  //网关地址
               unsigned char IP_NS[4];    //NS（名称解析）服务器地址
               unsigned char IP_Server[4];  //主服务器地址（与NS服务器可为同一个）
               unsigned char IP_Broadcast[4];  //广播地址

               int ReportTime;      //设备定时报告状态时间

               unsigned char LockType;   //电锁类型  0 －－ 电控锁  1 －－ 电磁锁
               unsigned char OpenLockTime;   //开锁时间  电控锁 0 ――100ms  1――200ms
                                             //          电磁锁 0 ――5s  1――10s

               unsigned char DelayLockTime;   //延时开锁  0 0s  1 3s  2 5s  3 10s
               unsigned char PassOpenLock;   //密码开锁
               unsigned char CardOpenLock;   //刷卡开锁
               unsigned char CardComm;       //刷卡通信
               unsigned char DoorDetect;       //门磁检测
               unsigned char RoomType;        //房号类型  0 数字型  1  字符型
               unsigned char CallWaitRing;        //呼叫回铃  0 普通型  1  卡通型  2  摇滚型   3  抒情型

               char EngineerPass[10];           //安装密码
               char OpenLockPass[10];             //开锁密码
               char HelpInfo[200];

               unsigned char VoiceHint;         //语音提示  0 关闭  1  打开
               unsigned char KeyVoice;          //按键音    0 关闭  1  打开
               
               int Ts_X0;                   //触摸屏
               int Ts_Y0;
               int Ts_deltaX;
               int Ts_deltaY;
              };

#ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
//ID卡信息
struct IDCardNo1{
               int Num;        //数量
               unsigned char SN[IDCARDMAXNUM*BYTEPERSN]; //ID卡号  最大10万张
               uint32_t serialnum;     //序号
              };
//写ID卡信息
struct RecvIDCardNo1{
               int isNewWriteFlag;  //新写标志
               int Num;             //卡数量
               int PackNum;         //包数量
               int PackIsRecved[IDCARDMAXNUM/CARDPERPACK + 1];  //包已接收标志
               unsigned char SN[IDCARDMAXNUM*BYTEPERSN]; //ID卡号  最大10万张
               uint32_t serialnum;     //序号
              };
//刷ID卡信息
struct BrushIDCard1{
               int Num;        //数量
               unsigned char Info[IDCARDBRUSHNUM*11]; //ID卡号  离线信息最多1万条
               //前4字节卡号，后7字节时间
              };
//读写ID卡数据结构
struct iddata1
  {
   #ifdef _IDCARDHAVEADDR      //ID卡带地址
    char Addr[20];       //地址编码
   #endif 
   unsigned int serialnum;  //序号
   unsigned int  Num;          //卡总数
   unsigned int  CurrNum;           //当前数据包卡数量
   unsigned int  TotalPackage;      //总包数
   unsigned int  CurrPackage;       //当前包数
  }__attribute__ ((packed));              
#endif
#ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
 //捕获图片传到中心
 struct Capture_Pic_Center1{
                int isVilid;             //
                char RemoteAddr[21];
                int jpegsize;
                unsigned char *jpeg_pic; //相片JPG数据
                struct tm *recpic_tm_t; //留照片时间
               };
#endif

//LCD屏幕显示
struct LCD_Display1{
                    int isFinished;   //显示完成
                    int ShowFlag[4];   //0  正常  1  反显
                    int PrevShowFlag[4];
                    char PrevText[4][20];  //上一屏
                    char CurrText[4][20];  //当前屏
                    int MaxRow;  //本次显示最大行号
                   };
struct DefenceCfg1{
               unsigned char DefenceStatus;       //布防状态
               unsigned char DefenceNum;          //防区模块个数
               unsigned char DefenceInfo[32][10]; //防区信息
              };
//串口接收缓冲区
struct commbuf1
 {
  int iput; // 环形缓冲区的当前放入位置
  int iget; // 缓冲区的当前取出位置
  int n; // 环形缓冲区中的元素总数量
  unsigned char buffer[COMMMAX];
 };
//状态提示信息缓冲
//Type
//          11 -- 呼叫中心
//          12 -- 户户通话
//          13 -- 监视
//          14 -- 免打扰
//          15 -- 查看留影
//          16 -- 对讲图像窗口

struct Remote1{
               int DenNum;             //目标数量  主机+副机
               unsigned char DenIP[4]; //对方IP或视频服务器IP
               unsigned char GroupIP[4]; //GroupIP
               unsigned char IP[10][4];    //对方IP
               int Added[10];                //已加入组
               char Addr[10][21];         //对方Addr
               int isDirect;       //主叫是否直通  0 直通  1 中转
           //    int isAssiDirect;       //被叫是否直通  0 直通  1 中转
              };


//单条留言内容结构体
struct LeaveWord1{
               uint8_t isValid;  //有效，未删除标志   1
               uint8_t isReaded; //已读标志    1
               uint8_t isLocked; //锁定标志    1
               char Time[32];    //接收时间    32
               uint8_t Type;     //类型        1   1--本地  2 --电话
               uint32_t Sn;      //序号        4
               int Length;       //长度        4
              };
//留言链表
typedef struct leavewordnode{
               struct LeaveWord1 LWord;
               struct node *llink, *rlink;
}LeaveWordNode1;                                       //内存分配为444

//存储文件到FLASH队列 数据结构 由于存储FLASH速度较慢 用线程来操作
struct Save_File_Buff1{
               int isValid; //是否有效
               int Type;    //存储类型 1－一类信息  2－单个信息  3－物业服务  4－本地设置
               int InfoType;   //信息类型
               int InfoNo;     //信息位置
               char cFromIP[15];   //用于读ID卡应答
              };

//UDP主动命令数据发送结构
struct Multi_Udp_Buff1{
               int isValid; //是否有效
               int SendNum; //当前发送次数
               int CurrOrder;//当前命令状态,VIDEOTALK VIDEOTALKTRANS VIDEOWATCH VIDEOWATCHTRANS
                             //主要用于需解析时，如单次命令置为0
               int m_Socket;
               char RemoteHost[20];
               char RemoteIP[20];               
               char DenIP[20]; //目标地址
               int RemotePort;
               unsigned char buf[1500];
               int DelayTime;  //等待时间
               int SendDelayTime; //发送等待时间计数  20101112
               int nlength;
              };
//COMM主动命令数据发送结构
struct Multi_Comm_Buff1{
               int isValid; //是否有效
               int SendNum; //当前发送次数
               int m_Comm;
               unsigned char buf[1500];
               int nlength;
              };
//呼叫输入框
struct Call_Input1{
               int Len; //是否有效
               char buf[20];
              };
//通话数据结构
struct talkdata1
  {
//   unsigned char Order;     //操作命令
   char HostAddr[20];       //主叫方地址
   unsigned char HostIP[4]; //主叫方IP地址
   char AssiAddr[20];       //被叫方地址
   unsigned char AssiIP[4]; //被叫方IP地址
   unsigned int timestamp;  //时间戳
   unsigned short DataType;          //数据类型
   unsigned short Frameno;           //帧序号
   unsigned int Framelen;            //帧数据长度
   unsigned short TotalPackage;      //总包数
   unsigned short CurrPackage;       //当前包数
   unsigned short Datalen;           //数据长度
   unsigned short PackLen;       //数据包最大长度
   char HeadSpace[9];
   }__attribute__ ((packed));
//信息数据结构
struct infodata1
  {
   char Addr[20];       //地址编码
   unsigned short Type; //类型
   unsigned int  Sn;         //序号
   unsigned short Length;   //数据长度
  }__attribute__ ((packed));

struct downfile1
  {
   char FlagText[20];     //标志字符串
   char FileName[20];
   unsigned int Filelen;            //文件大小
   unsigned short TotalPackage;      //总包数
   unsigned short CurrPackage;       //当前包数
   unsigned short Datalen;           //数据长度
  }__attribute__ ((packed));  
#ifndef CommonH
#define CommonH

  extern int DebugMode;

  struct TimeStamp1 TimeStamp;  //接收时间与播放时间，同步用
  //语音播放缓冲
  struct WavFileBuf1 WavFileBuf[WAVFILEMAX];

  struct LCD_Display1 LCD_Bak;   //LCD屏幕显示
  struct LCD_Display1 LCD_Display;   //LCD屏幕显示
  struct flcd_data f_data;
  int             fbdev;
  unsigned char  *fbmem;
  unsigned int    fb_uvoffset;
  unsigned int    fb_width;
  unsigned int    fb_height;
  unsigned int    fb_depth;

  int DeltaLen;  //数据包有效数据偏移量

  char wavFile[80];

  struct tm *curr_tm_t;

  int temp_audio_n;      //音频接收缓冲个数
  TempAudioNode1 *TempAudioNode_h;    //音频接收缓冲列表
    
  //系统初始化标志
  int InitSuccFlag;
  //本机状态设置
  struct Local1 Local;
  struct LocalCfg1 LocalCfg;

  #ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
   //ID卡信息
   struct IDCardNo1 IDCardNo;
   //刷ID卡信息
   struct BrushIDCard1 BrushIDCard;
   //写ID卡信息
   struct RecvIDCardNo1 RecvIDCardNo;
  #endif
  #ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
   //捕获图片传到中心
   int Capture_Pic_Num;
   struct Capture_Pic_Center1 Capture_Pic_Center[MAX_CAPTUREPIC_NUM];
   int Capture_Total_Package;
   unsigned char Capture_Send_Flag[2000];
  #endif
    
  //防区状态
  struct DefenceCfg1 DefenceCfg;

  //远端地址
  struct Remote1 Remote;
  char NullAddr[21];   //空字符串
  //呼叫输入框
  struct Call_Input1 Call_Input;
  //COMM
  int Comm2fd;  //串口2句柄
  int Comm3fd;  //串口3句柄
  int Comm4fd;  //串口4句柄
  //I2C
  int i2c_fd;
  //免费ARP
  int ARP_Socket;
  //检测网络连接
  int m_EthSocket;
  //UDP
  int m_DataSocket;
  int m_VideoSocket;
  int LocalDataPort;   //命令及数据UDP端口
  int LocalVideoPort;  //音视频UDP端口
  int RemoteDataPort;
  int RemoteVideoPort;
  int RemoteVideoServerPort;  //视频服务器音视频UDP端口
  char RemoteHost[20];
  char sPath[80];
  char currpath[80];   //自定义路径
  char wavPath[80];
  char UdpPackageHead[15];
  //FLASH存储线程
  int save_file_flag;
  pthread_t save_file_thread;
  void save_file_thread_func(void);
  sem_t save_file_sem;
  struct Save_File_Buff1 Save_File_Buff[SAVEMAX]; //FLASH存储缓冲最大值

  #ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
   //拆分包发送线程    如读ID卡应答时
   int dispart_send_flag;
   pthread_t dispart_send_thread;
   void dispart_send_thread_func(void);
   sem_t dispart_send_sem;
   struct Save_File_Buff1 Dispart_Send_Buff[DISPARTMAX]; //拆分包缓冲最大值
  #endif 

  //主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
  //用于UDP和Comm通信
  int multi_send_flag;
  pthread_t multi_send_thread;
  void multi_send_thread_func(void);
  sem_t multi_send_sem;
  struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10个UDP主动发送缓冲

  //主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
  //用于UDP和Comm通信
  int multi_comm_send_flag;
  pthread_t multi_comm_send_thread;
  void multi_comm_send_thread_func(void);
  sem_t multi_comm_send_sem;
  struct Multi_Comm_Buff1 Multi_Comm_Buff[COMMSENDMAX]; //10个COMM主动发送缓冲

  //watchdog
  int watchdog_fd; 


  int OpenLockTime[4];       //开锁时间
  char OpenLockTimeText[4][20];  //文本
  int DelayLockTime[4];       //延时开锁时间
  char DelayLockTimeText[4][20];       //文本
  char CallWaitRingText[4][20];       //呼叫回铃文本

  int CurrRow;                  //当前行

  struct TImage num_back_image;    //数字背景图像
  struct TImage num_image[13];    //数字图像

  struct TImage num_yk[11];

  struct TImage talk_image;		 //欢迎界面

  struct TImage talk1_image;    //可视对讲背景图像  单元
  struct TImage talk2_image;    //可视对讲背景图像  围墙
  
  struct TImage talk_connect_image;    //可视对讲 正在连接背景图像  单元

  struct TImage talk_start_image;    //可视对讲 通话背景图像  单元
  struct TLabel label_talk_start;    //可视对讲 通话背景 开锁Label

  struct TImage watch_image;    //监视背景图像

  struct TEdit openlock_edit;      //密码开锁Edit
  struct TImage openlock_image;    //密码开锁背景图像
  struct TImage door_openlock;    //门锁已打开

  //户户通话文本输入框
  struct TEdit r2r_edit[4];
  int CurrBox;            //当前输入框  0  1  2  3

  struct TImage setupback_image;    //设置窗口背景图像
  struct TEdit setup_pass_edit;           //设置窗口输入密码文本框

  struct TImage setupmain_image;

  struct TImage setup_info_image;      //查询本机信息窗口

  struct TImage setup_modify_image;       //修改大窗口

  struct TImage addr_image[1];     //房号设置窗口Image
  struct TImage mac_image[1];     //MAC地址设置窗口Image
  struct TImage ip_image[1];     //IP地址设置窗口Image
  struct TImage ipmask_image[1];     //子网掩码设置窗口Image
  struct TImage ipgate_image[1];     //网关地址设置窗口Image
  struct TImage ipserver_image[1];     //服务器地址设置窗口Image

  struct TImage engineer_pass_image[2];     //管理员密码设置窗口Image
  struct TImage lock_pass_image[2];     //开锁密码设置窗口Image

  struct TEdit modi_engi_edit[2];  //修改工程密码窗口文本框
  int Modi_Pass_Input_Num;  //修改密码输入次数
  int pass_win_no;

  struct TEdit addr_edit[6];  //房号设置窗口文本框

  struct TEdit mac_edit[1];  //MAC 地址设置窗口文本框

  struct TEdit ip_edit[5];  //IP地址设置窗口文本框

  struct TEdit ipmask_edit[1];  //子网掩码设置窗口文本框

  struct TEdit ipgate_edit[1];  //网关地址设置窗口文本框

  struct TEdit ipserver_edit[1];  //服务器地址设置窗口文本框

#else

  //****************add by xuqd*************************
  extern struct TImage talkover_image;		//呼叫结束
  //********************end*****************************

  extern int DebugMode;           //调试模式
  extern struct TimeStamp1 TimeStamp;  //接收时间与播放时间，同步用
  //语音播放缓冲
  extern struct WavFileBuf1 WavFileBuf[WAVFILEMAX];

  extern struct LCD_Display1 LCD_Bak;   //LCD屏幕显示
  extern struct LCD_Display1 LCD_Display;   //LCD屏幕显示
  extern struct flcd_data f_data;
  extern int             fbdev;
  extern unsigned char  *fbmem;
  extern unsigned int    fb_uvoffset;
  extern unsigned int    fb_width;
  extern unsigned int    fb_height;
  extern unsigned int    fb_depth;

  extern int DeltaLen;  //数据包有效数据偏移量

  extern char wavFile[80];

  extern struct tm *curr_tm_t;

  extern int temp_audio_n;      //音频接收缓冲个数
  extern TempAudioNode1 *TempAudioNode_h;    //音频接收缓冲列表
  //系统初始化标志
  extern int InitSuccFlag;  
  //本机状态设置
  extern struct Local1 Local;
  extern struct LocalCfg1 LocalCfg;
  
  #ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
   //ID卡信息
   extern struct IDCardNo1 IDCardNo;
   //刷ID卡信息
   extern struct BrushIDCard1 BrushIDCard;
   //写ID卡信息
   extern struct RecvIDCardNo1 RecvIDCardNo;
  #endif

  #ifdef _CAPTUREPIC_TO_CENTER  //捕获图片传到中心
   //捕获图片传到中心
   extern int Capture_Pic_Num;
   extern struct Capture_Pic_Center1 Capture_Pic_Center[MAX_CAPTUREPIC_NUM];
   extern int Capture_Total_Package;
   extern unsigned char Capture_Send_Flag[2000];
  #endif
    
  //防区状态
  extern struct DefenceCfg1 DefenceCfg;
  //远端地址
  extern struct Remote1 Remote;
  extern char NullAddr[21];   //空字符串
  //呼叫输入框
  extern struct Call_Input1 Call_Input;  
  //COMM
  extern int Comm2fd;  //串口2句柄
  extern int Comm3fd;  //串口3句柄
  extern int Comm4fd;  //串口4句柄
  //I2C
  extern int i2c_fd;
  //免费ARP
  extern int ARP_Socket;
  //检测网络连接
  extern int m_EthSocket;
  //UDP
  extern int m_DataSocket;
  extern int m_VideoSocket;
  extern int LocalDataPort;   //命令及数据UDP端口
  extern int LocalVideoPort;  //音视频UDP端口
  extern int RemoteDataPort;
  extern int RemoteVideoPort;
  extern int RemoteVideoServerPort;  //视频服务器音视频UDP端口
  extern char RemoteHost[20];
  extern char sPath[80];
  extern char currpath[80];   //自定义路径
  extern char wavPath[80];
  extern char UdpPackageHead[15];
  //FLASH存储线程
  extern int save_file_flag;
  extern pthread_t save_file_thread;
  extern void save_file_thread_func(void);
  extern sem_t save_file_sem;
  extern struct Save_File_Buff1 Save_File_Buff[SAVEMAX]; //FLASH存储缓冲最大值

  #ifdef _BRUSHIDCARD_SUPPORT   //刷卡功能支持
   //拆分包发送线程    如读ID卡应答时
   extern int dispart_send_flag;
   extern pthread_t dispart_send_thread;
   extern void dispart_send_thread_func(void);
   extern sem_t dispart_send_sem;
   extern struct Save_File_Buff1 Dispart_Send_Buff[DISPARTMAX]; //拆分包缓冲最大值
  #endif 

  //主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
  //用于UDP和Comm通信
  extern int multi_send_flag;
  extern pthread_t multi_send_thread;
  extern void multi_send_thread_func(void);
  extern sem_t multi_send_sem;
  extern struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10个UDP主动发送缓冲
  //主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
  //用于UDP和Comm通信
  extern int multi_comm_send_flag;
  extern pthread_t multi_comm_send_thread;
  extern void multi_comm_send_thread_func(void);
  extern sem_t multi_comm_send_sem;
  extern struct Multi_Comm_Buff1 Multi_Comm_Buff[COMMSENDMAX]; //10个COMM主动发送缓冲

  //watchdog
  extern int watchdog_fd;

  extern int OpenLockTime[4];       //开锁时间
  extern char OpenLockTimeText[4][20];  //文本
  extern int DelayLockTime[4];       //延时开锁时间
  extern char DelayLockTimeText[4][20];       //文本
  extern char CallWaitRingText[4][20];       //呼叫回铃文本

  extern int CurrRow;                  //当前行

  extern struct TImage num_back_image;    //数字背景图像
  extern struct TImage num_image[13];    //数字图像

  extern struct TImage num_yk[11];

  extern struct TImage talk_image;		 //欢迎界面

  extern struct TImage talk1_image;    //可视对讲背景图像  单元
  extern struct TImage talk2_image;    //可视对讲背景图像  围墙

  extern struct TImage talk_connect_image;    //可视对讲 正在连接背景图像  单元

  extern struct TImage talk_start_image;    //可视对讲 通话背景图像  单元
  extern struct TLabel label_talk_start;    //可视对讲 通话背景 开锁Label

  extern struct TImage watch_image;    //监视背景图像

  extern struct TEdit openlock_edit;      //密码开锁Edit
  extern struct TImage openlock_image;    //密码开锁背景图像
  extern struct TImage door_openlock;    //门锁已打开

  extern  struct TImage sip_ok;
  extern struct TImage sip_error;

  //户户通话文本输入框
  extern struct TEdit r2r_edit[4];
  extern int CurrBox;            //当前输入框  0  1  2  3

  extern struct TImage setupback_image;    //设置窗口背景图像
  extern struct TEdit setup_pass_edit;           //设置窗口输入密码文本框

  extern struct TImage setupmain_image;

  extern struct TImage setup_info_image;      //查询本机信息窗口

  extern struct TImage setup_modify_image;       //修改大窗口

  extern struct TImage addr_image[1];     //房号设置窗口Image
  extern struct TImage mac_image[1];     //MAC地址设置窗口Image
  extern struct TImage ip_image[1];     //IP地址设置窗口Image
  extern struct TImage ipmask_image[1];     //子网掩码设置窗口Image
  extern struct TImage ipgate_image[1];     //网关地址设置窗口Image
  extern struct TImage ipserver_image[1];     //服务器地址设置窗口Image

  extern struct TImage engineer_pass_image[2];     //管理员密码设置窗口Image
  extern struct TImage lock_pass_image[2];     //开锁密码设置窗口Image

  extern struct TEdit modi_engi_edit[2];  //修改工程密码窗口文本框
  extern int Modi_Pass_Input_Num;  //修改密码输入次数
  extern int pass_win_no;

  extern struct TEdit addr_edit[6];  //房号设置窗口文本框

  extern struct TEdit mac_edit[1];  //MAC 地址设置窗口文本框

  extern struct TEdit ip_edit[5];  //IP地址设置窗口文本框

  extern struct TEdit ipmask_edit[1];  //子网掩码设置窗口文本框

  extern struct TEdit ipgate_edit[1];  //网关地址设置窗口文本框

  extern struct TEdit ipserver_edit[1];  //服务器地址设置窗口文本框
#endif
