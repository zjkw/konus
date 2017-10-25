#pragma once

#include <memory>
#include <thread>
#include "korus/src/util/basic_defines.h"
#include "korus/src/util/task_queue.h"
#include "korus/src/util/thread_safe_objbase.h"
#include "backend_poller.h"
#include "timer_sheduler.h"
#include "idle_distributor.h"

class reactor_loop : public std::enable_shared_from_this<reactor_loop>, public thread_safe_objbase
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

	//Ϊ�˱����ⲿ����ֱ�ӵ��������timer/idleϵ�к������µĶ����ڲ�����״̬���������һ�£��ĳ�private����Ҫ���ⲿֻ����timer_helper/idle_helper
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

	// poll���
	friend	class tcp_listen;
	friend	class tcp_server_channel;
	friend	class tcp_client_channel;
	friend	class udp_server_channel;
	friend	class udp_client_channel;
	// ��ʼ����
	void start_sockio(sockio_channel* channel, SOCKIO_TYPE type);
	// ���ڸ���
	void update_sockio(sockio_channel* channel, SOCKIO_TYPE type);
	// ɾ��
	void stop_sockio(sockio_channel* channel);

	void run_once_inner();
};