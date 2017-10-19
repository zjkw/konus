#pragma once

#include <map>
#include <memory>
#include <thread>
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_client_channel.h"

// ��Ӧ�ò�ɼ��ࣺtcp_client_channel, tcp_client_callback, reactor_loop, tcp_client. ���������ڶ��̻߳����£����Զ�Ҫ����shared_ptr��װ�������������������
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
	// �ⲿʹ�� dynamic_pointer_cas �������������ָ��ת���� std::shared_ptr<tcp_client_callback>
	// connect_timeout ��ʾ���ӿ�ʼ��ú���û�յ�ȷ�������0��ʾ��Ҫ��ʱ����; 
	// connect_retry_wait ��ʾ֪������ʧ�ܻ�ʱ��Ҫ�ȶ�òſ�ʼ������Ϊ0��������Ϊ-1��ʾ������
	tcp_client(uint16_t thread_num, const std::string& server_addr, std::shared_ptr<tcp_client_callback> cb, 
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _server_addr(server_addr), _cb(cb), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
	}

	virtual ~tcp_client()
	{
		//����������Ϊ�����exit��ͻ std::unique_lock <std::mutex> lck(_mutex_pool);
		for (std::map<uint16_t, std::shared_ptr<thread_object>>::iterator it = _thread_pool.begin(); it != _thread_pool.end(); it++)
		{
			it->second->invalid();
		}
		_thread_pool.clear();
	}

	void start()
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
			assert(_cb);

			assert(_thread_num);
			int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
			assert(cpu_num);
			int32_t	offset = rand() % cpu_num;

			for (uint16_t i = 0; i < _thread_num; i++)
			{
				std::shared_ptr<thread_object>	thread_obj = std::make_shared<thread_object>((i + offset) % cpu_num);
				_thread_pool[i] = thread_obj;

				thread_obj->add_init_task(std::bind(&tcp_client::thread_init, this, thread_obj, _cb));
				thread_obj->start();
			}
		}
	}

private:
	std::thread::id							_tid;
	uint16_t								_thread_num;
	std::map<uint16_t, std::shared_ptr<thread_object>>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::shared_ptr<tcp_client_callback>	_cb;
	std::string								_server_addr;
	std::chrono::seconds					_connect_timeout;
	std::chrono::seconds					_connect_retry_wait;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void thread_init(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<tcp_client_callback> cb)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>(thread_obj);
		std::shared_ptr<tcp_client_channel>	channel = std::make_shared<tcp_client_channel>(reactor, _server_addr, cb, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);

		thread_obj->add_exit_task(std::bind(&tcp_client::thread_exit, this, thread_obj, reactor, channel));
		channel->connect();//tbd ����ֵ log print
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void thread_exit(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_client_channel> channel)
	{
		channel->close();	//�����ϲ㻹���ּ�ӻ�ֱ�����ã�����ʹ��ʧЧ����ֻ�ܹ���ʧЧ���������������ͷš�
		reactor->invalid();	
	}
};

// һ���߳�ӵ�ж��tcp_server
template <>
class tcp_client<reactor_loop>
{
public:
	// addr��ʽip:port
	tcp_client(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::shared_ptr<tcp_client_callback> cb,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
	{
		// ������ִ��::connect�������ⲿ�ֶ�����
		_channel = std::make_shared<tcp_client_channel>(reactor, server_addr, cb, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size);
	}

	virtual ~tcp_client()
	{
		_channel->close();
		_channel = nullptr;
	}

private:
	std::shared_ptr<tcp_client_channel>	_channel;
};