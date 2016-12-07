/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "pool.h"
#include <stdio.h>

ThreadPool::ThreadPool(unsigned int size, void(*init_pthread_handler)(), void*(*pthread_handler)(void*), void* arg, void(*exit_pthread_handler)())
{
	if(init_pthread_handler)
		(*init_pthread_handler)();
	m_size = size;

	m_exit_pthread_handler = exit_pthread_handler;
	m_pthread_list = new pthread_t[m_size];
	m_pthread_attr_list = new pthread_attr_t[m_size];
	if(m_pthread_list)
	{	
		for(int i = 0; i < m_size; i++)
		{
			PoolArg* pArg = new PoolArg;
			pArg->index = i;
			pArg->data = arg;
			
			pthread_attr_init(&m_pthread_attr_list[i]);
			pthread_attr_setdetachstate (&m_pthread_attr_list[i], PTHREAD_CREATE_DETACHED);
			
			if(pthread_create(&m_pthread_list[i], &m_pthread_attr_list[i], pthread_handler, pArg) != 0)
			{
				printf("Create thread failed: %d\n", i);
			}

			
		}
	}
}


ThreadPool::~ThreadPool()
{
	for(int i = 0; i < m_size; i++)
	{
		pthread_attr_destroy (&m_pthread_attr_list[i]);
	}
	if(m_exit_pthread_handler)
		(*m_exit_pthread_handler)();
	
    if(m_pthread_attr_list)
        delete[] m_pthread_attr_list;
	if(m_pthread_list)
		delete[] m_pthread_list;
}
