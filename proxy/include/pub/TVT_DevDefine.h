#ifndef __TVT_DEV_DEFINE_H__
#define __TVT_DEV_DEFINE_H__


#ifdef WIN32
#include <io.h>
#include <winsock2.h>
#include "windows.h"
#include "time.h"
#include "winbase.h"
#include <ws2tcpip.h>
#else //////Linux下面
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <pthread.h> 
#include <time.h> 
#include <signal.h>
#include <fcntl.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#endif

#pragma pack(4)

typedef enum _tvt_video_format_
{
    TVT_VIDEO_FORMAT_NTSC		= 0x01,
    TVT_VIDEO_FORMAT_PAL		= 0x02,
}TVT_VIDEO_FORMAT;

typedef enum _tvt_encode_type_
{
    TVT_ENCODE_TYPE_CBR,
    TVT_ENCODE_TYPE_VBR
}TVT_ENCODE_TYPE;

typedef enum   __tvt_encode_format__type__
{
   TVT_VIDEO_ENCODE_TYPE_H264  = 0,
   TVT_VIDEO_ENCODE_TYPE_MPEG4 = 1,
   TVT_AUDIO_ENCODE_TYPE_RAW   = 2,
   TVT_AUDIO_ENCODE_TYPE_G711A = 3,
   TVT_AUDIO_ENCODE_TYPE_G711U = 4,
   TVT_AUDIO_ENCODE_TYPE_ADPCM = 5,
   TVT_AUDIO_ENCODE_TYPE_G726  = 6,
   TVT_AUDIO_ENCODE_TYPE_AAC   = 7,
   TVT_AUDIO_ENCODE_TYPE_LPCM  = 8,
   TVT_PIC_ENCODE_TYPE_JPEG    = 9,
}TVT_ENCODE_FORMAT_TYPE;

typedef enum _tvt_stream_type_
{
    TVT_STREAM_TYPE_NONE		    = 0x00,

    TVT_STREAM_TYPE_VIDEO_1		= 0x01,			//视频1码流（对应主码流）
    TVT_STREAM_TYPE_VIDEO_2		= 0x02,			//视频2码流（对应子码流）
    TVT_STREAM_TYPE_VIDEO_3		= 0x03,			//视频3码流（备用）
    TVT_STREAM_TYPE_AUDIO		    = 0x04,			//音频
    TVT_STREAM_TYPE_PICTURE		= 0x05,			//图片
    TVT_STREAM_TYPE_TEXT		    = 0x06,			//文本数据
    TVT_FRAME_TYPE_BEGIN,
    TVT_FRAME_TYPE_END
}TVT_STREAM_TYPE;


//按位保存，最多不能超过32个。
typedef enum _tvt_video_size_
{
    TVT_VIDEO_SIZE_QCIF			= 0x0001,	//(NTSC 176 X 120),(PAL 176 X 144)
    TVT_VIDEO_SIZE_CIF			= 0x0002,	//(NTSC 352X 240),(PAL 352 X 288)
    TVT_VIDEO_SIZE_2CIF			= 0x0004,	//(NTSC 704 X 240),(PAL 704 X 288)
    TVT_VIDEO_SIZE_4CIF			= 0x0008,	//(NTSC 704 X 480),(PAL 704 X 576)

    TVT_VIDEO_SIZE_QVGA			= 0x0010,	//(320 X 240)
    TVT_VIDEO_SIZE_VGA			= 0x0020,	//(640 X 480)
    TVT_VIDEO_SIZE_960H			= 0x0040,	//(NTSC 960 X 480),(PAL 960 X 576)
    TVT_VIDEO_SIZE_SVGA			= 0x0080,	//(800 X 600)

    TVT_VIDEO_SIZE_XGA			= 0x0100,	//(1024 X 768)
    TVT_VIDEO_SIZE_XVGA			= 0x0200,	//(1280 X 960)
    TVT_VIDEO_SIZE_SXGA			= 0x0400,	//(1280 X 1024)
    TVT_VIDEO_SIZE_UXGA			= 0x0800,	//(1600 X 1200)

    TVT_VIDEO_SIZE_720P			= 0x1000,	//(1280 X 720)
    TVT_VIDEO_SIZE_1080P		= 0x2000,	//(1920 X 1080)
    TVT_VIDEO_SIZE_QXGA			= 0x4000,	//(2048 X 1536)
    TVT_VIDEO_SIZE_4K			= 0x8000,	//(3840 x 2160)
}TVT_VIDEO_SIZE;


/*此枚举的值用在了一个unsigned short类型上，所以最多只能定义16个*/
typedef enum _tvt_white_balance_
{
    TVT_WHITE_BALANCE_AUTO    = 0x0001,		//自动
    TVT_WHITE_BALANCE_MANUAL  = 0x0002,		//手动
    TVT_WHITE_BALANCE_LAMP    = 0x0004,		//白炽灯
    TVT_WHITE_BALANCE_OUTDOOR = 0x0008,		//户外
    TVT_WHITE_BALANCE_INDOOR  = 0x0010,		//室内
    TVT_WHITE_BALANCE_CLOUDY  = 0x0020,		//多云
    TVT_WHITE_BALANCE_SUNNY   = 0x0040,		//晴天
}TVT_WHITE_BALANCE;

typedef enum _tvt_net_stream_mode_
{
    TVT_NET_STREAM_SYSTEM,		//系统默认
    TVT_NET_STREAM_SHARPNESS,	//清晰度
    TVT_NET_STREAM_FLUENCY,		//流畅性
}TVT_NET_STREAM_MODE;

typedef enum  _tvt_osd_pos_
{
    TVT_OSD_LEFT_UP,            //左上
    TVT_OSD_LEFT_DWON,          //左下
    TVT_OSD_RIGHT_UP,           //右上
    TVT_OSD_RIGHT_DOWN,         //右下
}TVT_OSD_POS;

typedef struct _tvt_block_
{
    unsigned char	*pBuffer;			//块对应的内存地址*/
    unsigned int	bufLength;			//当前块的长度*/

    unsigned int	channel;			 //块对应的通道号*/
    unsigned int	streamType;			//对应的数据流的类型*/
    unsigned int	dataLength;			//存放的数据长度*/
}TVT_BLOCK;

typedef struct _tvt_local_time_
{
    short	year;			//年
    short	month;			//月
    short	mday;			//某月第几天

    short	hour;			//时
    short	minute;			//分
    short	second;			//秒

    int	microsecond;	//微秒
}TVT_LOCAL_TIME;

typedef struct _tvt_date_time_
{
    int	seconds;		//总秒数
    int	microsecond;	//微秒

#ifdef   __cplusplus
    _tvt_date_time_();
    _tvt_date_time_ & operator=(tm & time);
    _tvt_date_time_ & operator=(const timeval & time);
    _tvt_date_time_ & operator=(const _tvt_local_time_ & time);
    tm TM() const;
    timeval TimeVal() const;
    _tvt_local_time_ LocalTime() const;

    _tvt_date_time_ & operator+=(int usec);
    _tvt_date_time_ & operator-=(int usec);

    long long Dec(const _tvt_date_time_ & time);
    int operator-(const _tvt_date_time_ & time);

    bool operator!=(const _tvt_date_time_ & time) const;
    bool operator==(const _tvt_date_time_ & time) const;
    bool operator<(const _tvt_date_time_ & time) const;
    bool operator<=(const _tvt_date_time_ & time) const;
    bool operator>(const _tvt_date_time_ & time) const;
    bool operator>=(const _tvt_date_time_ & time) const;
#endif  //__cplusplus
}TVT_DATE_TIME;

typedef struct _tvt_data_frame_
{
    union
    {
        unsigned int   position;	//在缓冲池中的位置
        unsigned char   *pData;		//数据区指针（TVT_DATA_FRAME + DATA）
    };
    unsigned int   length;			//缓冲区大小（sizeof(TVT_DATA_FRAME) + frame.length）

    unsigned int	deviceID;		//设备编号
    unsigned int   cameraID;		//摄像机编号

    unsigned int	freameID;		//帧编号
    unsigned int	streamID;		//流编号

    struct  
    {
        unsigned short		width;		//宽度
        unsigned short		height;		//高度

        unsigned short		bKeyFrame;	//是否为关键帧
        unsigned short		encodeType;	//编码类型

        unsigned int		length;		//数据长度
        unsigned int		streamType;	//流类型，见TVT_STREAM_TYPE

        unsigned int		channel;	//通道编号

        TVT_LOCAL_TIME		localTime;	//格式化时间
        TVT_DATE_TIME		time;		//标准时间
    }frame;

    unsigned int	checkBits;      //校验位

#ifdef __cplusplus
    bool operator!=(const _tvt_data_frame_ & frame) const;
    bool operator==(const _tvt_data_frame_ & frame) const;
#endif //__cplusplus	
}TVT_DATA_FRAME;


#endif