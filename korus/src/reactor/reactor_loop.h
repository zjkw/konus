#pragma once

#include <memory>
#include <thread>
#include <vector>
#include <list>
#include "korus/src/util/basic_defines.h"
#include "korus/src/util/task_queue.h"
#include "korus/src/util/object_state.h"
#include "backend_poller.h"
#include "timer_sheduler.h"
#include "idle_distributor.h"

class reactor_loop : public std::enable_shared_from_this<reactor_loop>, public double_state
{
public:
	reactor_loop();
	virtual ~reactor_loop();
	
	void	run();	
	void	run_once();

	bool	is_current_thread();

	// ������������̣߳����뵽���������
	void	start_async_task(const async_task_t& task, const task_owner& owner);
	void	stop_async_task(const task_owner& owner);

	virtual void	invalid();

private:
	backend_poller*					_backend_poller;
	timer_sheduler*					_timer_sheduler;
	idle_distributor*				_idle_distributor;
	task_queue						_task_queue;
	std::thread::id					_tid;

	//Ϊ�˱����ⲿ����ֱ�ӵ��������timer/idleϵ�к������µĶ����ڲ�����״̬���������һ�£��ĳ�private����Ҫ���ⲿֻ����timer_helper/idle_helper������ֱ�ӵ���reactor_loop����
	friend timer_helper;
	// begin��ʾ��ʼʱ�䣬Ϊ0��ʾ�ޣ�interval��ʾ����ÿ����Ϊ0��ʾ��
	void	start_timer(timer_helper* timer_id, const std::chrono::system_clock::time_point& begin, const std::chrono::milliseconds& interval);
	void	start_timer(timer_helper* timer_id, const std::chrono::milliseconds& begin, const std::chrono::milliseconds& interval);
	void	stop_timer(timer_helper* timer_id);
	bool	exist_timer(timer_helper* timer_id);
	friend idle_helper;
	void	start_idle(idle_helper* idle_id);
	void	stop_idle(idle_helper* idle_id);
	bool	exist_idle(idle_helper* idle_id);

	// poll��ˣ����̰߳�ȫ��sockio_helperҲ���������ڲ�ʹ��
	friend	class sockio_helper;
	void start_sockio(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type);
	void update_sockio(sockio_helper* sockio_id, SOCKET fd, SOCKIO_TYPE type);
	void stop_sockio(sockio_helper* sockio_id, SOCKET fd);

	void run_once_inner();
};

template<typename T>
bool build_chain(std::shared_ptr<reactor_loop> reactor, T tail, const std::list<std::function<T()> >& chain)
{
	if (!tail)
	{
		return false;
	}
	std::vector<T> objs;
	typedef typename std::list<std::function<T()> >::const_iterator list_iterator;
	for (list_iterator it = chain.begin(); it != chain.end(); it++)
	{
		T t = (*it)();
		if (!t)
		{
			return false;
		}

		objs.push_back(t);
	}
	if (!objs.size())
	{
		return false;
	}
	objs.push_back(tail);

	for (size_t i = 0; i < objs.size(); i++)
	{
		T prev = i > 0 ? objs[i - 1] : nullptr;
		T next = i + 1 < objs.size() ? objs[i + 1] : nullptr;
		objs[i]->inner_init(reactor, prev, next);
	}

	return true;
}