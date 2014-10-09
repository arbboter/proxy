#include "aacEncode.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <iostream>

const int AUDIO_CHANNLS = 1;
const int SAMPLES_PER_SEC = 8000;
const int BITS_PER_SAMPLE = 16;
const float SAMPLES_PER_MILLION_SEC = SAMPLES_PER_SEC/1000;

const int RE_CALC_TIME = 60*60*1000;

CAacEncoder::CAacEncoder()
{

}
CAacEncoder::~CAacEncoder()
{

}

bool CAacEncoder::Init()
{
	m_totalSamples = 0;

	m_bGetTime = false;

	TVT_WAVEFORMATEX wmformat; 

    wmformat.nAvgBytesPerSec = SAMPLES_PER_SEC*BITS_PER_SAMPLE/8;
    wmformat.nBlockAlign = 2;
    wmformat.nChannels = AUDIO_CHANNLS;
    wmformat.nSamplesPerSec = SAMPLES_PER_SEC;
    wmformat.wBitsPerSample = BITS_PER_SAMPLE;
    wmformat.wFormatTag =WAVE_FORMAT_ALAW;
    faacEncConfigurationPtr format = NULL;

	if(!m_g711decode.Initial(&wmformat))
	{
		goto AAC_ENCODE_ERROR;
	}

    //³õÊ¼»¯aac±àÂëÆ÷
    m_encodeHandler = faacEncOpen(wmformat.nSamplesPerSec, wmformat.nChannels, &m_inputSamples, &m_maxOutputBytes);
	

	if(NULL == m_encodeHandler)
	{
		goto AAC_ENCODE_ERROR;
	}

	format = faacEncGetCurrentConfiguration(m_encodeHandler);
	format->outputFormat = 1;
	faacEncSetConfiguration(m_encodeHandler,format);


	m_encodeUnitLen = wmformat.wBitsPerSample/8*m_inputSamples;

    //³õÊ¼»¯±àÂë×ª»»»º´æ
	m_pcmDataLen = 0;
    m_PcmOutBuf = new unsigned char[m_encodeUnitLen*2];
    if(NULL == m_PcmOutBuf)
    {
        goto AAC_ENCODE_ERROR;
    }

    m_pAacOutBuff = new unsigned char[m_maxOutputBytes + sizeof(TVT_DATA_FRAME)];
    if(NULL == m_pAacOutBuff)
    {
        goto AAC_ENCODE_ERROR;
    }

    return true;

AAC_ENCODE_ERROR:

	Quit();

	return false;
}

void CAacEncoder::Quit()
{
	m_totalSamples = 0;
	m_bGetTime = false;

	m_g711decode.Quit();

	if (NULL != m_encodeHandler)
	{
		faacEncClose(m_encodeHandler);
		m_encodeHandler = NULL;
	}

	if (NULL != m_PcmOutBuf)
	{
		delete []  m_PcmOutBuf;
		m_PcmOutBuf = NULL;
	}

	if (NULL != m_pAacOutBuff)
	{
		delete [] m_pAacOutBuff;
		m_pAacOutBuff = NULL;
	}
}

bool CAacEncoder::AudioFrameToAac(TVT_DATA_FRAME *pAudInFrame, TVT_DATA_FRAME **ppAudOutFrame)
{
	bool ret = false;

    assert(pAudInFrame);
    assert(ppAudOutFrame);
	
	unsigned long tmpPcmDataLen = 0;
	if (m_g711decode.DecodeBuf(pAudInFrame->pData + sizeof(TVT_DATA_FRAME),pAudInFrame->frame.length,m_PcmOutBuf+m_pcmDataLen,&tmpPcmDataLen))
	{
		m_pcmDataLen += tmpPcmDataLen;
	}

	
	if (!m_bGetTime)
	{
		m_totalSamples = 0;
		m_dateTime = pAudInFrame->frame.time;
		m_bGetTime = true;
	}

	if (m_pcmDataLen < m_encodeUnitLen)
	{
		return ret;
	}

	int dataLen = faacEncEncode(m_encodeHandler, (int*)m_PcmOutBuf, m_inputSamples, m_pAacOutBuff+sizeof(TVT_DATA_FRAME),m_maxOutputBytes);
	int frameLen = sizeof(TVT_DATA_FRAME) + (dataLen +0x03)&~(0x03);

	if (dataLen > 0)
	{
		TVT_DATA_FRAME *pFrame =(TVT_DATA_FRAME*)m_pAacOutBuff;
		
		assert(NULL != pFrame);
		memset(pFrame,0,sizeof(TVT_DATA_FRAME));

		int millonsecs = m_totalSamples/SAMPLES_PER_MILLION_SEC;
		if (millonsecs > RE_CALC_TIME)
		{
			m_bGetTime = false;
		}
		m_dateTime += millonsecs;

		pFrame->frame.time		   =  m_dateTime;
		pFrame->frame.localTime    =  m_dateTime.LocalTime();
		pFrame->frame.bKeyFrame    =  pAudInFrame->frame.bKeyFrame;
		pFrame->frame.streamType   =  pAudInFrame->frame.streamType;
		pFrame->frame.channel      =  pAudInFrame->frame.channel;
		pFrame->frame.length       =  dataLen;
		pFrame->freameID           =  pAudInFrame->freameID;
		pFrame->frame.time         =  pAudInFrame->frame.time;
		pFrame->length			   =  frameLen;
		pFrame->pData			   =  m_pAacOutBuff;
		
		*ppAudOutFrame = pFrame;

		m_totalSamples += m_inputSamples;

		ret  = true;
	}

	m_pcmDataLen = m_pcmDataLen - m_encodeUnitLen;
	if (m_pcmDataLen > 0)
	{
		memmove(m_PcmOutBuf,m_PcmOutBuf + m_encodeUnitLen,m_pcmDataLen);
	}

	return ret;
}
