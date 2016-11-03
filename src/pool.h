/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#ifndef _POOL_H_
#define _POOL_H_

#include <pthread.h>

typedef struct
{
	unsigned int index;
	void* data;
}PoolArg;

class ThreadPool
{
public:
	ThreadPool(unsigned int size, void(*init_pthread_handler)(), void*(*pthread_handler)(void*), void* arg, void(*exit_pthread_handler)());
	virtual ~ThreadPool();

protected:
	int m_size;
	pthread_t* m_pthread_list;
	pthread_attr_t* m_pthread_attr_list;
	void(*m_exit_pthread_handler)();
};
#endif /* _POOL_H_ */
