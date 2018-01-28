#pragma once

#include <map>
#include <memory>
#include <thread>
#include "korus/src/thread/thread_object.h"
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_client_channel.h"

// ��Ӧ�ò�ɼ��ࣺtcp_client_handler_origin, tcp_client_handler_base, reactor_loop, tcp_client. ���������ڶ��̻߳����£����Զ�Ҫ����shared_ptr��װ�������������������
// tcp_client���ṩ����channel�Ľӿڣ����������ڲ������ԣ�����channel��������Ҳֻ�ǲ���Ӧ�������ϲ��Լ��㶨

// ռ��
template<typename T>
class tcp_client
{
public:
	tcp_client(){}
	virtual ~tcp_client(){}
};

// һ��tcp_clientӵ�ж��̣߳�Ӧ�ó����о����ࣿ���߳�����ͬһ�ļ���
template <>
class tcp_client<uint16_t>
{
public:
	// addr��ʽip:port
	// ��thread_numΪ0��ʾĬ�Ϻ���
	// ֧����Ե�԰�
	// �ⲿʹ�� dynamic_pointer_cas �������������ָ��ת���� std::shared_ptr<tcp_client_handler_base>
	// connect_timeout ��ʾ���ӿ�ʼ��ú���û�յ�ȷ�������0��ʾ��Ҫ��ʱ����; 
	// connect_retry_wait ��ʾ֪������ʧ�ܻ�ʱ��Ҫ�ȶ�òſ�ʼ������Ϊ0��������Ϊ-1��ʾ������
	tcp_client(uint16_t thread_num, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _server_addr(server_addr), _factory(factory), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
	}

	virtual ~tcp_client()
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
		//����ͬ���̣߳��������ݹ������⣺����/��������start����һ���߳̽���ʹ�����⸴�ӻ�������tcp_server�Ƿ����ü����ģ������ڽ���/����ʱ��ͬʱ�������������̵߳���start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic_flag::test_and_set���flag�Ƿ����ã���������ֱ�ӷ���true����û������������flagΪtrue���ٷ���false
		if (!_start.test_and_set())
		{
			inner_init();

			assert(_thread_num);
			int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
			assert(cpu_num);
			int32_t	offset = rand() % cpu_num;

			for (uint16_t i = 0; i < _thread_num; i++)
			{
				thread_object*	thread_obj = new thread_object(abs((i + offset) % cpu_num));
				_thread_pool[i] = thread_obj;

				thread_obj->add_init_task(std::bind(&tcp_client::thread_init, this, thread_obj));
				thread_obj->start();
			}
		}
	}

protected:
	void inner_init()
	{
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
	}
	tcp_client_handler_origin*			create_origin_channel(std::shared_ptr<reactor_loop> reactor)
	{
		return new tcp_client_handler_origin(reactor, _server_addr, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
	}
	std::shared_ptr<tcp_client_handler_base>	create_terminal_channel(std::shared_ptr<reactor_loop> reactor)
	{
		if (!_factory)
		{
			return nullptr;
		}
		std::shared_ptr<tcp_client_handler_base>	channel = _factory(reactor);
		return channel;
	}
	virtual std::shared_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*			origin_channel	=	create_origin_channel(reactor);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);
		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)terminal_channel.get());
		origin_channel->connect();

		return terminal_channel;
	}

	std::thread::id							_tid;
	uint16_t								_thread_num;
	std::map<uint16_t, thread_object*>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	tcp_client_channel_factory_t			_factory;
	std::string								_server_addr;
	std::chrono::seconds					_connect_timeout;
	std::chrono::seconds					_connect_retry_wait;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;
	
	virtual void thread_init(thread_object*	thread_obj)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = build_channel_chain(reactor);
		
		thread_obj->add_exit_task(std::bind(&tcp_client::thread_exit, this, thread_obj, reactor, terminal_channel));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_client_handler_base>	terminal_channel)
	{
		reactor->invalid();
	}
};

// һ���߳�ӵ�ж��tcp_server
template <>
class tcp_client<reactor_loop>
{
public:
	// addr��ʽip:port
	tcp_client(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _reactor(reactor), _server_addr(server_addr), _factory(factory), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
	}

	virtual ~tcp_client()
	{

	}
	
	virtual void start()
	{
		//����ͬ���̣߳��������ݹ������⣺����/��������start����һ���߳̽���ʹ�����⸴�ӻ�������tcp_server�Ƿ����ü����ģ������ڽ���/����ʱ��ͬʱ�������������̵߳���start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}
		//atomic_flag::test_and_set���flag�Ƿ����ã���������ֱ�ӷ���true����û������������flagΪtrue���ٷ���false
		if (!_start.test_and_set())
		{
			// ������ִ��::connect�������ⲿ�ֶ�����
			init_object(_reactor);
			
		}
	}
	
protected:
	virtual void init_object(std::shared_ptr<reactor_loop> reactor)
	{
		_channels = build_channel_chain(_reactor);
	}

	tcp_client_handler_origin*			create_origin_channel(std::shared_ptr<reactor_loop> reactor)
	{
		return new tcp_client_handler_origin(reactor, _server_addr, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
	}
	std::shared_ptr<tcp_client_handler_base>	create_terminal_channel(std::shared_ptr<reactor_loop> reactor)
	{
		if (!_factory)
		{
			return nullptr;
		}
		std::shared_ptr<tcp_client_handler_base>	channel = _factory(reactor);
		return channel;
	}
	std::shared_ptr<tcp_client_handler_base> build_channel_chain(std::shared_ptr<reactor_loop> reactor)
	{
		tcp_client_handler_origin*			origin_channel = create_origin_channel(reactor);
		std::shared_ptr<tcp_client_handler_base>	terminal_channel = create_terminal_channel(reactor);
		build_channel_chain_helper((tcp_client_handler_base*)origin_channel, (tcp_client_handler_base*)terminal_channel.get());
		terminal_channel->connect();

		return terminal_channel;
	}

	std::shared_ptr<reactor_loop>			_reactor;
	std::thread::id							_tid;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::string								_server_addr;
	tcp_client_channel_factory_t			_factory;
	std::chrono::seconds					_connect_timeout;
	std::chrono::seconds					_connect_retry_wait;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	std::shared_ptr<tcp_client_handler_base>		_channels;
};