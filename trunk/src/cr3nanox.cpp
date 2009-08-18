//
// C++ Implementation: jinke/lbook V3 viewer plugin
//
// Description:
//
//
// Author: Vadim Lopatin <vadim.lopatin@coolreader.org>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <unistd.h>
#include <sys/wait.h>
#include "cr3jinke.h"
#include <crengine.h>
#include <crgui.h>
#include <microwin/nano-X.h>
#include <microwin/nxcolors.h>
#include "cr3main.h"
#include "mainwnd.h"

#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <semaphore.h>

//#include <math.h>

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

// uncomment following line to allow running executables named .exe.txt
#define ALLOW_RUN_EXE 1
// uncomment to use separate .ini files for different formats
#define SEPARATE_INI_FILES 1

#define LEDTHREAD 1

static bool firstDocUpdate = true;


status_info_t lastState = {0,0,0};

static char last_bookmark[2048]= {0};
static int last_bookmark_page = 0;

static bool shuttingDown = false;

#define USE_JINKE_USER_DATA 0
#define USE_OWN_BATTERY_TEST 0

int getBatteryState()
{
#if 1
    // TODO: battery state
    return 100;
#else
#if USE_OWN_BATTERY_TEST==0
    lastState.batteryState = v3_callbacks->GetBatteryState();
    if ( lastState.batteryState >=0 && lastState.batteryState <=4 )
        return lastState.batteryState * 100 / 4;
    return 100;
#else
    FILE * f = fopen( "/dev/misc/s3c2410_batt", "rb" );
    if ( !f )
        return -1;
    int ch = fgetc( f );
    fclose(f);
    if ( ch == ' ' )
        return -1;
    if ( ch>=0 && ch <= 16 )
        return ch * 100 / 16;
    return 100;
#endif
#endif
}

#include <cri18n.h>

#define VIEWER_WINDOW_X         0
#define VIEWER_WINDOW_Y         0
#define VIEWER_WINDOW_WIDTH    600
#define VIEWER_WINDOW_HEIGHT   800

#if 0
#define COLOR_LEVEL         4 //4 bit bitmap
#define BMP_PANEL_NUBS   (1<<COLOR_LEVEL)//Palette numbers



const char *g_ImageResInfo[]=
{
    "logo",
};



//Only  support 16 degree bitmap
const unsigned long DegreePanel[16]={0x00000000,0x00505050,0x00808080,0x00ffffff,0x00ffffff,
                                     0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
                                     0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
                                     0x00ffffff};
typedef struct
{
    unsigned short int  bfType;
    unsigned long bfSize;
    unsigned short int  bfReserved1;
    unsigned short int  bfReserved2;
    unsigned long bfOffBits;
}BITMAPFILEHEADER;

#define BITMAPFILEHEADER_SIZE 14

typedef struct
{
    unsigned long biSize;
    long biWidth;
    long biHeight;
    unsigned short int biPlanes;
    unsigned short int biBitCount;
    unsigned long biCompression;
    unsigned long biSizeImage;
    long biXPelsPerMeter;
    long biYPelsPerMeter;
    unsigned long biClrUsed;
    unsigned long biClrImportant;
}BITMAPINFOHEADER;
#define BITMAPINFOHEADER_SIZE sizeof(BITMAPINFOHEADER)

typedef struct
{
    BITMAPFILEHEADER FileHeader;
    BITMAPINFOHEADER BmpInfo;
    MWPALENTRY   BmpPanel[BMP_PANEL_NUBS]; 
}BMPHEADER_16;


//Change 2 bit bitmap data to 4 bit bimmap picture and draw it
int GrBitmapEx(GR_WINDOW_ID id,GR_GC_ID gc,int x,int y,int width,int height,GR_CHAR *imagebits)
{
    BMPHEADER_16  lpHeader;
    int i;
    int Lines;
    int BmpBufBytsPerLine,BytesPerLine;
    unsigned char char1,char2,char3,char4,BmpChar1;
    unsigned char *Screen_Buf=NULL,*BufPtr,*BmpFileBuf,*HeaderBuf;
    int Bmpheadersize;
    GR_IMAGE_ID ImageId;

    //start
    Bmpheadersize=BITMAPFILEHEADER_SIZE+BITMAPINFOHEADER_SIZE+sizeof(MWPALENTRY)*BMP_PANEL_NUBS;
    BytesPerLine=(((width*4+7)/8+3)/4)*4;
    BmpBufBytsPerLine=((height*2+7)/8);
    
    BmpFileBuf=(unsigned char*)malloc(height*BytesPerLine+Bmpheadersize);
    if(!BmpFileBuf)
    {
        return 0;
    }
    memset(BmpFileBuf,0,height*BytesPerLine);
    HeaderBuf=BmpFileBuf;
    Screen_Buf=BmpFileBuf+Bmpheadersize;
    //Set Panel
   for(i=0;i<BMP_PANEL_NUBS;i++)
    {
        memcpy((unsigned char*)&lpHeader.BmpPanel[i],(unsigned char*)&DegreePanel[i],\
                sizeof(MWPALENTRY));
    }
    
    memcpy((unsigned char *)&lpHeader.FileHeader.bfType,"BM",2);
    lpHeader.FileHeader.bfSize=height*BytesPerLine+Bmpheadersize;
    lpHeader.FileHeader.bfReserved1=0;
    lpHeader.FileHeader.bfReserved2=0;
    lpHeader.FileHeader.bfOffBits=Bmpheadersize;
    lpHeader.BmpInfo.biSize=BITMAPINFOHEADER_SIZE;
    lpHeader.BmpInfo.biWidth=width;
    lpHeader.BmpInfo.biHeight=height;
    lpHeader.BmpInfo.biPlanes=1;
    lpHeader.BmpInfo.biBitCount=COLOR_LEVEL;
    lpHeader.BmpInfo.biCompression=0;
    lpHeader.BmpInfo.biSizeImage=height*BytesPerLine;
    lpHeader.BmpInfo.biXPelsPerMeter=0xEC4;
    lpHeader.BmpInfo.biYPelsPerMeter=0xEC4;
    lpHeader.BmpInfo.biClrUsed=0;
    lpHeader.BmpInfo.biClrImportant=0;

    memcpy(HeaderBuf,(char *)&lpHeader.FileHeader.bfType,2);
    memcpy(&HeaderBuf[2],(char *)&lpHeader.FileHeader.bfSize,4);
    memcpy(&HeaderBuf[6],(char *)&lpHeader.FileHeader.bfReserved1,2);
    memcpy(&HeaderBuf[8],(char *)&lpHeader.FileHeader.bfReserved2,2);
    memcpy(&HeaderBuf[10],(char *)&lpHeader.FileHeader.bfOffBits,4);
    HeaderBuf+=BITMAPFILEHEADER_SIZE;
    memcpy(HeaderBuf,(unsigned char *)&lpHeader.BmpInfo,BITMAPINFOHEADER_SIZE);
    HeaderBuf+=BITMAPINFOHEADER_SIZE;
    memcpy(HeaderBuf,(unsigned char *)lpHeader.BmpPanel,sizeof(MWPALENTRY)*BMP_PANEL_NUBS);
    HeaderBuf=BmpFileBuf;   
                
    Lines=0;
    //4 color degress change to 16 color degress,mainframe_img_bits is 4 color degress
    for(Lines=0;Lines<height;Lines++)
    {
        
        char1=0;
        char2=0;
        char3=0;
        char4=0;
        BufPtr=(unsigned char *)&Screen_Buf[(height-1-Lines)*BytesPerLine];
        for(i=0;i<BmpBufBytsPerLine;i++)
        {

            BmpChar1=imagebits[i+Lines*BmpBufBytsPerLine];
            char1=BmpChar1&0xc0;
            char1>>=6;
            char2=BmpChar1&0x30;
            char2>>=4;
            char1=(char1<<4)|char2;
            *BufPtr=char1;
            BufPtr++;

            char3=BmpChar1&0x0c;
            char3>>=2;
            char4=BmpChar1&0x03;
            char3=(char3<<4)|char4;
            *BufPtr=char3;
            BufPtr++;
        }
    }
    CRLog::trace("GrLoadImageFromBuffer");
    ImageId=GrLoadImageFromBuffer(BmpFileBuf,lpHeader.FileHeader.bfSize,GR_BACKGROUND_TOPLEFT);
    CRLog::trace("GrDrawImageToFit");
    GrDrawImageToFit(id,gc,x,y,width,height,ImageId);
    free(BmpFileBuf);
    return 0;
}

#endif

#ifndef USE_OLD_NANOX

#define COLOR_LEVEL     4 //图像转换成16级徽度的图形
#define BMPBUFGRAYLEVEL 2 //原是图形数据是4级徽度的图形
#define BMP_PANEL_NUBS   (1<<COLOR_LEVEL)//调色版颜色个数

const  unsigned long  panel256[]={
0X00000000,0X00010101,0X00020202,0X00030303,0X00040404,0X00050505,0X00060606,0X00070707,
0X00080808,0X00090909,0X000a0a0a,0X000b0b0b,0X000c0c0c,0X000d0d0d,0X000e0e0e,0X000f0f0f,
0X00101010,0X00111111,0X00121212,0X00131313,0X00141414,0X00151515,0X00161616,0X00171717,
0X00181818,0X00191919,0X001a1a1a,0X001b1b1b,0X001c1c1c,0X001d1d1d,0X001e1e1e,0X001f1f1f,
0X00202020,0X00212121,0X00222222,0X00232323,0X00242424,0X00252525,0X00262626,0X00272727,
0X00282828,0X00292929,0X002a2a2a,0X002b2b2b,0X002c2c2c,0X002d2d2d,0X002e2e2e,0X002f2f2f,
0X00303030,0X00313131,0X00323232,0X00333333,0X00343434,0X00353535,0X00363636,0X00373737,
0X00383838,0X00393939,0X003a3a3a,0X003b3b3b,0X003c3c3c,0X003d3d3d,0X003e3e3e,0X003f3f3f,
0X00404040,0X00414141,0X00424242,0X00434343,0X00444444,0X00454545,0X00464646,0X00474747,
0X00484848,0X00494949,0X004a4a4a,0X004b4b4b,0X004c4c4c,0X004d4d4d,0X004e4e4e,0X004f4f4f,
0X00505050,0X00515151,0X00525252,0X00535353,0X00545454,0X00555555,0X00565656,0X00575757,
0X00585858,0X00595959,0X005a5a5a,0X005b5b5b,0X005c5c5c,0X005d5d5d,0X005e5e5e,0X005f5f5f,
0X00606060,0X00616161,0X00626262,0X00636363,0X00646464,0X00656565,0X00666666,0X00676767,
0X00686868,0X00696969,0X006a6a6a,0X006b6b6b,0X006c6c6c,0X006d6d6d,0X006e6e6e,0X006f6f6f,
0X00707070,0X00717171,0X00727272,0X00737373,0X00747474,0X00757575,0X00767676,0X00777777,
0X00787878,0X00797979,0X007a7a7a,0X007b7b7b,0X007c7c7c,0X007d7d7d,0X007e7e7e,0X007f7f7f,
0X00808080,0X00818181,0X00828282,0X00838383,0X00848484,0X00858585,0X00868686,0X00878787,
0X00888888,0X00898989,0X008a8a8a,0X008b8b8b,0X008c8c8c,0X008d8d8d,0X008e8e8e,0X008f8f8f,
0X00909090,0X00919191,0X00929292,0X00939393,0X00949494,0X00959595,0X00969696,0X00979797,
0X00989898,0X00999999,0X009a9a9a,0X009b9b9b,0X009c9c9c,0X009d9d9d,0X009e9e9e,0X009f9f9f,
0X00a0a0a0,0X00a1a1a1,0X00a2a2a2,0X00a3a3a3,0X00a4a4a4,0X00a5a5a5,0X00a6a6a6,0X00a7a7a7,
0X00a8a8a8,0X00a9a9a9,0X00aaaaaa,0X00ababab,0X00acacac,0X00adadad,0X00aeaeae,0X00afafaf,
0X00b0b0b0,0X00b1b1b1,0X00b2b2b2,0X00b3b3b3,0X00b4b4b4,0X00b5b5b5,0X00b6b6b6,0X00b7b7b7,
0X00b8b8b8,0X00b9b9b9,0X00bababa,0X00bbbbbb,0X00bcbcbc,0X00bdbdbd,0X00bebebe,0X00bfbfbf,
0X00c0c0c0,0X00c1c1c1,0X00c2c2c2,0X00c3c3c3,0X00c4c4c4,0X00c5c5c5,0X00c6c6c6,0X00c7c7c7,
0X00c8c8c8,0X00c9c9c9,0X00cacaca,0X00cbcbcb,0X00cccccc,0X00cdcdcd,0X00cecece,0X00cfcfcf,
0X00d0d0d0,0X00d1d1d1,0X00d2d2d2,0X00d3d3d3,0X00d4d4d4,0X00d5d5d5,0X00d6d6d6,0X00d7d7d7,
0X00d8d8d8,0X00d9d9d9,0X00dadada,0X00dbdbdb,0X00dcdcdc,0X00dddddd,0X00dedede,0X00dfdfdf,
0X00e0e0e0,0X00e1e1e1,0X00e2e2e2,0X00e3e3e3,0X00e4e4e4,0X00e5e5e5,0X00e6e6e6,0X00e7e7e7,
0X00e8e8e8,0X00e9e9e9,0X00eaeaea,0X00ebebeb,0X00ececec,0X00ededed,0X00eeeeee,0X00efefef,
0X00f0f0f0,0X00f1f1f1,0X00f2f2f2,0X00f3f3f3,0X00f4f4f4,0X00f5f5f5,0X00f6f6f6,0X00f7f7f7,
0X00f8f8f8,0X00f9f9f9,0X00fafafa,0X00fbfbfb,0X00fcfcfc,0X00fdfdfd,0X00fefefe,0X00ffffff,
};
const unsigned long panel16[256]={
0x00000000,0x00505050,0x00808080,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,0x00ffffff,
};
                         

typedef struct
{
    unsigned short int  bfType;//文件类型
    unsigned long bfSize;//文件大小
    unsigned short int  bfReserved1;//保留
    unsigned short int  bfReserved2;//保留
    unsigned long bfOffBits;//图片数据位置
}BITMAPFILEHEADER;
#define BITMAPFILEHEADER_SIZE 14

typedef struct
{
    unsigned long biSize;//BMP 信息结构的大小
    long biWidth;//BMP宽度    
    long biHeight;//BMP 高度
    unsigned short int biPlanes;//保留(1) 
    unsigned short int biBitCount;//BMP 灰度等级
    unsigned long biCompression;//压缩类型
    unsigned long biSizeImage;//图片数据大小
    long biXPelsPerMeter;//X每英尺像素个数(固定)
    long biYPelsPerMeter;//Y每英尺像素个数(固定)
    unsigned long biClrUsed;//0表示调色版中的颜色值是最大的
    unsigned long biClrImportant;//0表示所有颜色都需要去调色版中查询
}BITMAPINFOHEADER;
#define BITMAPINFOHEADER_SIZE sizeof(BITMAPINFOHEADER)

typedef struct
{
    BITMAPFILEHEADER FileHeader;
    BITMAPINFOHEADER BmpInfo;
    MWPALENTRY   BmpPanel[BMP_PANEL_NUBS]; 
}BMPHEADER_16;

typedef struct
{
    BITMAPFILEHEADER FileHeader;
    BITMAPINFOHEADER BmpInfo;
    MWPALENTRY   BmpPanel[256]; 
}BMPHEADER_256;

int GrBitmapEx_Apollo_FOUR(GR_WINDOW_ID id,GR_GC_ID gc,int x,int y,int width,int height,int src_x,int src_y, int src_width,int src_height, GR_CHAR *imagebits)

{
    // TODO: Add extra validation here
    //4 bit 16 level degree color
    
    
    BMPHEADER_256 lpHeader;
    GR_IMAGE_ID ImageId;
    int HeaderLen;
    int i;
    int Lines;
    int BmpBufBytsPerLine=0,BytesPerLine=0;

    unsigned char char1,char2,char3,char4,BmpChar1;
    unsigned char *Screen_Buf=NULL,*BmpFileBuf=NULL,*BufPtr=NULL,*HeaderBuf=NULL;
    int GrayLevel=8;//256 gray level
    int BmpWidth=0,BmpHeight=0,PixesPerByte=0,BmpPanelNumbers=0,Bmpheadersize=0;

    BmpWidth=min(width,src_width);
    BmpHeight=min(height,src_height);

    PixesPerByte=8/GrayLevel;
    BmpPanelNumbers=1<<GrayLevel;
    BytesPerLine=(BmpWidth*GrayLevel+31)/32*4;
    BmpBufBytsPerLine=(src_width*2+7)/8;


    i=sizeof(BMPHEADER_256);
    HeaderLen=BITMAPFILEHEADER_SIZE+sizeof(BITMAPINFOHEADER)+sizeof(MWPALENTRY)*BmpPanelNumbers;
    memcpy((unsigned char *)&lpHeader.FileHeader.bfType,"BM",2);
    lpHeader.FileHeader.bfSize=(BmpHeight*BytesPerLine)/PixesPerByte+HeaderLen;
    lpHeader.FileHeader.bfReserved1=0;
    lpHeader.FileHeader.bfReserved2=0;
    lpHeader.FileHeader.bfOffBits=HeaderLen;
    lpHeader.BmpInfo.biSize=sizeof(BITMAPINFOHEADER);
    lpHeader.BmpInfo.biWidth=BmpWidth;
    lpHeader.BmpInfo.biHeight=BmpHeight;
    lpHeader.BmpInfo.biPlanes=1;
    lpHeader.BmpInfo.biBitCount=GrayLevel;
    lpHeader.BmpInfo.biCompression=0;
    lpHeader.BmpInfo.biSizeImage=lpHeader.FileHeader.bfSize-HeaderLen;
    lpHeader.BmpInfo.biXPelsPerMeter=0x257E;
    lpHeader.BmpInfo.biYPelsPerMeter=0x257E;
    lpHeader.BmpInfo.biClrUsed=0;
    lpHeader.BmpInfo.biClrImportant=0;
    
    Bmpheadersize=HeaderLen;
    BmpFileBuf=new unsigned char[BmpHeight*BytesPerLine+Bmpheadersize];

    if(!BmpFileBuf)
    {
        return 1;
    }
    memset(BmpFileBuf,0x0,BmpHeight*BytesPerLine+Bmpheadersize);
    HeaderBuf=BmpFileBuf;   
    Screen_Buf=BmpFileBuf+Bmpheadersize;
    BufPtr=Screen_Buf;

    memcpy(HeaderBuf,(char *)&lpHeader.FileHeader.bfType,2);
    memcpy(&HeaderBuf[2],(char *)&lpHeader.FileHeader.bfSize,4);
    memcpy(&HeaderBuf[6],(char *)&lpHeader.FileHeader.bfReserved1,2);
    memcpy(&HeaderBuf[8],(char *)&lpHeader.FileHeader.bfReserved2,2);
    memcpy(&HeaderBuf[10],(char *)&lpHeader.FileHeader.bfOffBits,4);
    HeaderBuf+=BITMAPFILEHEADER_SIZE;
    memcpy(HeaderBuf,(unsigned char *)&lpHeader.BmpInfo,BITMAPINFOHEADER_SIZE);
    HeaderBuf+=BITMAPINFOHEADER_SIZE;
    memcpy(HeaderBuf,(unsigned char *)&panel16,sizeof(MWPALENTRY)*BmpPanelNumbers);
    HeaderBuf=BmpFileBuf;   
                
    Lines=0;
    //4 color degress change to 16 color degress,mainframe_img_bits is 4 color degress
    for(Lines=0;Lines<BmpHeight;Lines++)
    {
        
        char1=0;
        char2=0;
        char3=0;
        char4=0;
        //倒像数据
        BufPtr=(unsigned char *)&Screen_Buf[(BmpHeight-1-Lines)*BytesPerLine];
        for(i=0;i<(BmpWidth*2+7)/8;i++)
        {
            BmpChar1=imagebits[i+(Lines+src_y)*BmpBufBytsPerLine+src_x/4];
            //一个字节转换成2个字节
            char1=BmpChar1&0xc0;
            char1>>=6;
            *BufPtr=char1;
            BufPtr++;
            char2=BmpChar1&0x30;
            char2>>=4;
            *BufPtr=char2;
            BufPtr++;
            char3=BmpChar1&0x0c;
            char3>>=2;
            *BufPtr=char3;
            BufPtr++;
            char4=BmpChar1&0x03;
            *BufPtr=char4;
            BufPtr++;
        }
    }
    ImageId=GrLoadImageFromBuffer(BmpFileBuf,lpHeader.FileHeader.bfSize,GR_BACKGROUND_TOPLEFT);
    GrDrawImageToFit(id,gc,x,y,BmpWidth,BmpHeight,ImageId);
    GrFreeImage(ImageId);
    free(BmpFileBuf);
    return 0;
}

#endif

class LedThreadApp
{
public:
    LedThreadApp();
    ~LedThreadApp();
    void init_led_sem();
    void destroy_led_sem();
    void post_led_sem();
    void cancel_led_thread();
    void create_led_thread();
private:
    //class MainViewer *m_app;
    pthread_t m_idled;
    
};

//LED STATE
#define LED_RED    1        
#define LED_GREEN  2
#define LED_YELLOW 3
#define LED_OFF    0
#define WAITTIP     0
//Tip icon coordinate info
#define DYNTIPX     270
#define DYNTIPY     500
#define DYNTIPW     60
#define DYNTIPH     60


static int g_iOpenLed;
static int g_iLedOpened;
static volatile int g_ledActive = 0;
//static int g_iProcessingTipValue;

static sem_t g_semled;
void vTellLed(void *vptr);

LedThreadApp::LedThreadApp()
{
//    m_app=ProcApp;
    
}

LedThreadApp::~LedThreadApp()
{
    
}


void LedThreadApp::init_led_sem()
{
#if LEDTHREAD==1
    if(sem_init(&g_semled, 0,1) == -1)
        exit(0);
#endif
}

void LedThreadApp::destroy_led_sem()
{
#if LEDTHREAD==1
    if(sem_destroy(&g_semled) == -1)
        exit(0);
#endif
}

void LedThreadApp::post_led_sem()
{
#if LEDTHREAD==1
    if(sem_post(&g_semled) == -1)
        exit(0);
#endif
}

void LedThreadApp::cancel_led_thread()
{
#if LEDTHREAD==1
    if(m_idled > 0){
        pthread_cancel(m_idled);
    }
#endif
}


void LedThreadApp::create_led_thread()
{
#if LEDTHREAD==1
    int ret_led;
    ret_led = pthread_create(&m_idled, NULL, (void*(*)(void*))vTellLed, NULL);
    if(ret_led != 0){
        printf("Create pthread error!\n");
        exit(1);
    }
#endif
}

void vTellLed(void *vptr)
{
    //int start = 0;
    int locked;
    g_iOpenLed = 1;
    g_iLedOpened = 0;
//    g_iProcessingTipValue = 0;
    
   //set cancel type
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    if(g_iOpenLed)
    {
        g_iLedOpened = open("/dev/s3c2410_led", O_RDONLY);
    //  g_iLedOpened = open("/dev/misc/s3c2410_led", O_RDONLY);
        g_iOpenLed = 0;
    }
    if(g_iLedOpened)
    {
        while(1)
        {   
            if(!sem_trywait(&g_semled))
            {
                //if(start)
                //    start = 0;
                //else
                //    start = 1;
            }
            else
            {
                if (g_ledActive > 0)
                {
                    ioctl(g_iLedOpened, LED_GREEN);
                    usleep(100000);
                    ioctl(g_iLedOpened, LED_OFF); 
                    usleep(100000);
                }
                else
                {
                    //g_iProcessingTipValue = 0;
                    ioctl(g_iLedOpened, LED_OFF);
                    sem_wait(&g_semled);
                    sem_post(&g_semled);
                }
            }
        }
    }
}

LedThreadApp * g_ledThread = NULL;

void initLeds()
{
    if ( !g_ledThread ) {
        g_ledThread = new LedThreadApp();
        g_ledThread->init_led_sem();
        g_ledThread->create_led_thread();
    }
}

void closeLeds()
{
    if ( g_ledThread ) {
        g_ledThread->cancel_led_thread();
        g_ledThread->destroy_led_sem();
        delete g_ledThread;
        g_ledThread = NULL;
    }
}

void postLeds( bool turnOn )
{
    if ( g_ledThread ) {
        if ( turnOn )
            g_ledActive++;
        else
            g_ledActive--;
        g_ledThread->post_led_sem();
    }
}

/// WXWidget support: draw to wxImage
class CRJinkeScreen : public CRGUIScreenBase
{
    public:
        static CRJinkeScreen * instance;
    protected:
        GR_WINDOW_ID _wid;
        GR_GC_ID _gc;
        
        virtual void update( const lvRect & rc2, bool full )
        {
        	if ( rc2.isEmpty() && !full )
        		return;
        	lvRect rc = rc2;
        	rc.left &= ~3;
        	rc.right = (rc.right + 3) & ~3;
            CRLog::debug("CRJinkeScreen::update()");
            if ( rc.height()>400 )
            	full = true;
            else
            	full = false;
			CRLog::debug("CRJinkeScreen::update( %d, %d, %d, %d, %s )", rc.left, rc.top, rc.right, rc.bottom, full ? "full" : "partial");
            //GR_BITMAP bmp;
            //GrBitmap(_wid, _gc, rc.left, rc.top, rc.width(), rc.height(), &bmp );
            int h = rc.height();

#if 0
            GR_IMAGE_HDR hdr;
            lUInt8 * buf = new lUInt8[600 * 800 * 4];
            static MWPALENTRY pal4[256] = { {0,0,0}, {0x55, 0x55, 0x55}, {0xAA, 0xAA, 0xAA}, {0xFF, 0xFF, 0xFF} };
            int palsize = 256;
            MWPALENTRY * pal = pal4;
            hdr.width = 600;          /* image width in pixels*/
            hdr.height = 800;         /* image height in pixels*/
            hdr.planes = 1;         /* # image planes*/
            hdr.bpp = 8;            /* bits per pixel (1, 4 or 8)*/
            hdr.pitch = 600;          /* bytes per line*/
            hdr.bytesperpixel = 1;  /* bytes per pixel*/
            hdr.compression = 0;    /* compression algorithm*/
            hdr.palsize = palsize;        /* palette size*/
            hdr.transcolor = -1;     /* transparent color or -1 if none*/
            hdr.palette = pal;        /* palette*/
            hdr.imagebits = (MWUCHAR*)buf;      /* image bits (dword right aligned)  MWUCHAR*/
            for ( int y=0; y<h; y++ ) {
                lUInt8 * line = _front->GetScanLine( y + rc.top );
                lUInt8 * dst = buf + y*600;
                for ( int x=0; x<600; x++ ) {
                    lUInt8 pixel = line[x>>2];
                    pixel >>= (3-(x&3))*2;
                    pixel &= 3;
                    dst[x] = pixel;
                }
            }
#endif
            CRLog::trace( "calling GrDrawImageBits wid=%08x, gc=%08x h=%d", (unsigned)_wid, (unsigned)_gc, h );
            //GrDrawImageBits(_wid,_gc, 0, rc.top, &hdr);
            //GrBitmapEx(_wid,_gc, 0, rc.top, 600, h, (GR_CHAR*)_front->GetScanLine(rc.top));
            //GR_BITMAP bmp;
            //GrBitmap(_wid, _gc, 0, rc.top, 600, h, (GR_BITMAP*)buf );
            //GrBitmap(_wid, _gc, 0, rc.top, 600, h, (GR_BITMAP*)_front->GetScanLine(0) );
            
            
            
      //      delete buf;

//            typedef struct {
//        int             width;          /* image width in pixels*/
//        int             height;         /* image height in pixels*/
//        int             planes;         /* # image planes*/
//        int             bpp;            /* bits per pixel (1, 4 or 8)*/
//        int             pitch;          /* bytes per line*/
//        int             bytesperpixel;  /* bytes per pixel*/
//        int             compression;    /* compression algorithm*/
//        int             palsize;        /* palette size*/
//        long            transcolor;     /* transparent color or -1 if none*/
//        MWPALENTRY *    palette;        /* palette*/
//        MWUCHAR *       imagebits;      /* image bits (dword right aligned)*/
//} MWIMAGEHDR, *PMWIMAGEHDR;

        #ifdef USE_OLD_NANOX
            GrBitmapEx_Apollo (_wid,_gc,0, rc.top, 600, h, 0, 0, 600, h, (GR_CHAR*)_front->GetScanLine(rc.top));
            if ( full )
                GrPrint_Apollo();
            else
                GrPartialPrint_Apollo(rc.left, rc.top, rc.width(), rc.height() );
        #else
            GrBitmapEx_Apollo_FOUR(_wid,_gc, 0, rc.top, 600, h, 0, 0, 600, h, (GR_CHAR*)_front->GetScanLine(rc.top) );
            if ( full )
                GrPrint(_wid);
            else
                GrPartialPrint(_wid, rc.left, rc.top, rc.width(), rc.height() );
        #endif
        }
    public:
        GR_WINDOW_ID getWID() { return _wid; }
        
        virtual ~CRJinkeScreen()
        {
            instance = NULL;
            GrClose();
        }
        CRJinkeScreen( int width, int height )
        :  CRGUIScreenBase( width, height, true )
        {
            if( GrOpen() < 0 ) 
            {
                fprintf(stderr, "Couldn't connect to Nano-X server\n");
                return;
            }

            GR_WM_PROPERTIES props;
            //GR_SCREEN_INFO si;
            //GrGetScreenInfo(&si);
    
        #ifdef USE_OLD_NANOX
            //GrSetBitmapExDepth_Apollo( 2 );
            _gc = GrNewGC();
            GrSetGCForeground(_gc, GR_COLOR_BLACK);
            GrSetGCBackground(_gc, GR_COLOR_WHITE);
            _wid = GrNewWindow(GR_ROOT_WINDOW_ID,VIEWER_WINDOW_X,VIEWER_WINDOW_Y,
            VIEWER_WINDOW_WIDTH,VIEWER_WINDOW_HEIGHT, 0, GR_COLOR_WHITE, 0);
            //_wid = GrNewWindow_Apollo(GR_APOLLO_ROOT_WINDOW_ID,VIEWER_WINDOW_X,VIEWER_WINDOW_Y,
            //VIEWER_WINDOW_WIDTH,VIEWER_WINDOW_HEIGHT, 0, GR_COLOR_WHITE, 0);
        #else
            _wid = GrNewWindow(GR_ROOT_WINDOW_ID,VIEWER_WINDOW_X,VIEWER_WINDOW_Y,
            VIEWER_WINDOW_WIDTH,VIEWER_WINDOW_HEIGHT, 0, GR_COLOR_WHITE, 0);
            _gc = GrNewGC();
            GrSetGCForeground(_gc, GR_COLOR_BLACK);
            GrSetGCBackground(_gc, GR_COLOR_WHITE);
        #endif
        
            GrSelectEvents(_wid, GR_EVENT_MASK_BUTTON_DOWN | \
                GR_EVENT_MASK_BUTTON_UP | GR_EVENT_MASK_MOUSE_POSITION |\
                GR_EVENT_MASK_EXPOSURE |GR_EVENT_MASK_KEY_UP|\
                GR_EVENT_MASK_KEY_DOWN | GR_EVENT_MASK_CLOSE_REQ);

            //Set Windows style
            props.flags = GR_WM_FLAGS_PROPS;
            props.props = GR_WM_PROPS_NODECORATE;
            GrSetWMProperties(_wid, &props);
        //#ifndef USE_OLD_NANOX
            GrMapWindow(_wid);    
            GrSetFocus(_wid);
        //#endif
            instance = this;
        }
};
CRJinkeScreen * CRJinkeScreen::instance = NULL;


V3DocViewWin * main_win = NULL;

class CRJinkeWindowManager : public CRGUIWindowManager
{
protected:
    GR_WINDOW_ID _wid;
public:
    /// translate string by key, return default value if not found
    virtual lString16 translateString( const char * key, const char * defValue )
    {
        CRLog::trace("Translate(%s)", key);
        lString16 res;
        //static char buf[2048];
        const char * res8 = NULL; //v3_callbacks->GetString( (char *)key );
        if ( res8 && res8[0] ) {
            CRLog::trace("   found(%s)", res8);
            res = Utf8ToUnicode( lString8(res8) );
        } else {
            CRLog::trace("   not found");
            res = Utf8ToUnicode( lString8(defValue) );
        }
        return res;
    }
    static CRJinkeWindowManager * instance;
    CRJinkeWindowManager( int dx, int dy )
    : CRGUIWindowManager(NULL)
    {
        CRJinkeScreen * s = new CRJinkeScreen( dx, dy );
        _screen = CRJinkeScreen::instance;
        if ( _screen ) {
            _wid = s->getWID();
            _ownScreen = true;
            instance = this;
        }
    }
    // runs event loop
    virtual int runEventLoop()
    {
        GR_EVENT event;
        while( !shuttingDown )
        {
            GrGetNextEvent(&event);
            switch(event.type) 
            {
                case GR_EVENT_TYPE_ERROR:
                    CRLog::debug("GR_EVENT_TYPE_ERROR");
                    break;
                case GR_EVENT_TYPE_CLOSE_REQ:
                    CRLog::debug("GR_EVENT_TYPE_CLOSE_REQ");
                    break;
                case GR_EVENT_TYPE_EXPOSURE:
                    CRLog::debug("GR_EVENT_TYPE_EXPOSURE");
/*
                    m_images->printImage("logo",0,0);
                    GrSetFontSize(m_state->fontid,32);
                    GrText(m_state->wid,m_state->gc,240,690,(char *)"Hello!\n",-1,GR_TFASCII|GR_TFTOP);
                    GrText(m_state->wid,m_state->gc,100,720,(char *)"This is only an example",-1,\
                        GR_TFASCII|GR_TFTOP);
*/
                    postLeds( true );
                    update(true);
                    if ( firstDocUpdate ) {
                        main_win->getDocView()->swapToCache();
                        firstDocUpdate = false;
                    }
                    postLeds( false );
                    break;
            case GR_EVENT_TYPE_BUTTON_DOWN:
                {
                    CRLog::debug("GR_EVENT_TYPE_BUTTON_DOWN");
/*
                char buf[128]={0};   
                GrClearArea(m_state->wid,10,770,400,28,0);
                GrSetFontSize(m_state->fontid,24);
                sprintf(buf,"mouse down: x=%d y=%d",event.mouse.x,event.mouse.y);
                GrText(m_state->wid,m_state->gc,10,770,(char *)buf,-1,GR_TFASCII|GR_TFTOP);
                GrPartialPrint(m_state->wid,10,770,400,28);  
*/
                }
                break;
            case GR_EVENT_TYPE_BUTTON_UP:
                    {
                    CRLog::debug("GR_EVENT_TYPE_BUTTON_UP");
/*
                char buf[128]={0};
                GrClearArea(m_state->wid,10,770,400,28,0);   
                GrSetFontSize(m_state->fontid,24);
                sprintf(buf,"mouse up: x=%d y=%d",event.mouse.x,event.mouse.y);
                GrText(m_state->wid,m_state->gc,10,770,(char *)buf,-1,GR_TFASCII|GR_TFTOP);
                    GrPartialPrint(m_state->wid,10,770,400,28);    
*/
                    }
                break;
            case GR_EVENT_TYPE_MOUSE_POSITION:
                    {
                    CRLog::debug("GR_EVENT_TYPE_MOUSE_POSITION");
/*
                char buf[128]={0};
                    GrClearArea(m_state->wid,10,770,400,28,0);
                GrSetFontSize(m_state->fontid,24);
                sprintf(buf,"mouse move: x=%d y=%d",event.mouse.x,event.mouse.y);
                GrText(m_state->wid,m_state->gc,10,770,(char *)buf,-1,GR_TFASCII|GR_TFTOP);
                    GrPartialPrint(m_state->wid,10,770,400,28);    

*/
                    }
                    break;
            case GR_EVENT_TYPE_KEY_DOWN:
                    CRLog::debug("GR_EVENT_TYPE_KEY_DOWN %d", (int)event.keystroke.ch );
                    {
                        static int convert_table[] = {
                        KEY_0, '0', 0,
                        KEY_1, '1', 0,
                        KEY_2, '2', 0,
                        KEY_3, '3', 0,
                        KEY_4, '4', 0,
                        KEY_5, '5', 0,
                        KEY_6, '6', 0,
                        KEY_7, '7', 0,
                        KEY_8, '8', 0,
                        KEY_9, '9', 0,
                        LONG_KEY_0, '0', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_1, '1', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_2, '2', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_3, '3', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_4, '4', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_5, '5', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_6, '6', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_7, '7', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_8, '8', KEY_FLAG_LONG_PRESS,
                        LONG_KEY_9, '9', KEY_FLAG_LONG_PRESS,
                        KEY_CANCEL, XK_Escape, 0,
                        KEY_OK, XK_Return, 0,
                        KEY_DOWN, XK_Up, 0,
                        KEY_UP, XK_Down, 0,
                        LONG_KEY_CANCEL, XK_Escape, KEY_FLAG_LONG_PRESS,
                        LONG_KEY_OK, XK_Return, KEY_FLAG_LONG_PRESS,
                        LONG_KEY_DOWN, XK_Up, KEY_FLAG_LONG_PRESS,
                        LONG_KEY_UP, XK_Down, KEY_FLAG_LONG_PRESS,
                        KEY_SHORTCUT_VOLUME_UP, XK_KP_Add, 0,
                        KEY_SHORTCUT_VOLUME_DOWN, XK_KP_Subtract, 0,
                        LONG_SHORTCUT_KEY_VOLUMN_UP, XK_KP_Add, KEY_FLAG_LONG_PRESS,
                        LONG_SHORTCUT_KEY_VOLUMN_DOWN, XK_KP_Subtract, KEY_FLAG_LONG_PRESS,
                        0, 0, 0 // end marker
                        };
                        int code = 0;
                        int flags = 0;
                        int keyId = event.keystroke.ch;
                        for ( int i=0; convert_table[i]; i+=3 ) {
                            if ( keyId==convert_table[i] ) {
                                code = convert_table[i+1];
                                flags = convert_table[i+2];
                                CRLog::debug( "OnKeyPressed( %d (%04x) ) - converted to %04x, %d", keyId, keyId, code, flags );
                            }
                        }
                        if ( !code ) {
                            CRLog::debug( "Unknown key code in OnKeyPressed() : %d (%04x)", keyId, keyId );
                            break;
                        }
                        bool needUpdate = CRJinkeWindowManager::instance->onKeyPressed( code, flags );
                        needUpdate = CRJinkeWindowManager::instance->processPostedEvents() || needUpdate;
                        if ( needUpdate ) {
                            postLeds( true );
                            CRLog::trace("Updating screen after keypress...");
                            CRJinkeWindowManager::instance->update( false );
                            postLeds( false );
                        }

                        if ( CRJinkeWindowManager::instance->getWindowCount()==0 ) {
                            shuttingDown = true;
                            // QUIT
                            CRLog::trace("windowCount==0, quitting");
                        }
                    }
                    break;
            case GR_EVENT_TYPE_FDINPUT:
                    CRLog::debug( "GR_EVENT_TYPE_FDINPUT" );
                    break;
            default:
                    break;
            }
        }
        do {
            GrPeekEvent(&event);
            if(event.type == GR_EVENT_TYPE_KEY_DOWN) {
                GrCheckNextEvent( &event );
            } else
                break;
        } while(1);
        return 1;
    }
    bool doCommand( int cmd, int params )
    {
        if ( !onCommand( cmd, params ) )
            return false;
        update( false );
        return true;
    }
};

CRJinkeWindowManager * CRJinkeWindowManager::instance = NULL;

class CRJinkeDocView : public V3DocViewWin {
public:
    static CRJinkeDocView * instance;
    CRJinkeDocView( CRGUIWindowManager * wm, lString16 dataDir )
    : V3DocViewWin( wm, dataDir )
    {
        instance = this;
    }
    virtual void closing()
    {
        strcpy( last_bookmark, GetCurrentPositionBookmark() );
        last_bookmark_page = CRJinkeDocView::instance->getDocView()->getCurPage();
        V3DocViewWin::closing();
    }
    virtual ~CRJinkeDocView()
    {
        instance = NULL;
    }
};
CRJinkeDocView * CRJinkeDocView::instance = NULL;


// some prototypes
//int InitDoc(char *fileName);


static int g_QuitSignalCounter = 0;

void QuitSignalCount(int sig)
{
    g_QuitSignalCounter++;
}
//wait for child exit and return back quckly
void WaitSignalChildExit(int sig)
{
    waitpid(0,0,WNOHANG);
}

void DoQuitSignal(int sig)
{
    exit(0);
}

void ExceptionExit(int sig)
{
    closeLeds();
    printf("ExceptionExit(%d)", sig);
    GrClose();
    exit(0);
}


int main( int argc, const char * argv[] )
{
    //g_ledThread = new 

    if ( argc<2 ) {
        printf("usage: cr3 <filename>\n");
        return 1;
    }

    signal(SIGINT,QuitSignalCount);
    signal(SIGTERM,QuitSignalCount);

    initLeds();
    //signal(SIGCHLD,WaitSignalChildExit);

    {
        postLeds( true );
        int res = InitDoc( (char *)argv[1] );

        if ( !res ) {
            printf("Failed to show file %s\n", argv[1]);
            closeLeds();
            return 2;
        }
        postLeds( false );
    }

   if(g_QuitSignalCounter)
   {
      g_QuitSignalCounter=0;
      GrClose();
      printf("INT signal \n");
      closeLeds();
      return 0;
   }

   signal(SIGINT,ExceptionExit);
   signal(SIGTERM,ExceptionExit);

    CRLog::info("Entering event loop");
    CRJinkeWindowManager::instance->runEventLoop();
    CRLog::info("Exiting event loop");

    closeLeds();

    return 0;
}


static char history_file_name[1024] = "/root/abook/.cr3hist";

int InitDoc(char *fileName)
{


    static const lChar16 * css_file_name = L"fb2.css"; // fb2

    CRLog::trace("InitDoc()");
#ifdef __i386__
    //CRLog::setFileLogger("/root/abook/crengine.log");
    CRLog::setStdoutLogger();
    CRLog::setLogLevel(CRLog::LL_TRACE);
#else
    //InitCREngineLog(NULL);
    InitCREngineLog("/root/abook/crengine/crlog.ini");
#endif

    lString16 bookmarkDir("/root/abook/bookmarks/");
    {
        lString8 fn(fileName);
        if ( fn.startsWith(lString8("/home")) ) {
            strcpy( history_file_name, "/home/.cr3hist" );
            bookmarkDir = lString16("/home/bookmarks/");
        }
        CRLog::info( "History file name: %s", history_file_name );
    }

    char manual_file[512] = "";
    {
        const char * lang = "ru"; //  TODO: get fro WOLLANG : v3_callbacks->GetString( "CR3_LANG" );
        if ( lang && lang[0] ) {
            // set translator
            CRLog::info("Current language is %s, looking for translation file", lang);
            lString16 mofilename = L"/root/crengine/i18n/" + lString16(lang) + L".mo";
            CRMoFileTranslator * t = new CRMoFileTranslator();
            if ( t->openMoFile( mofilename ) ) {
                CRLog::info("translation file %s.mo found", lang);
                CRI18NTranslator::setTranslator( t );
            } else {
                CRLog::info("translation file %s.mo not found", lang);
                delete t;
            }
            sprintf( manual_file, "/root/crengine/manual/cr3-manual-%s.fb2", lang );
        }
    }

    const lChar16 * ini_fname = L"cr3.ini";
#ifdef SEPARATE_INI_FILES
    if ( strstr(fileName, ".txt")!=NULL || strstr(fileName, ".tcr")!=NULL) {
        ini_fname = L"cr3-txt.ini";
        css_file_name = L"txt.css";
    } else if ( strstr(fileName, ".rtf")!=NULL ) {
        ini_fname = L"cr3-rtf.ini";
        css_file_name = L"rtf.css";
    } else if ( strstr(fileName, ".htm")!=NULL ) {
        ini_fname = L"cr3-htm.ini";
        css_file_name = L"htm.css";
    } else if ( strstr(fileName, ".epub")!=NULL ) {
        ini_fname = L"cr3-epub.ini";
        css_file_name = L"epub.css";
    } else {
        ini_fname = L"cr3-fb2.ini";
        css_file_name = L"fb2.css";
    }
#endif

    lString16Collection fontDirs;
    fontDirs.add( lString16(L"/root/abook/fonts") );
    fontDirs.add( lString16(L"/home/fonts") );
    //fontDirs.add( lString16(L"/root/crengine/fonts") ); // will be added
    CRLog::info("INIT...");
    if ( !InitCREngine( "/root/crengine/", fontDirs ) )
        return 0;



#ifdef ALLOW_RUN_EXE
    {
        __pid_t pid;
        if( strstr(fileName, ".exe.txt") || strstr(fileName, ".exe.fb2")) {
            pid = fork();
            if(!pid) {
                execve(fileName, NULL, NULL);
                exit(0);
            } else {
                waitpid(pid, NULL, 0);
                exit(0);
                //return 0;
            }
        }
    }
#endif
    {
        CRLog::trace("creating window manager...");
        CRJinkeWindowManager * wm = new CRJinkeWindowManager(600,800);
        //main_win = new V3DocViewWin( wm, lString16(CRSKIN) );

        const char * keymap_locations [] = {
            "/root/crengine/",
            "/home/crengine/",
            "/root/abook/crengine/",
            NULL,
        };
        loadKeymaps( *wm, keymap_locations );
        HyphMan::initDictionaries( lString16("/root/crengine/hyph") );

        if ( !wm->loadSkin(  lString16( L"/root/abook/crengine/skin" ) ) )
            if ( !wm->loadSkin(  lString16( L"/home/crengine/skin" ) ) )
                wm->loadSkin( lString16( L"/root/crengine/skin" ) );

        ldomDocCache::init( lString16(L"/root/abook/crengine/.cache"), 0x100000 * 64 ); /*96Mb*/

        CRLog::trace("creating main window...");
        main_win = new CRJinkeDocView( wm, lString16(L"/root/crengine") );
        CRLog::trace("setting colors...");
        main_win->getDocView()->setBackgroundColor(0xFFFFFF);
        main_win->getDocView()->setTextColor(0x000000);
        main_win->getDocView()->setFontSize( 20 );
        if ( manual_file[0] )
            main_win->setHelpFile( lString16( manual_file ) );
        if ( !main_win->loadDefaultCover( lString16( L"/root/abook/crengine/cr3_def_cover.png" ) ) )
            if ( !main_win->loadDefaultCover( lString16( L"/home/crengine/cr3_def_cover.png" ) ) )
                main_win->loadDefaultCover( lString16( L"/root/crengine/cr3_def_cover.png" ) );
        if ( !main_win->loadCSS(  lString16( L"/root/abook/crengine/" ) + lString16(css_file_name) ) )
            if ( !main_win->loadCSS(  lString16( L"/home/crengine/" ) + lString16(css_file_name) ) )
                main_win->loadCSS( lString16( L"/root/crengine/" ) + lString16(css_file_name) );
        main_win->setBookmarkDir( bookmarkDir );
        CRLog::trace("choosing init file...");
        static const lChar16 * dirs[] = {
            L"/root/abook/crengine/",
            L"/home/crengine/",
            L"/root/appdata/",
            NULL
        };
        int i;
        CRLog::debug("Loading settings...");
        lString16 ini;
        for ( i=0; dirs[i]; i++ ) {
            ini = lString16(dirs[i]) + ini_fname;
            if ( main_win->loadSettings( ini ) ) {
                break;
            }
        }
        CRLog::debug("settings at %s", UnicodeToUtf8(ini).c_str() );
#if USE_JINKE_USER_DATA!=1
    if ( !main_win->loadHistory( lString16(history_file_name) ) ) {
        CRLog::error("Cannot read history file %s", history_file_name);
    }
#endif

        LVDocView * _docview = main_win->getDocView();
        _docview->setBatteryState( ::getBatteryState() );
        wm->activateWindow( main_win );
        if ( !main_win->loadDocument( lString16(fileName) ) ) {
            printf("Cannot open book file %s\n", fileName);
            delete wm;
            return 0;
        } else {
            postLeds( true );
     #ifdef USE_OLD_NANOX
                    wm->update(true);
                    if ( firstDocUpdate ) {
                        main_win->getDocView()->swapToCache();
                        firstDocUpdate = false;
                    }
                    postLeds( false );
     #endif
        }
    }

    //_docview->setVisiblePageCount( 1 );



    //tocDebugDump( _docview->getToc() );

    return 1;
}

const char * GetCurrentPositionBookmark()
{
    if ( !CRJinkeDocView::instance )
        return last_bookmark;
    CRLog::trace("GetCurrentPositionBookmark() - returning empty string");
    //ldomXPointer ptr = main_win->getDocView()->getBookmark();
    //lString16 bmtext( !ptr ? L"" : ptr.toString() );
    static char buf[1024];
    //strcpy( buf, UnicodeToUtf8( bmtext ).c_str() );
    strcpy( buf, "" );
    CRLog::trace("   return bookmark=%s", buf);
    return buf;
}

