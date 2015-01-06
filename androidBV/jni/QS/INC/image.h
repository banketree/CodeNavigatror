#ifndef IMAGE_H
#define IMAGE_H

#define VIDEO_PICTURE_QUEUE_SIZE_MAX 20

#include <inttypes.h>

//��ť
struct TImageButton{
               char jpgname_up[80];
               char jpgname_down[80];
               char buttonname[20];
               unsigned char *image[2]; //0--up  1--down
               int width;
               int height;
               int xLeft;
               int yTop;
               uint8_t Visible;
               int EventType;
              };
struct TImageGif{
               int framenum; //��֡��
               int currframe; //��ǰ֡
               int time;      //֡���ʱ��  ms
               unsigned char *image[10];
               int width;
               int height;
               int xLeft;
               int yTop;
               uint8_t Visible;
              };
struct TImage{
               unsigned char *image;
               unsigned char *image_down;
               int width;
               int height;
               int xLeft;
               int yTop;
               uint8_t Visible;
              };
struct TLabel{
               unsigned char *image;      //����
               unsigned char *image_h;    //����
               int width;
               int height;
               int xLeft;
               int yTop;
               char Text[100];
               uint8_t Visible;
              };
struct TEdit{
               unsigned char *image;
               int width;
               int height;
               int xLeft;
               int yTop;
               int CursorX; //���X����
               int CursorY; //���Y����
               int CursorHeight; //���߶�
               int CursorCorlor;
               int fWidth;     //�ֿ��
               unsigned char *Cursor_H;     //���
               char Text[20];
               int BoxLen;         //�ı���ǰ���볤��
               int MaxLen;         //�ı���������볤��
               uint8_t Focus;        //��ʾ������
               uint8_t Visible;
              };
struct TPopupWin{
               unsigned char *image[2]; //0 --ͼ��  1--����
               int width;
               int height;
               int xLeft;
               int yTop;
               uint8_t Visible;
              };

struct flcd_data
{
    unsigned int buf_len;
    unsigned int uv_offset;
    unsigned int frame_no;
    unsigned int mp4_map_dma[VIDEO_PICTURE_QUEUE_SIZE_MAX];
};
/*��ʼ��
  fbmem - framebuffer ��ҳ��ַ
  fb_width - framebuffer ���
  fb_height - framebuffer �߶�
  f_data - flcd_data �ṹ
  return - 1:��ʼ���ɹ�  0:��ʼ��ʧ��
*/
int InitLibImage(unsigned char *fbmem, int fb_width, int fb_height, struct flcd_data *f_data);
/*����һJPG�ļ�����ʾ��framebuffer��
  xleft - ͼ�� x ����
  ytop - ͼ�� y ����
  jpegname - �ļ����ƣ�linuxֻ֧�־���·������֧�����·����
  true_size - 1: ��ͼ��ʵ�ʴ�С��ʾ  0: ��ʾ��һ��С�������ڣ�����������ʾ���ǽ�ȡ���ϲ�����ʾ��
  rect_width��rect_height -  true_size = 0 ʱ��Ч��С����Ŀ�Ⱥ͸߶�
  PageNo - ����ͼ����ʾ��framebuffer�ĵڼ�ҳ��0Ϊ��һҳ��
  return - 1:�ɹ�  0:ʧ��
  ע�����ڸú�����Ҫ�Ƚ���JPG��Ȼ����ʾ�����ٶȽ������ʺ�����ʾһЩ�����õ�ͼ��
      ��Ҫ��ʾһЩ����ͼ�񣬽������ImageLoadFromFile��DisplayImage����ʾ
      ���ڽ����ԭ��JPGͼ������ACDsee���ת����JPGͼ��ĸ߶���Ϊ16�ı��������������������쳣
*/
int DisplayJPG(int xleft, int ytop, char *jpegname, int true_size, int rect_width, int rect_height, int PageNo);

/*��JPG�ļ��н�ȡͼ�񻺳�, pic_buf ΪYUV420��ʽ
  jpegname - �ļ����ƣ�linuxֻ֧�־���·������֧�����·����
  rect_width - ͼ����
  rect_height - ͼ��߶�
  pic_buf - ��������ַ
  return - 1:�ɹ�  0:ʧ��
  ע�����ڽ����ԭ��JPGͼ������ACDsee���ת����JPGͼ��ĸ߶���Ϊ16�ı��������������������쳣
*/
int CapturePicFromJPG(char *jpegname, int rect_width, int rect_height, uint8_t *pic_buf);

/*��JPG�ļ��м���Buttonͼ�ΰ���
  jpegname - �ļ����ƣ�linuxֻ֧�־���·������֧�����·����
  t_button - ����Button�ṹ
  buttontype - ����Button ͼ������ͣ�0 : ����ͼ��  1 : ����ͼ��
  return - 1:�ɹ�  0:ʧ��
  ע�����ڽ����ԭ��JPGͼ������ACDsee���ת����JPGͼ��ĸ߶���Ϊ16�ı��������������������쳣  
*/
int ImageButtonLoadFromFile(char *jpegname, struct TImageButton *t_button, uint8_t buttontype);

/*��ʾͼ�ΰ���
  t_button - ����Button�ṹ
  buttontype - ����Button ͼ������ͣ�0 : ����ͼ��  1 : ����ͼ��
  PageNo - ����ͼ����ʾ��framebuffer�ĵڼ�ҳ��0Ϊ��һҳ��
  return - 1:�ɹ�  0:ʧ��
*/
int DisplayImageButton(struct TImageButton *t_button, uint8_t buttontype, int PageNo);

//�ͷ�ͼ�ΰ�������
//  return - 1:�ɹ�  0:ʧ��
int FreeImageButton(struct TImageButton *t_button);

//��JPG�ļ��м���Gif
//  return - 1:�ɹ�  0:ʧ��
int GifLoadFromFile(char *jpegname, struct TImageGif *t_gif);
//��ʾGif
//  return - 1:�ɹ�  0:ʧ��
int DisplayGif(struct TImageGif *t_gif, int PageNo);
//�ͷ�Gif
//  return - 1:�ɹ�  0:ʧ��
int FreeGif(struct TImageGif *t_gif);

/*��JPG�ļ��м���Imageͼ��
  jpegname - �ļ����ƣ�linuxֻ֧�־���·������֧�����·����
  t_image - Imageͼ�νṹ
  return - 1:�ɹ�  0:ʧ��
  ע�����ڽ����ԭ��JPGͼ������ACDsee���ת����JPGͼ��ĸ߶���Ϊ16�ı��������������������쳣
*/
int ImageLoadFromFile(char *jpegname, struct TImage *t_image);

/*��ʾImage
  PageNo - ����ͼ����ʾ��framebuffer�ĵڼ�ҳ��0Ϊ��һҳ��
  return - 1:�ɹ�  0:ʧ��
*/
int DisplayImage(struct TImage *t_image, int PageNo);

//�ͷ�Image
//  return - 1:�ɹ�  0:ʧ��
int FreeImage(struct TImage *t_image);

/*��JPG�ļ��м���Edit�ı���
  jpegname - �ļ����ƣ�linuxֻ֧�־���·������֧�����·����
  t_edit - Edit�ı���ṹ
  return - 1:�ɹ�  0:ʧ��
  ע�����ڽ����ԭ��JPGͼ������ACDsee���ת����JPGͼ��ĸ߶���Ϊ16�ı��������������������쳣
*/
int EditLoadFromFile(char *jpegname, struct TEdit *t_edit);
//��ʾEdit�ı���
//  return - 1:�ɹ�  0:ʧ��
int DisplayEdit(struct TEdit *t_edit, int PageNo);
//�ͷŻ���
//  return - 1:�ɹ�  0:ʧ��
int FreeEdit(struct TEdit *t_edit);

/*��ʾ��������
  t_popupwin - PopupWin�������ڽṹ
  w_type - 0 : ��ʾ  1 : ����
  PageNo - ����ͼ����ʾ��framebuffer�ĵڼ�ҳ��0Ϊ��һҳ��
  return - 1:�ɹ�  0:ʧ��
*/
int DisplayPopupWin(struct TPopupWin *t_popupwin, uint8_t w_type, int PageNo);

/*����Ļ�ϱ���һ�����򵽻�����
  xleft - ��Ļ x ����
  ytop - ��Ļ y ����
  nwidth - ���������
  nheight - �������߶�
  PicBuf - ��������ַ
  PageNo - ��Ļ��framebuffer�ĵڼ�ҳ
  return - 1:�ɹ�  0:ʧ��
*/
int SavePicBuf_Func(int xleft, int ytop, int nwidth, int nheight, unsigned char *PicBuf, int PageNo);

/*дһ�黺��������Ļ
  xleft - ��Ļ x ����
  ytop - ��Ļ y ����
  nwidth - ���������
  nheight - �������߶�
  PicBuf - ��������ַ
  PageNo - ��Ļ��framebuffer�ĵڼ�ҳ
  return - 1:�ɹ�  0:ʧ��
*/
int RestorePicBuf_Func(int xleft, int ytop,
                   int nwidth, int nheight, unsigned char *PicBuf, int PageNo);

/*��һ������������һ��������һ��������
  xleft -  x ����
  ytop -  y ����
  dwidth - Ŀ��(��)���������
  dheight - Ŀ��(��)�������߶�
  dest_PicBuf - Ŀ��(��)��������ַ
  swidth - Դ(��)���������
  sheight - Դ(��)�������߶�
  src_PicBuf - Դ(��)��������ַ
  return - 1:�ɹ�  0:ʧ��
  ע��Ŀ��(��)��������ȡ��߶���С�ڻ����Դ(��)��������ȡ��߶�
*/
int SavePicS_D_Func(int xleft, int ytop, int dwidth, int dheight,
              unsigned char *dest_PicBuf,int swidth, int sheight, unsigned char *src_PicBuf);
#endif /* IMAGE_H */
