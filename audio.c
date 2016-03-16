#include <stdio.h>
#include "audio.h"

int pcm_open(snd_pcm_t **handle,char *name,snd_pcm_stream_t stream)
{
	//设置为阻塞模式，非阻塞模式为SND_PCM_NONBLOCK
	return snd_pcm_open(handle,name,stream,0);
}
void pcm_close(snd_pcm_t *handle)
{
	if(snd_pcm_close(handle))
	{
		snd_pcm_nonblock(handle,2);
		snd_pcm_close(handle);
	}
	return;
}
/*
44100 1 s16_le
======sw dump========
tstamp_mode: NONE
tstamp_type: MONOTONIC
period_step: 1
avail_min: 4096
start_threshold: 16384
stop_threshold: 16384
silence_threshold: 0
silence_size: 0
boundary: 1073741824
======hw dump========
ACCESS:  RW_INTERLEAVED
FORMAT:  S16_LE
SUBFORMAT:  STD
SAMPLE_BITS: 16
FRAME_BITS: 16
CHANNELS: 1
RATE: 44100
PERIOD_TIME: (92879 92880)
PERIOD_SIZE: 4096
PERIOD_BYTES: 8192
PERIODS: 4
BUFFER_TIME: (371519 371520)
BUFFER_SIZE: 16384
BUFFER_BYTES: 32768
TICK_TIME: 0
======dump end========
*/

int pcm_setup(snd_pcm_t *handle,snd_pcm_format_t format,unsigned int rate, unsigned int channels,snd_pcm_uframes_t *chunk_size)
{
	int ret = 0;
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	snd_pcm_hw_params_alloca(&params);  //auto free when return caller
	snd_pcm_sw_params_alloca(&swparams);	//auto free when return caller

	ret = snd_pcm_hw_params_any(handle, params);
	if(ret<0)
	{
		printf("snd_pcm_hw_params_any failed\n");
		goto ERR0;
	}
#if _USE_MMAP
	snd_pcm_access_mask_t *mask = alloca(snd_pcm_access_mask_sizeof()); //auto free when return caller
	snd_pcm_access_mask_none(mask);
	snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_INTERLEAVED);
	snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_NONINTERLEAVED);
	snd_pcm_access_mask_set(mask, SND_PCM_ACCESS_MMAP_COMPLEX);
	ret = snd_pcm_hw_params_set_access_mask(handle, params, mask);
	if(ret<0)
	{
		printf("snd_pcm_hw_params_set_access_mask failed\n");
		goto ERR0;
	}
#else
	ret = snd_pcm_hw_params_set_access(handle, params,
						   SND_PCM_ACCESS_RW_INTERLEAVED);
	if(ret<0)
	{
		printf("snd_pcm_hw_params_set_access failed\n");
		goto ERR0;
	}
#endif

	ret = snd_pcm_hw_params_set_format(handle,params,format);
	if (ret < 0) 
	{
		printf("snd_pcm_hw_params_set_format failed;format=%d\n",format);
		goto ERR0;
	}
	ret = snd_pcm_hw_params_set_channels(handle,params,channels);
	if (ret < 0) 
	{
		printf("snd_pcm_hw_params_set_channels failed;channels=%d\n",channels);
		goto ERR0;
	}
	ret = snd_pcm_hw_params_set_rate(handle, params,rate, 0);
	if (ret < 0) 
	{
		printf("snd_pcm_hw_params_set_rate failed;rate=%d\n",rate);
		goto ERR0;
	}
	snd_pcm_uframes_t buffer_max;
	snd_pcm_uframes_t buffer_min;
	snd_pcm_uframes_t buffer_size;
	ret = snd_pcm_hw_params_get_buffer_size_max(params,&buffer_max);
	ret |= snd_pcm_hw_params_get_buffer_size_min(params,&buffer_min);
	if (ret < 0) 
	{
		printf("snd_pcm_hw_params_get_buffer_size failed;buffer_max=%d,buffer_min=%d\n",buffer_max,buffer_min);
		goto ERR0;
	}
	if(*chunk_size>buffer_max)
	{
		*chunk_size = buffer_max/8;
		buffer_size = buffer_max;
		
	}
	else
	{
		buffer_size = *chunk_size*8;
		if(buffer_size<buffer_min)
			buffer_size = buffer_min;
		else if(buffer_size>buffer_max)
			buffer_size = buffer_max;
	}
	ret = snd_pcm_hw_params_set_period_size_near(handle, params,chunk_size, 0);
	if(ret<0)
	{
		printf("snd_pcm_hw_params_set_period_size_near failed;period=%d\n",*chunk_size);
		goto ERR0;
	}
	ret = snd_pcm_hw_params_set_buffer_size_near(handle, params,&buffer_size);
	if(ret<0)
	{
		printf("snd_pcm_hw_params_set_buffer_size_near failed;buffer_size=%d\n",buffer_size);
		goto ERR0;
	}
	ret = snd_pcm_hw_params(handle, params);
	if (ret < 0) 
	{
		printf("snd_pcm_hw_params failed;\n");
		goto ERR0;
	}

	ret = snd_pcm_sw_params_current(handle,swparams);
	if (ret < 0) 
	{
		printf("snd_pcm_sw_params_current failed;\n");
		goto ERR0;
	}
	//最好设置与chunk_size一致,否则容易underrun
	ret = snd_pcm_sw_params_set_avail_min(handle,swparams,*chunk_size);
	if (ret < 0)
	{
		printf("snd_pcm_sw_params_set_avail_min failed;\n");
		goto ERR0;
	}
	//最好设置与buffer_size一致,否则容易underrun
	ret = snd_pcm_sw_params_set_start_threshold(handle, swparams,buffer_size);
	ret |= snd_pcm_sw_params_set_stop_threshold(handle, swparams,buffer_size);
	if (ret < 0)
	{
		printf("snd_pcm_sw_params_set_threshold failed;\n");
		goto ERR0;
	}

	
	ret = snd_pcm_sw_params(handle, swparams);
	if (ret < 0)
	{
		printf("snd_pcm_sw_params failed;\n");
		goto ERR0;
	}
#if _USE_MMAP
	const snd_pcm_channel_area_t *areas;
	snd_pcm_uframes_t offset, size = *chunk_size;
	ret = snd_pcm_mmap_begin(handle, &areas, &offset, &size);
	if (ret < 0)
	{
		printf("snd_pcm_mmap_begin failed;\n");
		goto ERR0;
	}
	snd_pcm_mmap_commit(handle, offset, 0);
#endif
ERR0:
	return ret;
}

int pcm_write(snd_pcm_t *handle,unsigned char * buf,snd_pcm_uframes_t count)
{
	int ret = 0;
	snd_pcm_format_t format;
	unsigned int channles;
	snd_pcm_uframes_t bytes_per_frame = 2;
	snd_pcm_hw_params_t *params; 
	snd_pcm_hw_params_alloca(&params);  //auto free when return caller
	snd_pcm_hw_params_current(handle,params);
	snd_pcm_hw_params_get_format(params,&format);
	snd_pcm_hw_params_get_channels(params,&channles);
	switch(format){
		case SND_PCM_FORMAT_U8:
			bytes_per_frame = 1*channles;
			break;
		case SND_PCM_FORMAT_S16_BE:
		case SND_PCM_FORMAT_S16_LE:
			bytes_per_frame = 2*channles;
			break;
		default:
			printf("It is not support format=%d\n",format);
			return -1;
	}
	while (1) 
	{
#if _USE_MMAP
		ret = snd_pcm_mmap_writei(handle,buf,count);
#else
		ret = snd_pcm_writei(handle,buf,count);
#endif
		if(ret==count)
		{
			ret = 0;
			break;
		}
		else if(ret == -EAGAIN || (ret >= 0 && ret < count))
		{
			snd_pcm_wait(handle, 10);
			count -= ret;
			buf += ret * bytes_per_frame;
		}
		else if (ret == -EPIPE) 
		{
			printf("EPIPE\n");
			snd_pcm_prepare(handle);
			snd_pcm_start(handle);
		} 
		else if (ret == -ESTRPIPE) 
		{
			printf("ESTRPIPE\n");
			
		}
		else
		{
			perror("pcm_write");
			break;	
		}
		
	}
	return ret;
}
int pcm_read(snd_pcm_t *handle,unsigned char * buf,snd_pcm_uframes_t count)
{
	int ret = 0;
	snd_pcm_format_t format;
	unsigned int channles;
	snd_pcm_uframes_t bytes_per_frame = 2;
	snd_pcm_hw_params_t *params; 
	snd_pcm_hw_params_alloca(&params);	//auto free when return caller
	snd_pcm_hw_params_current(handle,params);
	snd_pcm_hw_params_get_format(params,&format);
	snd_pcm_hw_params_get_channels(params,&channles);
	switch(format){
		case SND_PCM_FORMAT_U8:
			bytes_per_frame = 1*channles;
			break;
		case SND_PCM_FORMAT_S16_BE:
		case SND_PCM_FORMAT_S16_LE:
			bytes_per_frame = 2*channles;
			break;
		default:
			printf("It is not support format=%d\n",format);
			return -1;
	}
	snd_pcm_prepare(handle);
	while (1) 
	{	
#if _USE_MMAP
		ret = snd_pcm_mmap_readi(handle,buf,count);
#else
		ret = snd_pcm_readi(handle,buf,count);
#endif
		if(ret==count)
		{
			ret = 0;
			break;
		}
		else if(ret == -EAGAIN || (ret >= 0 && ret < count))
		{
			snd_pcm_wait(handle,10);
			count -= ret;
			buf += ret * bytes_per_frame;
		}
		else if (ret == -EPIPE) 
		{
			printf("EPIPE\n");
			snd_pcm_prepare(handle);
			snd_pcm_start(handle);
		} 
		else if (ret == -ESTRPIPE) 
		{
			printf("ESTRPIPE\n");
			
		}
		else
		{
			perror("pcm_read");
			break;	
		}
		
	}
	return ret;
}
int pcm_drain(snd_pcm_t *handle)
{
	return snd_pcm_drain(handle);
}


