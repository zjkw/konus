#pragma once

#include <stdint.h>
#include <functional>
#include <map>
#include <mutex>

using async_task_t = std::function<void()>;

//���̰߳�ȫ

class task_queue	
{
public:
	task_queue(bool persistence, bool nolock);
	virtual ~task_queue();

	void			add(const async_task_t& task);
	void			execute();
	bool			is_empty();
	void			clear();
	
private:
	bool								_persistence;//�Ƿ�־�
	bool								_nolock;

	uint64_t							_seq;
	std::map<uint64_t, async_task_t>	_task_list;
	std::recursive_mutex				_mutex;
};