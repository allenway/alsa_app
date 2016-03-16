#include <stdio.h>
#include "audio.h"
#include "thread.h"

#define buf_count 	60*1024*1024
unsigned char buf[buf_count];
snd_pcm_uframes_t chunk_size[2] = {512,5112};
size_t chunk_bytes[2]; 
int channels = 1;

int cap_start = 0;
int cap_end = 0;

void *play(void *data)
{
	snd_pcm_t *playback;
	playback = (snd_pcm_t *)data;
	int ret;
	unsigned char *pcm;
	sleep(1);
	while(1)
	{
		if((cap_end-cap_start)>=chunk_bytes[0])
		{
			pcm = &buf[cap_start];
			ret = pcm_write(playback,pcm,chunk_size[0]);
			if(ret)
			{
				printf("pcm_read error,ret=%d\n",ret);
			}
			else
			{
				cap_start+=chunk_size[0];
			}
		}
		else
		{
			printf("play no buf,start:%d,end:%d\n",cap_start,cap_end);
			sleep(1);
		}
	}

}
void *cap(void *data)
{
	int ret;
	snd_pcm_t *capture;
	capture = (snd_pcm_t *)data;
	unsigned char *pcm;
	snd_pcm_prepare(capture);
	while(1)
	{
		if((buf_count-cap_end - 1)>=chunk_bytes[1])
		{
			pcm = &buf[cap_end+1];
			ret = pcm_read(capture,pcm,chunk_size[1]);
			if(ret)
			{
				printf("pcm_read error,ret=%d\n",ret);
			}
			else
			{
				cap_end+=chunk_size[1];
			}
		}
		else
		{
			printf("cap no buf\n");
			sleep(5);
		}
	}
}

int main()
{
	int ret = 0;
	printf("loopback 44100 mono s16_le\n");
	
	snd_pcm_t *playback,*capture;
	//step1, open pcm device
	ret = pcm_open(&playback,"default",SND_PCM_STREAM_PLAYBACK);
	if(ret)
	{
		printf("pcm_open failed,ret=%d\n",ret);
		goto ERR0;
	}
	ret = pcm_open(&capture,"default",SND_PCM_STREAM_CAPTURE);
	if(ret)
	{
		printf("pcm_open failed,ret=%d\n",ret);
		goto ERR01;
	}
	
	//step2 setup pcm
	
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	ret = pcm_setup(playback,format,44100,channels,&chunk_size[0]);
	if(ret)
	{
		goto ERR1;
	}
	ret = pcm_setup(capture,format,44100,channels,&chunk_size[1]);
	if(ret)
	{
		goto ERR1;
	}
	
	size_t bytes_per_frame;
	switch(format){
		case SND_PCM_FORMAT_U8:
			bytes_per_frame = 1*channels;
			break;
		case SND_PCM_FORMAT_S16_BE:
		case SND_PCM_FORMAT_S16_LE:
			bytes_per_frame = 2*channels;
			break;
		default:
			printf("It is not support format=%d\n",format);
			goto ERR1;
	}
	
	chunk_bytes[0]= chunk_size[0]*bytes_per_frame;
	chunk_bytes[1]= chunk_size[1]*bytes_per_frame;
	//step4 play
	
	pthread_t pp,pc;
	ret = ThreadCreate( &pp,play,playback);
	ret = ThreadCreate( &pc,cap,capture);
	while(1)
	{
		sleep(1000);
	}

ERR1:
	pcm_close(capture);
ERR01:
	pcm_close(playback);
	
ERR0:
	return ret;
}


