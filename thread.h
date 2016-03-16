#ifndef __THREAD_H__
#define __THREAD_H__

#include <pthread.h>

//
// �߳����ȼ�����
//
#define	THREAD_SCHED_POLICY		SCHED_FIFO

//
// �߳����ȼ�����, ���ȼ����岻Ӧ�ø���50.
#define THREAD_PRIORITY_COMMON	30
#define THREAD_PRIORITY_WDT		40

typedef struct _ThreadMaintain_
{
	pthread_t 	id;
	int     	runFlag;
} THREAD_MAINTAIN_T;

#ifdef __cplusplus
extern "C" {
#endif

int ThreadCreate( pthread_t *threadId, void *(*threadFunc)(void*), void *arg );
int ThreadJoin( pthread_t thread, void **valuePtr );
int ThreadDetach( pthread_t thread );
void ThreadExit( void *valuePtr );
unsigned long ThreadSelf();
    

#ifdef __cplusplus
}
#endif

#endif  

