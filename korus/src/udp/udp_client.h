#pragma once

#include <memory>
#include <unistd.h>
#include <sys/sysinfo.h> 
#include "korus/src/thread/thread_object.h"
#include "udp_client_channel.h"

// ��Ӧ�ò�ɼ��ࣺudp_client_handler_origin, udp_client_handler_base, reactor_loop, udp_client. ���������ڶ��̻߳����£����Զ�Ҫ����shared_ptr��װ�������������������
// udp_client���ṩ����channel�Ľӿڣ����������ڲ������ԣ�����channel��������Ҳֻ�ǲ���Ӧ�������ϲ��Լ��㶨
// ����ж����ʵ������ͬ����ip�˿ڣ���Ҫע�⵱ĳ���̹ҵ���ԭ�Ⱥ�֮ͨ�ŵĶԶ˶��󽫺��ִ�ĳ����ͨ�ţ����ϸ�Ҫ��������߼�����ӳ��ʱ����Ҫ��ת����ܾ�����

// ռ��
template<typename T>
class udp_client
{
public:
	udp_client(){}
	virtual ~udp_client(){}
};

// һ��udp_clientӵ�ж��̣߳�Ӧ�ó����о����ࣿ���߳�����ͬһ�ļ���
template <>
class udp_client<uint16_t>
{
public:
	// addr��ʽip:port
	// ֧��reuseportʱ�����thread_num����Ϊ0��ʾĬ�Ϻ������Զ���Ե�԰�
	// �ⲿʹ�� dynamic_pointer_cast �������������ָ��ת���� std::shared_ptr<udp_client_handler_base>
	// sock_write_size������д���׽ӿڵ�UDP���ݱ��Ĵ�С���ޣ�������tcp����	
	// ��ʹ��reuseport����£�ע��������Ƿ�Ҳ���������ѡ���Ϊ5Ԫ�����ȷ��һ��"����"
#ifndef REUSEPORT_OPTION
	udp_client(const udp_client_channel_factory_t& factory, const std::string& bind_addr = "", const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _bind_addr(bind_addr), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#else
	udp_client(uint16_t thread_num, const udp_client_channel_factory_t& factory, const std::string& bind_addr = "", const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _factory(factory), _bind_addr(bind_addr), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
#endif
	{
		_tid = std::this_thread::get_id();
	}

	virtual ~udp_client()
	{
		//����������Ϊ�����exit��ͻ std::unique_lock <std::mutex> lck(_mutex_pool);
		for (std::map<uint16_t, thread_object*>::iterator it = _thread_pool.begin(); it != _thread_pool.end(); it++)
		{
			delete it->second;
		}
		_thread_pool.clear();
	}

	virtual void start()
	{
		//����ͬ���̣߳��������ݹ������⣺����/��������start����һ���߳̽���ʹ�����⸴�ӻ�������udp_client�Ƿ����ü����ģ������ڽ���/����ʱ��ͬʱ�������������̵߳���start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic::test_and_set���flag�Ƿ����ã���������ֱ�ӷ���true����û������������flagΪtrue���ٷ���false
		if (!_start.test_and_set())
		{
			inner_init();
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

				thread_obj->add_init_task(std::bind(&udp_client::common_thread_init, this, thread_obj, _factory));
				thread_obj->start();
			}
		}
	}

protected:
	udp_client_channel_factory_t			_factory;
	void inner_init()
	{
#ifdef REUSEPORT_OPTION
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
#endif
	}

private:
	std::thread::id							_tid;
#ifdef REUSEPORT_OPTION
	uint16_t								_thread_num;
#endif
	std::map<uint16_t, thread_object*>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::string								_bind_addr;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void common_thread_init(thread_object*	thread_obj, const udp_client_channel_factory_t& factory)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		udp_client_handler_origin*	origin_channel = new udp_client_handler_origin(reactor, _bind_addr, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		std::shared_ptr<udp_client_handler_base>	terminal_channel = factory(reactor);
		build_channel_chain_helper((udp_client_handler_base*)origin_channel, (udp_client_handler_base*)terminal_channel.get());

		thread_obj->add_exit_task(std::bind(&udp_client::common_thread_exit, this, thread_obj, reactor, terminal_channel));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));

		origin_channel->start();
	}
	void common_thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_client_handler_base>	terminal_channel)
	{
		reactor->invalid();	//�����ϲ㻹���ּ�ӻ�ֱ�����ã�����ʹ��ʧЧ����ֻ�ܹ���ʧЧ���������������ͷš�
	}
};

// һ���߳�ӵ�ж��udp_client
template <>
class udp_client<reactor_loop>
{
public:
	// addr��ʽip:port
	udp_client(std::shared_ptr<reactor_loop> reactor, const udp_client_channel_factory_t& factory, const std::string& bind_addr = "", const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE,
		const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _reactor(reactor), _bind_addr(bind_addr), _self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();

	}

	virtual ~udp_client()
	{

	}
	virtual void start()
	{
		//����ͬ���̣߳��������ݹ������⣺����/��������start����һ���߳̽���ʹ�����⸴�ӻ�������udp_client�Ƿ����ü����ģ������ڽ���/����ʱ��ͬʱ�������������̵߳���start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic::test_and_set���flag�Ƿ����ã���������ֱ�ӷ���true����û������������flagΪtrue���ٷ���false
		if (!_start.test_and_set())
		{
			assert(_factory);

			udp_client_handler_origin* origin_channel = new udp_client_handler_origin(_reactor, _bind_addr, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			_channel = _factory(_reactor);
			build_channel_chain_helper((udp_client_handler_base*)origin_channel, (udp_client_handler_base*)_channel.get());
			_channel->start();
		}
	}

protected:
	udp_client_channel_factory_t			_factory;

private:
	std::thread::id							_tid;
	std::shared_ptr<reactor_loop>			_reactor;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::string								_bind_addr;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	std::shared_ptr<udp_client_handler_base>		_channel;
};