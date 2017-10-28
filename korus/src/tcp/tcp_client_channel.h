#pragma once

#include <unistd.h>
#include <memory>
#include <atomic>
#include <chrono>
#include <list>
#include "korus/src/util/thread_safe_objbase.h"
#include "korus/src/reactor/timer_helper.h"
#include "korus/src/reactor/sockio_helper.h"
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_channel_base.h"

class tcp_client_handler_base;

using tcp_client_channel_factory_t = std::function<std::shared_ptr<tcp_client_handler_base>()>;
using tcp_client_channel_factory_chain_t = std::list<tcp_client_channel_factory_t>;

enum TCP_CLTCONN_STATE
{
	CNS_CLOSED = 0,
	CNS_CONNECTING = 2,
	CNS_CONNECTED = 3,
};

//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class tcp_client_handler_base : public std::enable_shared_from_this<tcp_client_handler_base>, public thread_safe_objbase
{
public:
	tcp_client_handler_base() : _reactor(nullptr), _tunnel_prev(nullptr), _tunnel_next(nullptr){}
	virtual ~tcp_client_handler_base(){ assert(!_tunnel_prev); assert(!_tunnel_next); }	// ����ִ��inner_final

	//override------------------
	virtual void	on_init(){}
	virtual void	on_final(){}
	virtual void	on_connect()	{ if (_tunnel_prev)_tunnel_prev->on_connect(); }
	virtual void	on_closed()	{ if (_tunnel_prev)_tunnel_prev->on_closed(); }
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code)	{ if (!_tunnel_prev) return CMS_INNER_AUTO_CLOSE; return _tunnel_prev->on_error(code); }
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len)	{ if (!_tunnel_prev) return 0; return _tunnel_prev->on_recv_split(buf, len); }
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len)	{ if (!_tunnel_prev) return; return _tunnel_prev->on_recv_pkg(buf, len); }

	virtual int32_t	send(const void* buf, const size_t len)	{ if (!_tunnel_next) return CEC_INVALID_SOCKET; return _tunnel_next->send(buf, len); }
	virtual void	close()									{ if (_tunnel_next)_tunnel_next->close(); }
	virtual void	shutdown(int32_t howto)					{ if (_tunnel_next)_tunnel_next->shutdown(howto); }
	virtual void	connect()								{ if (_tunnel_next)_tunnel_next->connect(); }
	virtual TCP_CLTCONN_STATE	get_state()					{ if (!_tunnel_next) return CNS_CLOSED;	return _tunnel_next->get_state(); }
	virtual std::shared_ptr<reactor_loop>	reactor()		{ return _reactor; }
	virtual bool	can_delete(long call_ref_count)			//��ͷ����б�Ķ������ôˣ���Ҫ����
	{
		if (is_valid())
		{
			return false;
		}
		long ref = 0;
		if (_tunnel_prev)
		{
			ref++;
		}
		if (_tunnel_next)
		{
			ref++;
		}
		if (call_ref_count + ref + 1 == shared_from_this().use_count())
		{
			// �������������ϲ�ѯ
			if (_tunnel_prev)
			{
				return _tunnel_prev->can_delete(0);
			}
			else
			{
				return true;
			}
		}

		return false;
	}
private:
	template<typename T> friend bool build_chain(std::shared_ptr<reactor_loop> reactor, T tail, const std::list<std::function<T()> >& chain);
	template<typename T> friend class tcp_client;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_client_handler_base> tunnel_prev, std::shared_ptr<tcp_client_handler_base> tunnel_next)
	{
		_reactor = reactor;
		_tunnel_prev = tunnel_prev;
		_tunnel_next = tunnel_next;

		on_init();
	}
	void	inner_final()
	{
		if (_tunnel_prev)
		{
			_tunnel_prev->inner_final();
		}
		_reactor = nullptr;
		_tunnel_prev = nullptr;
		_tunnel_next = nullptr;

		on_final();
	}

	std::shared_ptr<reactor_loop>		_reactor;
	std::shared_ptr<tcp_client_handler_base> _tunnel_prev;
	std::shared_ptr<tcp_client_handler_base> _tunnel_next;
};

//�ⲿ���ȷ��tcp_client�ܰ���channel/handler�����ڣ������ܱ�֤��Դ���գ����������õ�����(channel��handler)û�п���ƵĽ�ɫ����������ʱ���ĺ�������check_detach_relation

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���

class tcp_client_channel : public tcp_channel_base, public tcp_client_handler_base
{
public:
	tcp_client_channel(const std::string& server_addr, std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_client_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	virtual int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	virtual void		close();
	virtual void		shutdown(int32_t howto);// �����ο�ȫ�ֺ��� ::shutdown
	virtual void		connect();
	virtual TCP_CLTCONN_STATE	get_state()	{ return _conn_state; }

private:
	SOCKET									_conn_fd;
	std::atomic<TCP_CLTCONN_STATE>			_conn_state;
	std::string								_server_addr;

	std::chrono::seconds					_connect_timeout;	//����ʱ����ִ��connect�ҵȴ�״̬�£���connect����ڳ�ʱǰ���������ص���ʱ��������������ǿ���л���CLOSED
	std::chrono::seconds					_connect_retry_wait;//����ʱ�����Ѿ���ȷconnectʧ��/��ʱ�������������connectʱ��ص���ʱ���������������Զ�ִ��connect
	timer_helper							_timer_connect_timeout;
	timer_helper							_timer_connect_retry_wait;
	void									on_timer_connect_timeout(timer_helper* timer_id);
	void									on_timer_connect_retry_wait(timer_helper* timer_id);

	template<typename T> friend class tcp_client;
	void		invalid();

	sockio_helper	_sockio_helper_connect;
	sockio_helper	_sockio_helper;
	virtual void on_sockio_write_connect(sockio_helper* sockio_id);
	virtual void on_sockio_read(sockio_helper* sockio_id);
	virtual void on_sockio_write(sockio_helper* sockio_id);
	
	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
