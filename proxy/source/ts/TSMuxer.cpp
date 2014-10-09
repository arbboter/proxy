#include "TSMuxer.h"
#include "TSStream.h"

//////////////////////////////////////////////////////////////////////////

TS_STREAM * alloc_ts(const TS_OPTION & option)
{
    TS_STREAM * ts = new TS_STREAM;
    memset(ts, 0x0, sizeof(TS_STREAM));

    int pid = 0x0100;

    ts->version = 0;
    ts->bit_rate = option.bit_rate;
    ts->pat_period = (ts->bit_rate * PAT_RETRANS_TIME) / (TS_PACKET_SIZE * 8 * 1000);

    ts->pat.cc = 15;
    ts->pat.pid = 0x0000;

    ts->program_num = option.programs.size();
    assert(ts->program_num > 0);
    ts->program = new TS_PROGRAM *[ts->program_num];
    for (int i = 0; i < ts->program_num; i++)
    {
        ts->program[i] = new TS_PROGRAM;
        ts->program[i]->pmt.pid = pid++;
        ts->program[i]->pmt.cc = 15;
        ts->program[i]->sid = 0x1;
        ts->program[i]->pcr_pid = 0x1fff;
        ts->program[i]->pcr_period = (ts->bit_rate * PCR_RETRANS_TIME) / (TS_PACKET_SIZE * 8 * 1000);
        ts->program[i]->pcr_packet_count = ts->program[i]->pcr_period - 1;

        ts->program[i]->stream_num = option.programs[i].streams.size();
        assert(ts->program[i]->stream_num > 0);
        ts->program[i]->stream = new ELEM_STREAM *[ts->program[i]->stream_num];
        for (int j = 0; j < ts->program[i]->stream_num; j++)
        {
            ts->program[i]->stream[j] = new ELEM_STREAM;
            ts->program[i]->stream[j]->type = option.programs[i].streams[j].type;
            ts->program[i]->stream[j]->pid = pid++;
            ts->program[i]->stream[j]->cc = 15;
            if (ts->program[i]->stream[j]->type == 'v' && ts->program[i]->pcr_pid == 0x1fff)
            {
                ts->program[i]->pcr_pid = ts->program[i]->stream[j]->pid;
            }
        }
    }

    return ts;
}

void free_ts(TS_STREAM * ts)
{
    for (int i = 0; i < ts->program_num; i++)
    {
        for (int j = 0; j < ts->program[i]->stream_num; j++)
        {
            delete ts->program[i]->stream[j];
        }

        delete [] ts->program[i]->stream;

        delete ts->program[i];
    }

    delete [] ts->program;

    delete ts;
}

int estimate_ts_pack_size(TS_STREAM * ts, int program_index, int stream_index, int len)
{
    int n = (len + TS_PACKET_SIZE - 1) / TS_PACKET_SIZE;
    int pat_num = (n + ts->pat_period - 1) / ts->pat_period;
    int pmt_num = n * ts->program_num;
    int pcr_num = (n + ts->program[program_index]->pcr_period - 1) / ts->program[program_index]->pcr_period;

    return (n + pat_num + pmt_num + pcr_num) * TS_PACKET_SIZE;
}

//////////////////////////////////////////////////////////////////////////

CTSMuxer::CTSMuxer(void) : 
    m_pBuffer(NULL),
    m_ts(NULL)
{

}


CTSMuxer::~CTSMuxer(void)
{
    if (m_pBuffer)
    {
        DestroyBuffer();
    }
}

void CTSMuxer::Init( TS_OPTION & option )
{
    m_option = option;

    m_ts = alloc_ts(option);
    
    m_pts.resize(option.programs.size());
    m_prevt.resize(option.programs.size());
    m_payloadBuf.resize(option.programs.size());

    for (size_t i = 0; i < option.programs.size(); i++)
    {
        m_pts[i].resize(option.programs[i].streams.size());
        m_prevt[i].resize(option.programs[i].streams.size());
        m_payloadBuf[i].resize(option.programs[i].streams.size());
        
        for (size_t j = 0; j < option.programs[i].streams.size(); j++)
        {
            if (option.programs[i].streams[j].type == 'a')
            {
                m_payloadBuf[i][j].buf = new char[MIN_PAYLOAD_SIZE];
                m_payloadBuf[i][j].len = 0;
            }
            else
            {
                m_payloadBuf[i][j].buf = NULL;
                m_payloadBuf[i][j].len = 0;
            }
        }
    }
}

void CTSMuxer::Quit()
{
    free_ts((TS_STREAM *)m_ts);
    m_ts = NULL;

    for (size_t i = 0; i < m_option.programs.size(); i++)
    {
        for (size_t j = 0; j < m_option.programs[i].streams.size(); j++)
        {
            if(m_payloadBuf[i][j].buf)
            {
                delete [] m_payloadBuf[i][j].buf;
            }
        }
    }
}

void CTSMuxer::AllocBuffer( int size )
{
    m_pBuffer = new CDataBufferEx(size);
}

void CTSMuxer::DestroyBuffer()
{
    delete m_pBuffer;
    m_pBuffer = NULL;
}

#define RESET_PTS_POLE  prev_time = frame->frame.time;\
                        unsigned long long tmp = (unsigned long long)prev_time.seconds * 1000000 + (unsigned long long)prev_time.microsecond;\
                        last_pts = (tmp * 90000) / 1000000

int CTSMuxer::Put( int program, int stream, TVT_DATA_FRAME * frame )
{
    TS_STREAM *ts = (TS_STREAM *)m_ts;
    if (program >= ts->program_num || program < 0)
    {
        return TS_MUXER_PARAM_ERROR;
    }
    
    if (stream >= ts->program[program]->stream_num || stream < 0)
    {
        return TS_MUXER_PARAM_ERROR;
    }

    if (!frame)
    {
        return TS_MUXER_PARAM_ERROR;
    }

ProcesFrame:

    char * frame_data = (char *)frame->pData + sizeof(TVT_DATA_FRAME);
    TVT_DATE_TIME frame_time = frame->frame.time;
    char *pData = frame_data;
    int datalen = frame->frame.length;
    char frame_type;

    //////////////////////////////////////////////////////////////////////////

    if(frame->frame.streamType == TVT_STREAM_TYPE_VIDEO_1 || 
        frame->frame.streamType == TVT_STREAM_TYPE_VIDEO_2 ||
        frame->frame.streamType == TVT_STREAM_TYPE_VIDEO_3 )
    {
        frame_type = 'v';
    }
    else if (frame->frame.streamType == TVT_STREAM_TYPE_AUDIO)
    {
        frame_type = 'a';

        char * payload_buf = m_payloadBuf[program][stream].buf;
        int & payload_len = m_payloadBuf[program][stream].len;

        if (payload_len + frame->frame.length > MIN_PAYLOAD_SIZE)
        {
            if (payload_len != 0)
            {
                pData = payload_buf;
                datalen = payload_len;
                frame_time = m_payloadBuf[program][stream].t;
            }
        }
        else
        {
            if (payload_len == 0)
            {
                m_payloadBuf[program][stream].t = frame->frame.time;
            }

            memcpy(payload_buf + payload_len, pData, datalen);
            payload_len += datalen;
            return TS_MUXER_OK;
        }
    }
    else
    {
        return TS_MUXER_PARAM_ERROR;
    }
    
    //buffer
    //////////////////////////////////////////////////////////////////////////

    int expect_len = estimate_ts_pack_size(ts, program, stream, datalen);

    if (ts->out_len + expect_len > ts->out_buf_size)
    {
        if (ts->out_buf)
        {
            m_pBuffer->UsedBuffer((unsigned char *)ts->out_buf, ts->out_len);
            m_out.push(std::make_pair(ts->out_buf, ts->out_len));
        }
        char * buf = NULL;
        unsigned int buf_len = expect_len < DEFAULT_PAGE_LENGTH ? DEFAULT_PAGE_LENGTH : ((expect_len + 0x3) & (~0x3));
        if(0 == m_pBuffer->GetBuffer((unsigned char **)&buf, buf_len))
        {
            ts->out_buf = buf;
            ts->out_buf_size = buf_len;
            ts->out_len = 0;
        }
        else
        {
            ts->out_buf = NULL;
            ts->out_buf_size = 0;
            ts->out_len = 0;
            return TS_MUXER_NO_BUFFER;
        }
    }

    //pts
    //////////////////////////////////////////////////////////////////////////

    TVT_DATE_TIME & prev_time = m_prevt[program][stream];
    unsigned long long & last_pts = m_pts[program][stream];
    
    unsigned long long diff = 0;

    if (prev_time.seconds == 0 && prev_time.microsecond == 0)
    {
        //first
       RESET_PTS_POLE;
    }
    else
    {
        diff = frame_time.Dec(prev_time);
        if (diff < 0)
        {
            assert(false);
            RESET_PTS_POLE;
        }
        else if (diff > 10000000)
        {
            RESET_PTS_POLE;
            diff = -1;
        }
    }

    unsigned long long pts = 0;

    if (diff < 0)
    {
        if (frame_type == 'v')
        {
            pts = last_pts + 90000 / m_option.programs[program].streams[stream].fps_or_bps;
        }
        else if (frame_type == 'a')
        {
            pts = last_pts + 90000 * datalen / m_option.programs[program].streams[stream].fps_or_bps;
        }
        else
        {
            assert(false);
        }
    }
    else
    {
        pts = last_pts + diff * 90000 / 1000000;
    }

    last_pts = pts;
    prev_time = frame_time;

    //////////////////////////////////////////////////////////////////////////

    ts_put_frame(ts, ts->program[program], ts->program[program]->stream[stream], 
        pData, datalen, !!frame->frame.bKeyFrame, pts);

    if (pData != frame_data)
    {
        if (frame->frame.length > MIN_PAYLOAD_SIZE)
        {
            m_payloadBuf[program][stream].len = 0;
            goto ProcesFrame;
        }
        else
        {
            memcpy(m_payloadBuf[program][stream].buf, frame_data, frame->frame.length);
            m_payloadBuf[program][stream].len = frame->frame.length;
            m_payloadBuf[program][stream].t = frame->frame.time;
        }
    }

    return TS_MUXER_OK;
}

int CTSMuxer::Get( char ** buf, int * len )
{
    if (m_out.empty())
    {
        return TS_MUXER_NO_OUTPUT;
    }
    else
    {
        *buf = m_out.front().first;
        *len = m_out.front().second;

        m_out.pop();
    }
    return TS_MUXER_OK;
}

void CTSMuxer::Free( char * buf, int len )
{
    m_pBuffer->ReleaseBuf((unsigned char *)buf, len);
}

void CTSMuxer::Flush()
{
    TS_STREAM * ts = (TS_STREAM *)m_ts;
    if (ts->out_buf)
    {
        m_pBuffer->UsedBuffer((unsigned char *)ts->out_buf, ts->out_len);
        m_out.push(std::make_pair(ts->out_buf, ts->out_len));

        ts->out_buf = NULL;
        ts->out_buf_size = 0;
        ts->out_len = 0;
    }
}

void TS_OPTION::SetBitRate( int rate )
{
    this->bit_rate = rate;
}

int TS_OPTION::AddStream( int program, char type )
{
    if (program > (int)programs.size() || program < 0)
    {
        return -1;
    }
    int n = programs[program].streams.size();
    programs[program].streams.push_back(stream());
    programs[program].streams[n].type = type;
    return n;
}

int TS_OPTION::AddProgram()
{
    int n = programs.size();
    programs.push_back(program());
    return programs.size() - 1;
}

void TS_OPTION::SetStreamFPSOrBPS( int program, int stream, int val )
{
    if (program > (int)programs.size() || program < 0)
    {
        return;
    }

    if (stream > (int)programs[program].streams.size() || stream < 0)
    {
        return;
    }

    programs[program].streams[stream].fps_or_bps = val;
}


