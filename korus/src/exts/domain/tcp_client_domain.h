#pragma once

#include <mutex>
#include "korus/src/util/socket_ops.h"
#include "korus/src/tcp/tcp_client.h"
#include "domain_async_resolve_helper.h"
#include "domain_cache_mgr.h"

// ��Ӧ�ò�ɼ��ࣺtcp_client_channel, tcp_client_handler_base, reactor_loop, tcp_client_domain. ���������ڶ��̻߳����£����Զ�Ҫ����shared_ptr��װ�������������������
// tcp_client_domain���ṩ����channel�Ľӿڣ����������ڲ������ԣ�����channel��������Ҳֻ�ǲ���Ӧ�������ϲ��Լ��㶨

// ռ��
template<typename T>
class tcp_client_domain
{
public:
	tcp_client_domain(){}
	virtual ~tcp_client_domain(){}
};

// һ��tcp_client_domainӵ�ж��̣߳�Ӧ�ó����о����ࣿ���߳�����ͬһ�ļ���
template <>
class tcp_client_domain<uint16_t>
{
public:
	// addr��ʽip:port �� domain:port
	// ��thread_numΪ0��ʾĬ�Ϻ���
	// ֧����Ե�԰�
	// �ⲿʹ�� dynamic_pointer_cas �������������ָ��ת���� std::shared_ptr<tcp_client_handler_base>
	// connect_timeout ��ʾ���ӿ�ʼ��ú���û�յ�ȷ�������0��ʾ��Ҫ��ʱ����; 
	// connect_retry_wait ��ʾ֪������ʧ�ܻ�ʱ��Ҫ�ȶ�òſ�ʼ������Ϊ0��������Ϊ-1��ʾ������
	tcp_client_domain(uint16_t thread_num, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _thread_num(thread_num), _server_addr(server_addr), _factory(factory), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size), _native_tcp_client(nullptr), _thread_obj(nullptr)
	{
		_tid = std::this_thread::get_id();
	}

	virtual ~tcp_client_domain()
	{
		delete _thread_obj;
		delete _native_tcp_client;
	}

	virtual void start()
	{
		//����ͬ���̣߳��������ݹ������⣺����/��������start����һ���߳̽���ʹ�����⸴�ӻ�������tcp_server�Ƿ����ü����ģ������ڽ���/����ʱ��ͬʱ�������������̵߳���start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}

		std::string host, port;
		if (!sockaddr_from_string(_server_addr, host, port))
		{
			assert(false);
			return;
		}

		SOCK_ADDR_TYPE	sat = addrtype_from_string(host);
		if (sat == SAT_DOMAIN)
		{
			std::string ip;
			if (query_ip_by_domain(host, ip))
			{
				create_native_tcp_client(ip + ":" + port);
			}
			else
			{
				if (!_thread_obj)
				{
					//����һ���߳�
					inner_init();

					assert(_thread_num);
					int32_t cpu_num = sysconf(_SC_NPROCESSORS_CONF);
					assert(cpu_num);
					int32_t	offset = rand() % cpu_num;

					_thread_obj = new thread_object(abs(offset) % cpu_num);

					_thread_obj->add_init_task(std::bind(&tcp_client_domain::tcp_client_domain::thread_init, this, _thread_obj, host, port));
					_thread_obj->start();
				}
			}
		}
		else if (sat == SAT_IPV4)
		{
			create_native_tcp_client(_server_addr);
		}
		else
		{
			assert(false);
			return;
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

	void thread_init(thread_object*	thread_obj, const std::string& host, const std::string& port)
	{
		std::shared_ptr<reactor_loop>		reactor = std::make_shared<reactor_loop>();
		domain_async_resolve_helper*		resolve = new domain_async_resolve_helper(reactor.get());
		resolve->bind(std::bind(&tcp_client_domain::on_resolve_result, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		thread_obj->add_exit_task(std::bind(&tcp_client_domain::tcp_client_domain::thread_exit, this, thread_obj, reactor, resolve));
		
		std::string ip;
		DOMAIN_RESOLVE_STATE state = resolve->start(host, ip);	//���ӳɹ���ǿ�Ƹ���
		if (DRS_SUCCESS == state)
		{
			create_native_tcp_client(ip + ":" + port);
		}
		else if (DRS_PENDING == state)
		{
			thread_obj->add_resident_task(std::bind(&reactor_loop::run_once, reactor));
		}
		else
		{
			thread_obj->clear_regual_task();
		}
	}
	void thread_exit(thread_object*	thread_obj, std::shared_ptr<reactor_loop> reactor, domain_async_resolve_helper*	resolve)
	{
		delete resolve;

		reactor->invalid();
	}

	void	on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip)
	{
		_thread_obj->clear_regual_task();

		if (result == DRS_SUCCESS)
		{
			std::string host, port;
			if (!sockaddr_from_string(_server_addr, host, port))
			{
				assert(false);
				return;
			}
			printf("on_resolve_result result: %d, domain: %s, ip: %s\n", (int)result, domain.c_str(), ip.c_str());
			create_native_tcp_client(ip + ":" + port);
		}
		else
		{
			//tbd...
		}
	}

private:
	tcp_client<uint16_t>*					_native_tcp_client;
	std::mutex								_mutex;
	std::thread::id							_tid;
	thread_object*							_thread_obj;
	uint16_t								_thread_num;
	tcp_client_channel_factory_t			_factory;
	std::string								_server_addr;
	std::chrono::seconds					_connect_timeout;
	std::chrono::seconds					_connect_retry_wait;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	void create_native_tcp_client(const std::string& addr)
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (!_native_tcp_client)
		{
			_native_tcp_client = new tcp_client<uint16_t>(_thread_num, addr, _factory, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			_native_tcp_client->start();
		}
	}
};

// һ���߳�ӵ�ж��tcp_server
template <>
class tcp_client_domain<reactor_loop>
{
public:
	// addr��ʽip:port �� domain:port
	tcp_client_domain(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _reactor(reactor), _server_addr(server_addr), _factory(factory), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
		_self_read_size(self_read_size), _self_write_size(self_write_size), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size), _native_tcp_client(nullptr), _resolve(nullptr)
	{
		_tid = std::this_thread::get_id();
	}

	virtual ~tcp_client_domain()
	{
		delete _resolve;
		delete _native_tcp_client;
	}
	
	virtual void start()
	{
		//����ͬ���̣߳��������ݹ������⣺����/��������start����һ���߳̽���ʹ�����⸴�ӻ�������tcp_server�Ƿ����ü����ģ������ڽ���/����ʱ��ͬʱ�������������̵߳���start
		if (_tid != std::this_thread::get_id())
		{
			assert(false);
			return;
		}

		std::string host, port;
		if (!sockaddr_from_string(_server_addr, host, port))
		{
			assert(false);
			return;
		}

		SOCK_ADDR_TYPE	sat = addrtype_from_string(host);
		if (sat == SAT_DOMAIN)
		{
			std::string ip;
			if (query_ip_by_domain(host, ip))
			{
				create_native_tcp_client(ip + ":" + port);
			}
			else
			{
				if (!_resolve)
				{
					_resolve = new domain_async_resolve_helper(_reactor.get());
					_resolve->bind(std::bind(&tcp_client_domain::on_resolve_result, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
					
					DOMAIN_RESOLVE_STATE state = _resolve->start(host, ip);	//���ӳɹ���ǿ�Ƹ���
					if (DRS_SUCCESS == state)
					{
						create_native_tcp_client(ip + ":" + port);
					}
				}
			}
		}
		else if (sat == SAT_IPV4)
		{
			create_native_tcp_client(_server_addr);
		}
		else
		{
			assert(false);
			return;
		}
	}

protected:
	void	on_resolve_result(DOMAIN_RESOLVE_STATE result, const std::string& domain, const std::string& ip)
	{
		if (result == DRS_SUCCESS)
		{
			std::string host, port;
			if (!sockaddr_from_string(_server_addr, host, port))
			{
				assert(false);
				return;
			}

			create_native_tcp_client(ip + ":" + port);
		}
		else
		{
			//tbd...
		}
	}


private:
	tcp_client<reactor_loop>*				_native_tcp_client;
	std::shared_ptr<reactor_loop>			_reactor;
	std::thread::id							_tid;
	std::string								_server_addr;
	tcp_client_channel_factory_t			_factory;
	std::chrono::seconds					_connect_timeout;
	std::chrono::seconds					_connect_retry_wait;
	uint32_t								_self_read_size;
	uint32_t								_self_write_size;
	uint32_t								_sock_read_size;
	uint32_t								_sock_write_size;

	domain_async_resolve_helper*			_resolve;

	void create_native_tcp_client(const std::string& addr)
	{
		if (!_native_tcp_client)
		{
			_native_tcp_client = new tcp_client<reactor_loop>(_reactor, addr, _factory, _connect_timeout, _connect_retry_wait, _self_read_size, _self_write_size, _sock_read_size, _sock_write_size);
			_native_tcp_client->start();
		}
	}
};