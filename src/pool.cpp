/*
	Copyright (c) openheap, uplusware
	uplusware@gmail.com
*/
#include "pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ThreadPool::ThreadPool(unsigned int size, void(*init_pthread_handler)(), void*(*pthread_handler)(void*), void* arg, int arg_len, void(*exit_pthread_handler)(), int pre_step)
{
    m_init_pthread_handler = init_pthread_handler;
    m_pthread_handler = pthread_handler;
	m_exit_pthread_handler = exit_pthread_handler;
    
	if(m_init_pthread_handler)
		(*m_init_pthread_handler)();
	m_size = size;
    
    if(arg && arg_len > 0)
    {
        m_arg = malloc(arg_len);
        memcpy(m_arg, arg, arg_len);
    }
    else
    {
        m_arg = NULL;
    }
    
    for(int t = 0; t < pre_step; t++)
    {
        pthread_t pthread_instance;
		
        PoolArg* pArg = new PoolArg;
        pArg->data = m_arg;

        if(pthread_create(&pthread_instance, NULL, m_pthread_handler, pArg) == 0)
		{
			pthread_detach(pthread_instance);
		}
    }
}

void ThreadPool::More(int step)
{
	for(int t = 0; t < step; t++)
    {
        pthread_t pthread_instance;
		
        PoolArg* pArg = new PoolArg;
        pArg->data = m_arg;

        if(pthread_create(&pthread_instance, NULL, m_pthread_handler, pArg) == 0)
		{
			pthread_detach(pthread_instance);
		}
    }
}

ThreadPool::~ThreadPool()
{
	if(m_exit_pthread_handler)
		(*m_exit_pthread_handler)();
    
    if(m_arg)
        free(m_arg);
}
