#include "G711Codec.h"

extern int linear2ulaw(int pcm_val);
extern int ulaw2linear(int u_val);
extern int linear2alaw(int pcm_val);
extern int alaw2linear(int u_val);


CG711Codec::CG711Codec(void)
{
}

CG711Codec::~CG711Codec(void)
{
}

bool CG711Codec::Initial(TVT_WAVEFORMATEX *pWfx)
{
	if(pWfx->wFormatTag == WAVE_FORMAT_ALAW)
	{
		m_decoder = alaw2linear;
		m_encoder = linear2alaw;
	}
	else if(pWfx->wFormatTag == WAVE_FORMAT_MULAW)
	{
		m_decoder = ulaw2linear;
		m_encoder = linear2ulaw;
	}
	else
	{
		return false;
	}

	return true;
}

void CG711Codec::Quit()
{

}

bool CG711Codec::DecodeBuf(unsigned char *pInData, unsigned long inLen, unsigned char *pOutData, unsigned long *pOutLen)
{
	long sampleNum = inLen;
	unsigned short * pOut = (unsigned short *)(pOutData);

	for(long i = 0; i < sampleNum; i++)
	{
		*pOut++ = m_decoder(pInData[i]);
	}

	*pOutLen = inLen * 2;

	return true;
}

bool CG711Codec::EncodeBuf(unsigned char *pInData, unsigned long inLen, unsigned char *pOutData, unsigned long *pOutLen)
{
	long sampleNum = inLen/2;
	unsigned short * pIn = (unsigned short *)(pInData);

	for(long i = 0; i < sampleNum; i++)
	{
		pOutData[i] = m_encoder(*pIn++);
	}

	*pOutLen = sampleNum;

	return true;
}

