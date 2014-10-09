#ifndef __TS_MUXER_H__
#define __TS_MUXER_H__

#include "TVT_PubDefine.h"
#include "DataBufferEx.h"
#include <vector>
#include <list>
#include <queue>

struct TS_OPTION
{
    int                     bit_rate;

    struct stream
    {
        char    type;
        int     fps_or_bps;
    };

    struct program
    {
        std::vector<stream> streams;
    };

    std::vector<program>    programs;

    void    SetBitRate(int rate);
    void    SetStreamFPSOrBPS(int program, int stream, int val);
    int     AddStream(int program, char type);
    int     AddProgram();
};

class CTSMuxer
{
    struct PAYLOAD
    {
        char *          buf;
        int             len;
        TVT_DATE_TIME   t;
    };

    const static int MIN_PLAYLOAD_NUM = 16;
    const static int MIN_PAYLOAD_SIZE = (MIN_PLAYLOAD_NUM - 1) * 184 + 170;

public:
    enum 
    {
        TS_MUXER_OK,
        TS_MUXER_PARAM_ERROR,
        TS_MUXER_NO_BUFFER,
        TS_MUXER_NO_OUTPUT,
    };

    CTSMuxer(void);
    ~CTSMuxer(void);
    
    void    Init(TS_OPTION & option);
    void    Quit();

    void    AllocBuffer(int size);
    void    DestroyBuffer();

    int     Put(int program, int stream, TVT_DATA_FRAME * frame);
    
    int     Get(char ** buf, int * len);
    void    Free(char * buf, int len);

    void    Flush();

private:
    TS_OPTION                                       m_option;
    CDataBufferEx *                                 m_pBuffer;
    void *                                          m_ts;
    std::queue<std::pair<char *, int> >             m_out;
    std::vector<std::vector<unsigned long long> >   m_pts;
    std::vector<std::vector<TVT_DATE_TIME> >        m_prevt;
    std::vector<std::vector<PAYLOAD> >              m_payloadBuf;
};

#endif
