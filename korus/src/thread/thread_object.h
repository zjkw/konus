#pragma once

#include <assert.h>
#include <condition_variable>
#include <thread>
#include <mutex>

#include "korus/src/util/object_state.h"
#include "korus/src/util/task_queue.h"

//֧�ֳ�פ�������(���ṩ��ʽ�����ӿ�)
//֧��һ�����������
//thread_object�Ĵ��������ٱ�����ͬһ���߳�

class thread_object : public noncopyable
{
public:
	explicit thread_object(uint16_t index);
	virtual ~thread_object();

	//�̳߳�פ����ʱ����
	void add_resident_task(const async_task_t& task)
	{
		add_task_helper(&_resident_task_queue, task);
	}
	void add_disposible_task(const async_task_t& task)
	{
		add_task_helper(&_disposible_task_queue, task);
	}
	//�̳߳�ʼ���˳�����
	void add_init_task(const async_task_t& task)
	{
		add_task_helper(&_init_task_queue, task, false);
	}
	void add_exit_task(const async_task_t& task)
	{
		add_task_helper(&_exit_task_queue, task, false);
	}
	void start();
	void clear_regual_task();
	
private:
	uint16_t						_thread_index;

	bool							_is_start;
	std::condition_variable			_cond_start;			//�����������ȴ������̴߳������
	std::mutex						_mutex_start;

	bool							_is_quit;				//��join�����ȴ�
	bool							_is_main_loop_exit;		//�˳���ѭ������Ҫ���߳��ڲ��Ǽ�/����

	bool							_is_taskempty;
	std::condition_variable			_cond_taskempty;		//������ʱ�����߳������ȴ�
	std::mutex						_mutex_taskempty;

	task_queue						_resident_task_queue;
	task_queue						_disposible_task_queue;

	task_queue						_init_task_queue;
	task_queue						_exit_task_queue;
	
	std::shared_ptr<std::thread>	_thread_ptr;
	
	std::thread::id					_tid;		

	void	thread_routine();
	void	clear();

	void add_task_helper(task_queue* tq, const async_task_t& task, bool wakeup = true)
	{
		std::unique_lock <std::mutex> lck(_mutex_taskempty);
		tq->add(task);

		if (wakeup)
		{
			if (_is_taskempty)
			{
				_is_taskempty = false;
				lck.unlock();
				_cond_taskempty.notify_one();
			}
		}
	}
};