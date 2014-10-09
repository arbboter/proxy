#ifndef  __AAC_ENCODER__
#define  __AAC_ENCODER__
#include "faac.h"
#include "TVT_DevDefine.h"
#include "G711Codec.h"

class  CAacEncoder
{
public:
    CAacEncoder();
    ~CAacEncoder();

    bool Init();
    void Quit();

    bool AudioFrameToAac(TVT_DATA_FRAME *pAudInFrame, TVT_DATA_FRAME **ppAudOutFrame);

private:
	unsigned long           m_maxOutputBytes;	//aac编码器最大输出字节数
	unsigned long           m_inputSamples;		//aac编码器最大输入样本数

	unsigned char*          m_PcmOutBuf;		//编码转换缓存
    unsigned long           m_pcmDataLen;		//缓存中的数据量

    faacEncHandle           m_encodeHandler;	//aac编码器句柄
	unsigned long			m_encodeUnitLen;	//aac解码块大小
	unsigned char*          m_pOutBuff;			//g711解码输出缓存
    unsigned char*          m_pAacOutBuff;		//aac编码输出缓存

	TVT_DATE_TIME			m_dateTime;		//保存当前帧的时间
	bool					m_bGetTime;		//时候已经开始保存

	long long				m_totalSamples; //当前已经获取的音频采样数
	CG711Codec				m_g711decode;	//解码g711;

};
#endif