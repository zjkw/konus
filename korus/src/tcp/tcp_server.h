#pragma once

#include <unistd.h>
#include <sys/sysinfo.h> 
#include "tcp_listen.h"

#define DEFAULT_LISTEN_BACKLOG	20
#define DEFAULT_DEFER_ACCEPT	3

// ��Ӧ�ò�ɼ��ࣺtcp_server_channel, tcp_server_callback, reactor_loop, tcp_server. ���������ڶ��̻߳����£����Զ�Ҫ����shared_ptr��װ�������������������
// tcp_server���ṩ����channel�Ľӿڣ����������ڲ������ԣ�����channel��������Ҳֻ�ǲ���Ӧ�������ϲ��Լ��㶨

// ռ��
template<typename T>
class tcp_server
{
public:
	tcp_server(){}
	virtual ~tcp_server(){}
};

// һ��tcp_serverӵ�ж��߳�
template <>
class tcp_server<uint16_t>
{
public:
	// addr��ʽip:port
	// ��thread_numΪ0��ʾĬ�Ϻ���
	// ֧����Ե�԰�
	// �ⲿʹ�� dynamic_pointer_cast �������������ָ��ת���� std::shared_ptr<tcp_server_callback>
	tcp_server(	uint16_t thread_num, const std::string& listen_addr, std::shared_ptr<tcp_server_callback> cb, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT, 
				const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
				: _thread_num(thread_num), _listen_addr(listen_addr), _cb(cb), _backlog(backlog), _defer_accept(defer_accept),
				_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size)
	{
		_tid = std::this_thread::get_id();
		if (!_thread_num)
		{
			_thread_num = (uint16_t)sysconf(_SC_NPROCESSORS_CONF);
		}
		srand(time(NULL));
	}

	virtual ~tcp_server()
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
		//atomic::test_and_set���flag�Ƿ����ã���������ֱ�ӷ���true����û������������flagΪtrue���ٷ���false
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

				thread_obj->add_init_task(std::bind(&tcp_server::thread_init, this, thread_obj, _cb));
				thread_obj->start();
			}
			_cb = nullptr;//����tcp_server_callback��tcp_server��������
		}
	}

private:
	std::thread::id							_tid;
	uint16_t								_thread_num;
	std::map<uint16_t, std::shared_ptr<thread_object>>		_thread_pool;
	std::atomic_flag						_start = ATOMIC_FLAG_INIT;
	std::shared_ptr<tcp_server_callback>	_cb;
	std::string								_listen_addr;
	uint32_t								_backlog;
	uint32_t								_defer_accept;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void thread_init(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<tcp_server_callback> cb)
	{
		std::shared_ptr<reactor_loop>	reactor = std::make_shared<reactor_loop>(thread_obj);
		tcp_listen*						listen = new tcp_listen(reactor);

		thread_obj->add_exit_task(std::bind(&tcp_server::thread_exit, this, thread_obj, reactor, listen));

		listen->add_listen(this, _listen_addr, cb, _backlog, _defer_accept, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
	}
	void thread_exit(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<reactor_loop> reactor, tcp_listen* listen)
	{
		delete listen;		//listenʼ����reactor�̣߳�����ɾ����û�����
		reactor->invalid();	//�����ϲ㻹���ּ�ӻ�ֱ�����ã�����ʹ��ʧЧ����ֻ�ܹ���ʧЧ���������������ͷš�
	}
};

// һ���߳�ӵ�ж��tcp_server
template <>
class tcp_server<reactor_loop>
{
public:
	// addr��ʽip:port
	tcp_server(std::shared_ptr<reactor_loop> reactor, const std::string& listen_addr, std::shared_ptr<tcp_server_callback> cb, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
	{
		_listen = new tcp_listen(reactor);
		_listen->add_listen(this, listen_addr, cb, backlog, defer_accept, self_read_size, self_write_size, sock_read_size, sock_write_size);
	}

	virtual ~tcp_server()
	{
		delete _listen;
		_listen = nullptr;
	}

private:
	tcp_listen*								_listen;
};