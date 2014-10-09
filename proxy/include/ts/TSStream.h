#ifndef __TS_H__
#define __TS_H__

#define TS_PACKET_SIZE 188
#define PAT_RETRANS_TIME 100
#define PCR_RETRANS_TIME 20

typedef struct _ts_section_
{
    int         pid;
    int         cc;
}TS_TABLE;

typedef struct _elem_stream_
{
    char        type;
    int         pid;
    int         cc;
}ELEM_STREAM;

typedef struct _ts_program_
{
    TS_TABLE        pmt;
    int             sid;

    ELEM_STREAM **  stream;
    int             stream_num;

    int             pcr_pid;
    int             pcr_packet_count;
    int             pcr_period;
}TS_PROGRAM;

typedef struct _ts_stream_
{
    int             version;
    char            prev_key;
    int             pat_period;
    int             pat_packet_count;

    int             bit_rate;

    TS_TABLE        pat;

    TS_PROGRAM **   program;
    int             program_num;

    char *          out_buf;
    int             out_len;
    int             out_buf_size;
}TS_STREAM;


void ts_put_frame(TS_STREAM *ts, TS_PROGRAM * program, ELEM_STREAM *stream, 
                  char * data, int len, char key, unsigned long long pts);

#endif