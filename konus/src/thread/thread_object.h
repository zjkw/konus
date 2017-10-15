#ifndef _THREADOBJ_H
#define _THREADOBJ_H

#include <condition_variable>
#include <thread>
#include <memory>
#include <mutex>

#include "..\util\thread_safe_objbase.h"
#include "..\util\task_queue.h"

//֧�ֳ�פ�������(���ṩ��ʽ�����ӿ�)
//֧��һ�����������
//thread_object�Ĵ��������ٱ�����ͬһ���߳�

class thread_object : public std::enable_shared_from_this<thread_object>, public thread_safe_objbase
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

	bool is_current_thread()			//�ⲿ��֤���߳������󣬲ſ��ܵ��ñ�����
	{ 
		assert(_is_start);
		return _tid == std::this_thread::get_id(); 
	}

	virtual void	invalid();

private:
	uint16_t						_thread_index;

	bool							_is_start;
	std::condition_variable			_cond_start;			//�����������ȴ������̴߳������
	std::mutex						_mutex_start;

	bool							_is_quit;				//��join�����ȴ�
			
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

	void add_task_helper(task_queue* tq, const async_task_t& task, bool wakeup = true)
	{
		if (!is_valid())
		{
			return;
		}
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

#endif