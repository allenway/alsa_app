/*
*wav文件转换成pcm数据
*
*/
#include <endian.h>
#include <byteswap.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((a) | ((b)<<8) | ((c)<<16) | ((d)<<24))
#define LE_SHORT(v)		(v)
#define LE_INT(v)		(v)
#define BE_SHORT(v)		bswap_16(v)
#define BE_INT(v)		bswap_32(v)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define COMPOSE_ID(a,b,c,d)	((d) | ((c)<<8) | ((b)<<16) | ((a)<<24))
#define LE_SHORT(v)		bswap_16(v)
#define LE_INT(v)		bswap_32(v)
#define BE_SHORT(v)		(v)
#define BE_INT(v)		(v)
#else
#error "Wrong endian"
#endif
/* Note: the following macros evaluate the parameter v twice */

#define TO_CPU_SHORT(v, be) \
	((be) ? BE_SHORT(v) : LE_SHORT(v))
#define TO_CPU_INT(v, be) \
	((be) ? BE_INT(v) : LE_INT(v))


#define WAV_RIFF		COMPOSE_ID('R','I','F','F')
#define WAV_RIFX		COMPOSE_ID('R','I','F','X')
#define WAV_WAVE		COMPOSE_ID('W','A','V','E')
#define WAV_FMT			COMPOSE_ID('f','m','t',' ')
#define WAV_DATA		COMPOSE_ID('d','a','t','a')

/* WAVE fmt block constants from Microsoft mmreg.h header */
#define WAV_FMT_PCM             0x0001
#define WAV_FMT_IEEE_FLOAT      0x0003
#define WAV_FMT_DOLBY_AC3_SPDIF 0x0092
#define WAV_FMT_EXTENSIBLE      0xfffe


#pragma pack(1)
typedef struct {
	u_int magic;		/* 'RIFF' */
	u_int length;		/* filelen */
	u_int type;		/* 'WAVE' */
} WaveRiff;
typedef struct {
	u_int type; 	/* 'fmt ' */
	u_int length;	/* 0x10 */
	u_short format;		/* see WAV_FMT_* */
	u_short channels;
	u_int sample_fq;	/* frequence of sample */
	u_int byte_p_sec;
	u_short byte_p_spl;	/* samplesize; 1 or 2 bytes */
	u_short bit_p_spl;	/* 8, 12 or 16 bit */
} WaveFmt;

typedef struct {
	u_int type;		/* 'data' */
	u_int length;	/* data count */
} WaveData;

typedef struct{
	WaveRiff riff;
	WaveFmt fmt;
	WaveData data;
} WaveHeaderInfo;
#pragma pack()

static ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t result = 0, res;

	while (count > 0) {
		if ((res = read(fd, buf, count)) == 0)
			break;
		if (res < 0)
			return result > 0 ? result : res;
		count -= res;
		result += res;
		buf = (char *)buf + res;
	}
	return result;
}

int main(int argc,char *argv[])
{
	int wav;
	int ret;
	if(argc<3)
	{
		printf("Usage:%s <m|s> <file> \n",argv[0]);
		printf("m:mono,s:stereo\n");
		ret  = -1;
		goto ERR0;
	}
	wav = open(argv[2],O_RDONLY);
	if(wav<0)
	{
		perror(argv[2]);
		ret -1;
		goto ERR0;
	}

	unsigned char audiobuf[8196] = {0};
	
	size_t size;
	int big_endian;
	/* read bytes for WAVE-header */
	
	size = sizeof(WaveHeaderInfo);
	if ((size_t)safe_read(wav, audiobuf, size) != size) {
		perror("safe_read");
		ret -1;
		goto ERR1;
	}
	//STEP1 check wave headr
	WaveHeaderInfo *h = (WaveHeaderInfo *)audiobuf;
	if(h->riff.magic == WAV_RIFF)
		big_endian = 0;
	else if(h->riff.magic == WAV_RIFX)
		big_endian = 1;
	else
	{
		ret = -1;
		printf("cann't not support %s,it's not have 'RIFF/RIFX' flag\n",argv[2]);
		goto ERR1;
	}
	if(h->riff.type != WAV_WAVE)
	{
		ret = -1;
		printf("cann't not support %s,it's not have 'WAVE' flag\n",argv[2]);
		goto ERR1;
	}	
	//STEP2 check fmt
	if(h->fmt.type != WAV_FMT || TO_CPU_INT(h->fmt.length, big_endian) != 0x10)
	{
		ret = -1;
		printf("cann't not support %s,it's not have 'fmt' flag\n",argv[2]);
		goto ERR1;
	}
	if(h->fmt.format!=WAV_FMT_PCM)
	{
		ret = -1;
		printf("cann't not support %s,it's not have 'fmt' flag\n",argv[2]);
		goto ERR1;
	}
	char *format;
	unsigned int channels;
	unsigned int rate;
	channels = TO_CPU_SHORT(h->fmt.channels, big_endian);
	switch (TO_CPU_SHORT(h->fmt.bit_p_spl, big_endian)) {
	case 8:
		format = "U8";
		break;
	case 16:
		if (big_endian)
			format = "S16_BE";
		else
			format = "S16_LE";
		break;
	case 24:
		switch (TO_CPU_SHORT(h->fmt.byte_p_spl, big_endian) /channels) {
		case 3:
			if (big_endian)
				format = "S24_3BE";
			else
				format = "S24_3LE";
			break;
		case 4:
			if (big_endian)
				format = "S24_BE";
			else
				format = "S24_LE";
			break;
		default:
			ret = -1;
			printf("cann't not support %s,unknow 24bit\n",argv[2]);
			goto ERR1;
		}
		break;
	case 32:
			if (big_endian)
				format = "S32_BE";
			else
				format = "S32_LE";
		break;
	default:
		ret = -1;
		printf("cann't not support %s,unknow 32bit\n",argv[2]);
		goto ERR1;
	}
	rate = TO_CPU_INT(h->fmt.sample_fq, big_endian);
	if(h->data.type!=WAV_DATA)
	{
	    ret = -1;
		printf("cann't not support %s,it's not have 'data' flag\n",argv[2]);
		goto ERR1;
	}
	int pcm;
	char name[256] = {0};
	if(argv[1][0]=='m')
		sprintf(name,"%s.%d.1.%s",argv[2],rate,format);
	else
		sprintf(name,"%s.%d.%d.%s",argv[2],rate,channels,format);
	pcm = open(name,O_WRONLY|O_CREAT|O_TRUNC,0644);
	if(pcm<0)
	{
		ret = -1;
		perror(name);
		goto ERR1;
	}
	if(argv[1][0]!='m'||channels==1)
	{
		while(1)
		{
			size = read(wav,audiobuf,8196);
			if(size<=0)
				break;
			char *buf = audiobuf;
			while(1)
			{
				ret = write(pcm,buf,size);
				if(ret==size)
					break;
				else if(ret<0)
				{
					ret = -1;
					perror("write");
					goto ERR2;
				}
				size -= ret;
				buf += ret;
			}
		}
	}
	else
	{
		int k,m;
		m = TO_CPU_SHORT(h->fmt.byte_p_spl,big_endian);
		k = m/channels;
		while(1)
		{
			char *buf = audiobuf;
			int copysize;
			copysize = 8196-8196%m;
			size = copysize;
			while(1)
			{
				
				ret = read(wav,buf,size);
				if(ret<0)
				{
					ret = 0;
					goto ERR2;
				}
				else if(ret==0)
				{
					copysize = copysize -size;
					break;
				}
				size -= ret;
				if(size==0)
				{
					break;
				}
				buf += ret;
				
			}
			if(copysize==0)
				break;
			int i = 0,j=0;
			
			while(j<copysize)
			{
				audiobuf[i] = audiobuf[j];
				i++;
				j++;
				if(i%k==0)
				{
					j+=k*(channels-1);
				}
				
			}
			size = copysize/channels;
			buf = audiobuf;
			while(1)
			{
				

				ret = write(pcm,buf,size);
				if(ret==size)
					break;
				else if(ret<0)
				{
					ret = -1;
					perror("write");
					goto ERR2;
				}
				size -= ret;
				buf += ret;
			}
		}
	}
ERR2:
	close(pcm);
ERR1:
	close(wav);
ERR0:
	return ret;
}

