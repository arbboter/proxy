
#ifndef __BASE_64_H__
#define __BASE_64_H__

class CBase64
{
public:
	CBase64(void);
	~CBase64(void);

	void SetLineMaxLen(long lineMaxLen);

	long Encode(const unsigned char *pSrc, long srcLen, char *pDest, long destLen);
	long Decode(const char *pSrc, long srcLen, unsigned char *pDest, long destLen);

private:
	long			m_lineMaxLen;
};

#endif
