#ifndef __AUDIO_H___
#define __AUDIO_H___
#define _GNU_SOURCE
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#define _USE_MMAP 1
//打开句柄
extern int pcm_open(snd_pcm_t **handle,char *name,snd_pcm_stream_t stream);
//关闭句柄
extern void pcm_close(snd_pcm_t *handle);
//参数设置，chunk_size尽量设置比较大的值,否则会发送underrun或overrun
extern int pcm_setup(snd_pcm_t *handle,snd_pcm_format_t format,unsigned int rate, unsigned int channels,snd_pcm_uframes_t *chunk_size);
//写pcm数据
extern int pcm_write(snd_pcm_t *handle,unsigned char * buf,snd_pcm_uframes_t size);
//读pcm数据
extern int pcm_read(snd_pcm_t *handle,unsigned char * buf,snd_pcm_uframes_t size);
//清空pcm缓存数据,由于关闭句柄前操作
extern int pcm_drain(snd_pcm_t *handle);
#endif // __AUDIO_H___