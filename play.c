#include <stdio.h>
#include "audio.h"

int main(int argc,char *argv[])
{
	int ret = 0;
	if(argc<2)
	{
		printf("Usage:%s <filename>\n",argv[0]);
		printf("This app only support S16_LE 44100 MONO pcm format!\n");
		ret = -1;
		goto ERR0;
	}
	snd_pcm_t *handle;
	//step1, open pcm device
	ret = pcm_open(&handle,"default",SND_PCM_STREAM_PLAYBACK);
	if(ret)
	{
		printf("pcm_open failed,ret=%d\n",ret);
		goto ERR0;
	}
	//step2 setup pcm
	snd_pcm_uframes_t chunk_size = 512;
	int channels = 1;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	ret = pcm_setup(handle,format,44100,channels,&chunk_size);
	if(ret)
	{
		goto ERR1;
	}
	//step3 open file
	int fd;
	fd = open(argv[1],O_RDONLY);
	if(fd<0)
	{
		perror(argv[1]);
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
			goto ERR2;
	}
	size_t chunk_bytes = chunk_size*bytes_per_frame;
	//step4 play
	unsigned char *buf = malloc(chunk_bytes);
	if(buf==NULL)
	{
		perror("malloc");
		ret = -1;
		goto ERR2;
	}
	printf("play %s\n",argv[1]);
	printf("chunk_size=%d;chunk_bytes=%d\n",chunk_size,chunk_bytes);
	while(1)
	{
		ret = read(fd,buf,chunk_bytes);
		if(ret<0)
		{
			perror("read\n");
			goto ERR3;
		}
		//printf("runnig read bytes:%d\n",ret);
		if(ret==0)
			break;
		ret = pcm_write(handle,buf,ret/bytes_per_frame);
		if(ret)
		{
			printf("pcm_write failed,ret=%d\n",ret);
			goto ERR3;
		}
		
	}
	pcm_drain(handle);
	printf("play over\n");
ERR3:
	free(buf);
ERR2:
	close(fd);
ERR1:
	pcm_close(handle);
ERR0:
	return ret;
}


