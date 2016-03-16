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
	ret = pcm_open(&handle,"default",SND_PCM_STREAM_CAPTURE);
	if(ret)
	{
		printf("pcm_open failed,ret=%d\n",ret);
		goto ERR0;
	}
	//step2 setup pcm
	snd_pcm_uframes_t chunk_size = 5112;
	int channels = 1;
	snd_pcm_format_t format = SND_PCM_FORMAT_S16_LE;
	ret = pcm_setup(handle,format,44100,channels,&chunk_size);
	if(ret)
	{
		goto ERR1;
	}
	char *ftype;
	size_t bytes_per_frame;
	switch(format){
		case SND_PCM_FORMAT_U8:
			bytes_per_frame = 1*channels;
			ftype = "U8";
			break;
		case SND_PCM_FORMAT_S16_BE:
			ftype = "S16_BE";
			bytes_per_frame = 2*channels;
			break;
		case SND_PCM_FORMAT_S16_LE:
			ftype = "S16_LE";
			bytes_per_frame = 2*channels;
			break;
		default:
			printf("It is not support format=%d\n",format);
			goto ERR1;
	}
	size_t chunk_bytes = chunk_size*bytes_per_frame;
	//step3 open file
	int fd;
	char filename[256] = {0};
	sprintf(filename,"%s.%d.%d.%s",argv[1],44100,channels,ftype);
	fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0644);
	if(fd<0)
	{
		perror(argv[1]);
		goto ERR1;
	}
	
	
	//step4 play
	unsigned char *buf = malloc(chunk_bytes);
	if(buf==NULL)
	{
		perror("malloc");
		ret = -1;
		goto ERR1;
	}
	printf("record %s\n",argv[1]);
	printf("chunk_size=%d;chunk_bytes=%d\n",chunk_size,chunk_bytes);

	while(1)
	{
		ret = pcm_read(handle,buf,chunk_size);
		if(ret)
		{
			printf("pcm_read failed,ret=%d\n",ret);
			goto ERR2;
		}
		ret = write(fd,buf,chunk_size*bytes_per_frame);
		if(ret<0)
		{
			perror("write\n");
			goto ERR2;
		}
		fsync(fd);
	}
	pcm_drain(handle);
	printf("reocrd over\n");
ERR2:
	free(buf);
ERR1:
	pcm_close(handle);
ERR0:
	return ret;
}


