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

//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//�ⲿ���ȷ��tcp_client�ܰ���channel/handler�����ڣ������ܱ�֤��Դ���գ����������õ�����(channel��handler)û�п���ƵĽ�ɫ����������ʱ���ĺ�������check_detach_relation

enum TCP_CLTCONN_STATE
{
	CNS_CLOSED = 0,
	CNS_CONNECTING = 2,
	CNS_CONNECTED = 3,
};
//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���

class tcp_client_channel : public std::enable_shared_from_this<tcp_client_channel>, public thread_safe_objbase, public tcp_channel_base
{
public:
	tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::shared_ptr<tcp_client_handler_base> cb,
		std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_client_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	void		close();
	void		shutdown(int32_t howto);// �����ο�ȫ�ֺ��� ::shutdown
	void		connect();
	TCP_CLTCONN_STATE	get_state()	{ return _conn_state; }
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	SOCKET									_conn_fd;
	std::atomic<TCP_CLTCONN_STATE>			_conn_state;
	std::string								_server_addr;
	std::shared_ptr<reactor_loop>			_reactor;
	std::shared_ptr<tcp_client_handler_base>	_cb;

	std::chrono::seconds					_connect_timeout;	//����ʱ����ִ��connect�ҵȴ�״̬�£���connect����ڳ�ʱǰ���������ص���ʱ��������������ǿ���л���CLOSED
	std::chrono::seconds					_connect_retry_wait;//����ʱ�����Ѿ���ȷconnectʧ��/��ʱ�������������connectʱ��ص���ʱ���������������Զ�ִ��connect
	timer_helper							_timer_connect_timeout;
	timer_helper							_timer_connect_retry_wait;
	void									on_timer_connect_timeout(timer_helper* timer_id);
	void									on_timer_connect_retry_wait(timer_helper* timer_id);

	template<typename T> friend class tcp_client;
	// channelҪ����/����Ҫ��handler�Ƿ�û���������������
	bool		check_detach_relation(long call_ref_count);	//true��ʾ�Ѿ���������ϵ
	void		invalid();

	sockio_helper	_sockio_helper_connect;
	sockio_helper	_sockio_helper;
	virtual void on_sockio_write_connect(sockio_helper* sockio_id);
	virtual void on_sockio_read(sockio_helper* sockio_id);
	virtual void on_sockio_write(sockio_helper* sockio_id);
	
	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class tcp_client_handler_base : public std::enable_shared_from_this<tcp_client_handler_base>, public thread_safe_objbase
{
public:
	tcp_client_handler_base() : _reactor(nullptr), _channel(nullptr){}
	virtual ~tcp_client_handler_base(){ assert(!_reactor); assert(!_channel); }

	//override------------------
	virtual void	on_init() = 0;
	virtual void	on_final() = 0;
	virtual void	on_connect() = 0;
	virtual void	on_closed() = 0;
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code) = 0;
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len) = 0;
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len) = 0;

protected:
	int32_t	send(const void* buf, const size_t len)	{ if (!_channel) return CEC_INVALID_SOCKET; return _channel->send(buf, len); }
	void	close()									{ if (_channel)_channel->close(); }
	void	shutdown(int32_t howto)					{ if (_channel)_channel->shutdown(howto); }
	void	connect()								{ if (_channel)_channel->connect(); }
	TCP_CLTCONN_STATE	get_state()					{ if (!_channel) return CNS_CLOSED;	return _channel->get_state(); }
	std::shared_ptr<reactor_loop>	get_reactor()	{ return _reactor; }

private:
	template<typename T> friend class tcp_client;
	friend class tcp_client_channel;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_client_channel> channel)
	{
		_reactor = reactor;
		_channel = channel;

		on_init();
	}
	void	inner_final()
	{
		_reactor = nullptr;
		_channel = nullptr;

		on_final();
	}

	std::shared_ptr<reactor_loop>		_reactor;
	std::shared_ptr<tcp_client_channel> _channel;
};