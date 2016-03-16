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
//�򿪾��
extern int pcm_open(snd_pcm_t **handle,char *name,snd_pcm_stream_t stream);
//�رվ��
extern void pcm_close(snd_pcm_t *handle);
//�������ã�chunk_size�������ñȽϴ��ֵ,����ᷢ��underrun��overrun
extern int pcm_setup(snd_pcm_t *handle,snd_pcm_format_t format,unsigned int rate, unsigned int channels,snd_pcm_uframes_t *chunk_size);
//дpcm����
extern int pcm_write(snd_pcm_t *handle,unsigned char * buf,snd_pcm_uframes_t size);
//��pcm����
extern int pcm_read(snd_pcm_t *handle,unsigned char * buf,snd_pcm_uframes_t size);
//���pcm��������,���ڹرվ��ǰ����
extern int pcm_drain(snd_pcm_t *handle);
#endif // __AUDIO_H___