#include "cs537.h"
#include "request.h"
#include <pthread.h>
#include <assert.h>
#include <sched.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t filled = PTHREAD_COND_INITIALIZER;

void Thread_mutex_lock(pthread_mutex_t *mutex) {
    int ret = pthread_mutex_lock(mutex);
    assert(ret == 0);
}
                                                                                
void Thread_mutex_unlock(pthread_mutex_t *mutex) {
    int ret = pthread_mutex_unlock(mutex);
    assert(ret == 0);
}

void Thread_cond_wait(pthread_cond_t * filled, pthread_mutex_t *mutex) {
    pthread_cond_wait(filled, mutex);
}
void Thread_cond_signal(pthread_cond_t * filled) {
   pthread_cond_signal(&filled);
}
                                                                              
void Thread_create(pthread_t *thread, const pthread_attr_t *attr, 	
	       void *(*start_routine)(void*), void *arg) {
    int ret = pthread_create(thread, attr, start_routine, arg);
    assert(ret == 0);
}

void Thread_join(pthread_t thread, void **value_ptr) {
    int ret = pthread_join(thread, value_ptr);
    assert(ret == 0);
}

int thread_pool_size, buffer_size;
int put_index = 0, get_index = 0, count = 0;
int *buffer_ptr;

void getargs(int *port, int *threads,int *buffers,int argc, char *argv[])
{
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *threads = atoi(argv[2]);
    *buffers = atoi(argv[3]);
}


void* consumer(void *args){
	while(1){
		int connfd;
		Thread_mutex_lock(&mutex);
		while(count == 0)
			Thread_cond_wait(&filled, &mutex);
		connfd = buffer_ptr[get_index];
		get_index = (get_index + 1) % buffer_size;
	    count--;
		Thread_mutex_unlock(&mutex);
		requestHandle(connfd);
		Close(connfd);
	}
}

void thread_pool_create(int size, int buf_size){
	int loop;
	pthread_t pool[size];
	for(loop=0;loop<size;loop++){
		Thread_create(&pool[loop],NULL,consumer,NULL);
	}
	thread_pool_size = size;
	buffer_size = buf_size;
	buffer_ptr = (int*)malloc(buf_size * sizeof(int));
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, threads, buffers;
    struct sockaddr_in clientaddr;

    getargs(&port, &threads, &buffers, argc, argv);
    thread_pool_create(threads,buffers);

    listenfd = Open_listenfd(port);

    while (1) {
		clientlen = sizeof(clientaddr);
		connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

		Thread_mutex_lock(&mutex);
		if(count == buffer_size){
			Thread_mutex_unlock(&mutex);
			Close(connfd);
		}
		else {
			buffer_ptr[put_index] = connfd;
			put_index = ( put_index + 1) % buffer_size;
	        count++;
			pthread_cond_signal(&filled);
			Thread_mutex_unlock(&mutex);
		}
    }

}