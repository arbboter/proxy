#include "TSStream.h"
#include <string.h>
#include <assert.h>


#define PAT_TID   0x00
#define PMT_TID   0x02
#define SDT_TID   0x42

#define PUT16(p, x) *p++ = (x) >> 8; *p++ = (x)

static unsigned int crc32table[256] = {   
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,   
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,   
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,   
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,   
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,   
    0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,   
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,   
    0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,   
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,   
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,   
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,   
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,   
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,   
    0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,   
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,   
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,   
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,   
    0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,   
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,   
    0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,   
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,   
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,   
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,   
    0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,   
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,   
    0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,   
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,   
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,   
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,   
    0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,   
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,   
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,   
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,   
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,   
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,   
    0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,   
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,   
    0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,   
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,   
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,   
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,   
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,   
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

static unsigned int CRC32(unsigned char *data, int len)
{   
    int i;   
    int crc = 0xFFFFFFFF;       
    for(i = 0; i < len; i++)
        crc = (crc << 8) ^ crc32table[((crc >> 24) ^ *data++) & 0xFF];       
    return crc;
}

static void fill_pes_pts(char * p, int fourbits, unsigned long long pts)
{
    int val;

    val = fourbits << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    *p++ = val;
    val = (((pts >> 15) & 0x7fff) << 1) | 1;
    *p++ = val >> 8;
    *p++ = val;
    val = (((pts) & 0x7fff) << 1) | 1;
    *p++ = val >> 8;
    *p++ = val;
}

static char * pack_pes(char * buf, char type, char * data, int len, unsigned long long pts, unsigned long long dts)
{
    char * p = buf;
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x01;

    if (type == 'v')
    {
        *p++ = (char)0xe0;
    }
    else if (type == 'a')
    {
        *p++ = (char)0xc0;
    }

    int header_len = 5;
    int flags = 0x80;
    if (type == 'v')
    {
        flags |= 0x40;
        header_len += 5;
    }

    int packet_len = len + header_len + 3;
    if (type == 'v' && packet_len > 65535)
    {
        packet_len = 0;
    }

    *p++ = packet_len >> 8;
    *p++ = packet_len;
    *p++ = (char)0x80;
    *p++ = flags;
    *p++ = header_len;

    fill_pes_pts(p, flags >> 6, pts);
    p += 5;

    if (type == 'v')
    {
        fill_pes_pts(p, 1, dts);
        p += 5;
    }

    return p;
}

static void set_ts_adaption_field_flag(char * packet, int flag)
{
    if ((packet[3] & 0x20) == 0) {
        packet[3] |= 0x20;
        packet[4] = 1;
        packet[5] = 0;
    }
    packet[5] |= flag;
}

static char * ts_packet_payload_pos(char * packet)
{
    if (packet[3] & 0x20)
        return packet + 5 + packet[4];
    else
        return packet + 4;
}

static void pack_ts_info_table(TS_STREAM * ts, TS_TABLE * table, char * buf, int len)
{
    char ts_packet[TS_PACKET_SIZE];
    char * p;
    int tmp;
    int l;
    int remain;

    unsigned int crc = CRC32((unsigned char *)buf, len - 4);
    buf[len - 4] = (crc >> 24) & 0xff;
    buf[len - 3] = (crc >> 16) & 0xff;
    buf[len - 2] = (crc >> 8) & 0xff;
    buf[len - 1] = (crc) & 0xff;

    char first = 1;
    while (len > 0)
    {
        p = ts_packet;
        *p++ = 0x47;
        tmp = (table->pid >> 8);
        if (first)
        {
            tmp |= 0x40;
        }
        *p++ = tmp;
        *p++ = table->pid;
        table->cc = (table->cc + 1) & 0xf;
        *p++ = 0x10 | table->cc;
        if (first)
        {
            *p++ = 0;
        }
        l = TS_PACKET_SIZE - (p - ts_packet);
        if (l > len)
        {
            l = len;
        }
        memcpy(p, buf, l);
        p += l;
        remain = TS_PACKET_SIZE - (p - ts_packet);
        if (remain > 0)
        {
            memset(p, 0xff, remain);
        }

        assert(ts->out_len + TS_PACKET_SIZE <= ts->out_buf_size);
        memcpy(ts->out_buf + ts->out_len, ts_packet, TS_PACKET_SIZE);
        ts->out_len += TS_PACKET_SIZE;

        buf += l;
        len -= l;

        first = 0;
    }
}

static void fill_ts_info_table(TS_STREAM * ts, TS_TABLE * table, int tid, int id, int version, 
                               int sec_num, int last_sec_num, char * buf, int len)
{
    char table_buf[1024];
    char *p = table_buf;
    unsigned int flags = 0xb000;

    int length = len + 5 + 4 + 3;

    *p++ = tid;
    PUT16(p, flags | (len + 5 + 4));
    PUT16(p, id);
    *p++ = 0xc1 | (version << 1);
    *p++ = sec_num;
    *p++ = last_sec_num;
    memcpy(p, buf, len);

    pack_ts_info_table(ts, table, table_buf, length);
}

static void add_pat(TS_STREAM * ts)
{
    char pat[1024];
    char * p = pat;

    for (int i = 0; i < ts->program_num; i++)
    {
        TS_PROGRAM * program = ts->program[i];
        PUT16(p, program->sid);
        PUT16(p, program->pmt.pid | 0xe000);
    }

    fill_ts_info_table(ts, &ts->pat, PAT_TID, 1, ts->version, 0, 0, pat, p - pat);
}

static void add_pmt(TS_STREAM * ts)
{
    char pmt[1024];
    int stream_type;

    for (int i = 0; i < ts->program_num; i++)
    {
        TS_PROGRAM * program = ts->program[i];
        char * p = pmt;

        PUT16(p, 0xe000 | program->pcr_pid);

        *p++ = 0xf0;
        *p++ = 0x00;
        for (int j = 0; j < program->stream_num; j++)
        {
            ELEM_STREAM * stream = program->stream[j];

            if (stream->type == 'v')
            {
                stream_type = 0x1b; //h264
            }
            else if(stream->type == 'a')
            {
                stream_type = 0x0f; //aac
            }
            else
            {
                assert(false);
            }

            *p++ = stream_type;
            PUT16(p , 0xe000 | stream->pid);
            *p++ = 0xf0;
            *p++ = 0x00;
        }

        fill_ts_info_table(ts, &program->pmt, PMT_TID, program->sid, ts->version, 0, 0, pmt, p - pmt);
    }
}

static void add_si_info(TS_STREAM *ts, char force_pat)
{

    if (++ts->pat_packet_count >= ts->pat_period || force_pat)
    {
        ts->pat_packet_count = 0;
        add_pat(ts);
        add_pmt(ts);
    }
}

void ts_put_frame(TS_STREAM *ts, TS_PROGRAM * program, ELEM_STREAM *stream, 
                         char * data, int len, char key, unsigned long long pts)
{
    char    force_pat = (key && stream->type == 'v' && !ts->prev_key);
    char    ts_packet[TS_PACKET_SIZE];
    char    is_start = 1;
    char    tmp;
    int     header_len;
    int     payload_len;
    int     stuffing_len;
    int     add_pcr;

    unsigned long long pcr;
    unsigned long long high, low;

    while (len > 0)
    {
        add_si_info(ts, force_pat);
        force_pat = 0;

        add_pcr = 0;
        if (stream->pid == program->pcr_pid)
        {
            program->pcr_packet_count ++;
            if (program->pcr_packet_count >= program->pcr_period)
            {
                program->pcr_packet_count = 0;
                add_pcr = 1;
            }
        }

        char *p = ts_packet;
        *p++ = 0x47;
        tmp = (stream->pid >> 8);
        if (is_start)
        {
            tmp |= 0x40;
        }

        *p++ = tmp;
        *p++ = stream->pid;
        stream->cc = (stream->cc + 1) & 0xf;
        *p++ = stream->cc | 0x10;
        if (key && is_start)
        {
            set_ts_adaption_field_flag(ts_packet, 0x40);
            p = ts_packet_payload_pos(ts_packet);
        }

        if (add_pcr)
        {
            set_ts_adaption_field_flag(ts_packet, 0x10);
            p = ts_packet_payload_pos(ts_packet);

            pcr = pts * 300;

            high = pcr / 300; 
            low = pcr % 300; 

            *p++ = high >> 25;
            *p++ = high >> 17;
            *p++ = high >> 9;
            *p++ = high >> 1;
            *p++ = high << 7 | low >> 8 | 0x7e;
            *p++ = low;
            ts_packet[4] += 6;
            p = ts_packet_payload_pos(ts_packet);
        }

        if (is_start)
        {
            if (stream->type == 'v')
            {
                len += 6;
            }
            
            p = pack_pes(p, stream->type, data, len, pts, pts);
        }

        header_len = p - ts_packet;
        payload_len = TS_PACKET_SIZE - header_len;
        if (payload_len > len)
        {
            payload_len = len;
        }

        stuffing_len = TS_PACKET_SIZE - header_len - payload_len;

        if (stuffing_len > 0)
        {
            if (ts_packet[3] & 0x20)
            {
                int af_len = ts_packet[4] + 1;
                memmove(ts_packet + 4 + af_len + stuffing_len, ts_packet + 4 + af_len, header_len - (4 + af_len));
                ts_packet[4] += stuffing_len;
                memset(ts_packet + 4 + af_len, 0xff, stuffing_len);
            }
            else
            {
                memmove(ts_packet + 4 + stuffing_len, ts_packet + 4, header_len - 4);
                ts_packet[3] |= 0x20;
                ts_packet[4] = stuffing_len - 1;
                if (stuffing_len >= 2)
                {
                    ts_packet[5] = 0x00;
                    memset(ts_packet + 6, 0xff, stuffing_len - 2);
                }
            }
        }

        if (is_start && stream->type == 'v')
        {
            char *p = ts_packet + TS_PACKET_SIZE - payload_len;
            *p++ = 0x00;*p++ = 0x00;*p++ = 0x00;*p++ = 0x01;*p++ = 0x09;*p++ = 0xf0;
            payload_len -= 6;
            len -= 6;
            memcpy(p, data, payload_len);
        }
        else
        {
            memcpy(ts_packet + TS_PACKET_SIZE - payload_len, data, payload_len);
        }

        data += payload_len;
        len -= payload_len;

        //out
        assert(ts->out_buf_size >= ts->out_len + TS_PACKET_SIZE);
        memcpy(ts->out_buf + ts->out_len, ts_packet, TS_PACKET_SIZE);
        ts->out_len += TS_PACKET_SIZE;

        is_start = 0;
    }

    ts->prev_key = key;
}

