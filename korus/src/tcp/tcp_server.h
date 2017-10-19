#pragma once

#include <unistd.h>
#include <sys/sysinfo.h> 
#include "tcp_listen.h"
#include "tcp_server_channel_creator.h"

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
#ifdef REUSEPORT_TRADITION
				, _listen_thread(nullptr), _is_listen_init(false), _alone_listen(nullptr), _num_worker_ready(0)
#endif
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
#ifdef REUSEPORT_TRADITION
		//����ֻ��tcp_server���ã�������������
		_listen_thread->invalid();		
		_listen_thread = nullptr;
#endif
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
			int32_t	offset = rand();
			
#ifdef REUSEPORT_TRADITION
			// ����1������listen�߳�
			_listen_thread = std::make_shared<thread_object>(abs((offset++) % cpu_num));

			_listen_thread->add_init_task(std::bind(&tcp_server::listen_thread_init, this, _listen_thread, _cb, offset));
			_listen_thread->start();	
			if (1 != _thread_num)
			{
				{
					std::unique_lock <std::mutex> lck(_mutex_listen_init);
					while (!_is_listen_init)
					{
						_cond_listen_init.wait(lck);
					}
				}
				_num_worker_ready = _thread_num;
				tcp_listen* listen = _alone_listen;
#else
				tcp_listen* listen = nullptr;
#endif
				
				// ����2�����������߳�
				for (uint16_t i = 0; i < _thread_num; i++)
				{
					std::shared_ptr<thread_object>	thread_obj = std::make_shared<thread_object>(abs((i + offset) % cpu_num));
					_thread_pool[i] = thread_obj;

					thread_obj->add_init_task(std::bind(&tcp_server::common_thread_init, this, thread_obj, _cb, listen));
					thread_obj->start();
				}
#ifdef REUSEPORT_TRADITION
			}
#endif
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

	// �������������������Բ���shared_ptr����ͬ
#ifdef REUSEPORT_TRADITION				
	std::shared_ptr<thread_object>			_listen_thread;
	tcp_listen*								_alone_listen;				//listen�̴߳��������̲߳�������

	bool									_is_listen_init;
	std::condition_variable					_cond_listen_init;			//�����������ȴ����������߳�init���
	std::mutex								_mutex_listen_init;

	uint16_t								_num_worker_ready;
	std::condition_variable					_cond_worker_ready;			//�����������ȴ������߳�start���
	std::mutex								_mutex_worker_ready;
#endif

#ifdef REUSEPORT_TRADITION
	void listen_thread_init(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<tcp_server_callback> cb, int32_t offset)
	{
		std::shared_ptr<reactor_loop>	reactor = std::make_shared<reactor_loop>(thread_obj);
		_alone_listen = new tcp_listen(reactor, _listen_addr, _backlog, _defer_accept);
		tcp_server_channel_creator*		creator = nullptr;
		if(1 == _thread_num)	//�͸���ͬһ���̺߳���
		{
			creator = new tcp_server_channel_creator(reactor, cb, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			_alone_listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
		}

		thread_obj->add_exit_task(std::bind(&tcp_server::listen_thread_exit, this, thread_obj, reactor, _alone_listen, creator));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));

		// ֪ͨ�����̣�listen�̳߳�ʼ�����
		_is_listen_init = true;
		_cond_listen_init.notify_one();

		// �ȴ������̳߳�ʼ�����
		if(1 != _thread_num)
		{
			std::unique_lock <std::mutex> lck(_mutex_worker_ready);
			while (!_num_worker_ready)
			{
				_cond_worker_ready.wait(lck);
			}
		}

		_alone_listen->start();
	}
	void listen_thread_exit(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<reactor_loop> reactor, tcp_listen* listen, tcp_server_channel_creator* creator)
	{
		assert(_alone_listen == listen);
		delete listen;		
		_alone_listen = nullptr;
		delete creator;     // ����Ϊ�գ��� "1 !=_thread_num" ʱ
		reactor->invalid();	// �����ϲ㻹���ּ�ӻ�ֱ�����ã�����ʹ��ʧЧ����ֻ�ܹ���ʧЧ���������������ͷš�
	}
#endif
	void common_thread_init(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<tcp_server_callback> cb, tcp_listen*	alone_listen)
	{
		std::shared_ptr<reactor_loop>	reactor = std::make_shared<reactor_loop>(thread_obj);
		tcp_server_channel_creator*		creator = new tcp_server_channel_creator(reactor, cb, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
		tcp_listen*						listen = nullptr;
		if (!alone_listen)
		{
			listen = new tcp_listen(reactor, _listen_addr, _backlog, _defer_accept);
			listen->start();
			listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
		}
		else
		{
			alone_listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
		}

		thread_obj->add_exit_task(std::bind(&tcp_server::common_thread_exit, this, thread_obj, reactor, listen, creator));
		thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
				
#ifdef REUSEPORT_TRADITION
		std::unique_lock <std::mutex> lck(_mutex_worker_ready);
		_num_worker_ready--;
		_cond_worker_ready.notify_one();
#endif
	}
	void common_thread_exit(std::shared_ptr<thread_object>	thread_obj, std::shared_ptr<reactor_loop> reactor, tcp_listen* listen, tcp_server_channel_creator*	creator)
	{
		delete listen;		
		delete creator;
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
		_creator = new tcp_server_channel_creator(reactor, cb, self_read_size, self_write_size, sock_read_size, sock_write_size);
		_listen = new tcp_listen(reactor, listen_addr, backlog, defer_accept);
		_listen->add_accept_handler(std::bind(&tcp_server_channel_creator::on_newfd, _creator, std::placeholders::_1, std::placeholders::_2)); //fd + sockaddr_in
	}

	virtual ~tcp_server()
	{
		delete _listen;		
		delete _creator;
	}
	
private:
	tcp_listen*								_listen;
	tcp_server_channel_creator*				_creator;
};