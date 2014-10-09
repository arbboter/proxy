#ifndef _G711_CODEC_H_
#define _G711_CODEC_H_

typedef unsigned short      WORD;
typedef unsigned long       DWORD;
typedef struct typeWAVEFORMATEX
{
	WORD        wFormatTag;         /* format type */
	WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
	DWORD       nSamplesPerSec;     /* sample rate */
	DWORD       nAvgBytesPerSec;    /* for buffer estimation */
	WORD        nBlockAlign;        /* block size of data */
	WORD        wBitsPerSample;     /* number of bits per sample of mono data */
	WORD        cbSize;             /* the count in bytes of the size of */
	/* extra information (after cbSize) */
} TVT_WAVEFORMATEX;

#define  WAVE_FORMAT_ALAW   0x0006
#define  WAVE_FORMAT_MULAW  0x0007

typedef int (*G711_Decoder)(int sample);
typedef int (*G711_Encoder)(int sample);


class CG711Codec
{
public:
	CG711Codec(void);
	~CG711Codec(void);

	bool Initial(TVT_WAVEFORMATEX *pWfx);
	void Quit();

	bool DecodeBuf(unsigned char *pInData, unsigned long inLen, unsigned char *pOutData, unsigned long *pOutLen);
	bool EncodeBuf(unsigned char *pInData, unsigned long inLen, unsigned char *pOutData, unsigned long *pOutLen);

private:
	G711_Decoder	m_decoder;
	G711_Encoder	m_encoder;
};

#endif
