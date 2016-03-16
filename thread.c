#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "thread.h"


int ThreadCreate( pthread_t *threadId, void *(*threadFunc)(void*), void *arg )
{
	return pthread_create( threadId, NULL, threadFunc, arg );     
}

unsigned long ThreadSelf()
{
	return (unsigned long)pthread_self();
}

int ThreadJoin( pthread_t thread, void **valuePtr )
{
	int errNo = pthread_join( thread, valuePtr );
	return errNo;
}

int ThreadDetach( pthread_t thread )
{
	int errNo = pthread_detach( thread );
	return errNo;
}

void ThreadExit( void *valuePtr )
{
	pthread_exit( valuePtr );
}

