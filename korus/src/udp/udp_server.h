#pragma once

#include <memory>
#include <unistd.h>
#include <sys/sysinfo.h> 
#include "korus/src/thread/thread_object.h"
#include "udp_server_channel.h"

// ��Ӧ�ò�ɼ��ࣺudp_server_channel, udp_server_handler_base, reactor_loop, udp_server. ���������ڶ��̻߳����£����Զ�Ҫ����shared_ptr��װ�������������������
// udp_server���ṩ����channel�Ľӿڣ����������ڲ������ԣ�����channel��������Ҳֻ�ǲ���Ӧ�������ϲ��Լ��㶨
// ����ж����ʵ������ͬ����ip�˿ڣ���Ҫע�⵱ĳ���̹ҵ���ԭ�Ⱥ�֮ͨ�ŵĶԶ˶��󽫺��ִ�ĳ����ͨ�ţ����ϸ�Ҫ��������߼�����ӳ��ʱ����Ҫ��ת����ܾ�����

// ռ��
template<typename T>
class udp_server
{
public:
	udp_server(){}
	virtual ~udp_server(){}
};

using udp_server_channel_factory_t = std::function<std::shared_ptr<udp_server_handler_base>()>;

// һ��udp_serverӵ�ж��߳�
template <>
class udp_server<uint16_t>
{
public:
	// addr��ʽip:port
	// ֧��reuseportʱ�����thread_num����Ϊ0��ʾĬ�Ϻ������Զ���Ե�԰�
	// �ⲿʹ�� dynamic_pointer_cast �������������ָ��ת���� std::shared_ptr<udp_server_handler_base>
	// sock_write_size������д���׽ӿڵ�UDP���ݱ��Ĵ�С���ޣ�������tcp����	
#ifndef REUSEPORT_OPTION
	udp_server(const std::string& local_addr, const udp_server_channel_factory_t& factory, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		:	_local_addr(local_addr), _factory(factory), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#else
	udp_server(uint16_t thread_num, const std::string& local_addr, const udp_server_channel_factory_t& factory, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _local_addr(local_addr), _factory(factory), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#endif
	{
		_tid = std::this_thread::get_id();
#ifdef REUSEPORT_OPTION
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
#endif
	}

	virtual ~udp_server()
	{
		//����������Ϊ�����exit��ͻ std::unique_lock <std::mutex> lck(_mutex_pool);
		for (std::map<uint16_t, thread_object*>::iterator it = _thread_pool.begin(); it != _thread_pool.end(); it++)
		{
			delete it->second;
		}
		_thread_pool.clear();
	}

	void start()
	{
		//����ͬ���̣߳��������ݹ������⣺����/��������start����һ���߳̽���ʹ�����⸴�ӻ�������udp_server�Ƿ����ü����ģ������ڽ���/����ʱ��ͬʱ�������������̵߳���start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic::test_and_set���flag�Ƿ����ã���������ֱ�ӷ���true����û������������flagΪtrue���ٷ���false
		if (!_start.test_and_set())
		{
			assert(_factory);

			int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
			assert(cpu_num);
			int32_t	offset = rand();

#ifndef REUSEPORT_OPTION
			// ����1������listen�߳�
			for (uint16_t i = 0; i < 1; i++)
#else
			assert(_thread_num);
			for (uint16_t i = 0; i < _thread_num; i++)
#endif
			{
				thread_object*	thread_obj = new thread_object(abs((i + offset) % cpu_num));
				_thread_pool[i] = thread_obj;

				thread_obj->add_init_task(std::bind(&udp_server::common_thread_init, this, thread_obj, _factory));
				thread_obj->start();
			}
		}
	}

private:
	std::thread::id							_tid;
#ifdef REUSEPORT_OPTION
	uint16_t								_thread_num;
#endif
	std::map<uint16_t, thread_object*>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	udp_server_channel_factory_t			_factory;
	std::string								_local_addr;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void common_thread_init(thread_object*	thread_obj, const udp_server_channel_factory_t& factory)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		std::shared_ptr<udp_server_handler_base> cb = factory();
		std::shared_ptr<udp_server_channel>	channel = std::make_shared<udp_server_channel>(reactor, _local_addr, cb, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		cb->inner_init(reactor, channel);
		channel->start();
		
		thread_obj->add_exit_task(std::bind(&udp_server::common_thread_exit, this, thread_obj, reactor, channel));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void common_thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_server_channel>	channel)
	{
		channel->invalid();
		channel->check_detach_relation(1);
		reactor->invalid();	//�����ϲ㻹���ּ�ӻ�ֱ�����ã�����ʹ��ʧЧ����ֻ�ܹ���ʧЧ���������������ͷš�
	}
};

// һ���߳�ӵ�ж��udp_server
template <>
class udp_server<reactor_loop>
{
public:
	// addr��ʽip:port
	udp_server(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, const udp_server_channel_factory_t& factory, 
				const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
	{
		std::shared_ptr<udp_server_handler_base> cb = factory();
		_channel = std::make_shared<udp_server_channel>(reactor, local_addr, cb, self_read_size, self_write_size, sock_read_size, sock_write_size);
		cb->inner_init(reactor, _channel);
		_channel->start();
	}

	virtual ~udp_server()
	{
		_channel->invalid();
		_channel->check_detach_relation(1);
	}
	
private:
	std::shared_ptr<udp_server_channel>	_channel;
};