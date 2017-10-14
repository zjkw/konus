#pragma once

#include <memory>
#include <atomic>
#include <chrono>
#include "thread_safe_objbase.h"
#include "tcp_channel_base.h"
#include "timer_helper.h"
#include "reactor_loop.h"

//connect��Ҫ�Ѵ���(EINTR/EINPROGRESS/EAGAIN)����Fatal.

class tcp_client_callback;
class tcp_client_channel : public std::enable_shared_from_this<tcp_client_channel>, public thread_safe_objbase, public sockio_channel, public tcp_channel_base
{
public:
	enum CONN_STATE
	{
		CNS_CLOSED = 0,
		CNS_CONNECTING = 2,
		CNS_CONNECTED = 3,
	};
	tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::shared_ptr<tcp_client_callback> cb,
		std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_client_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������
	void		close();
	void		shutdown(int32_t howto);// �����ο�ȫ�ֺ��� ::shutdown
	void		connect();
	CONN_STATE	get_state()	{ return _conn_state; }
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	SOCKET									_conn_fd;
	std::atomic<CONN_STATE>					_conn_state;
	std::string								_server_addr;
	std::shared_ptr<reactor_loop>			_reactor;
	std::shared_ptr<tcp_client_callback>	_cb;

	std::chrono::seconds					_connect_timeout;	//����ʱ����ִ��connect�ҵȴ�״̬�£���connect����ڳ�ʱǰ���������ص���ʱ��������������ǿ���л���CLOSED
	std::chrono::seconds					_connect_retry_wait;//����ʱ�����Ѿ���ȷconnectʧ��/��ʱ�������������connectʱ��ص���ʱ���������������Զ�ִ��connect
	timer_helper							_timer_connect_timeout;
	timer_helper							_timer_connect_retry_wait;
	void									on_timer_connect_timeout(timer_helper* timer_id);
	void									on_timer_connect_retry_wait(timer_helper* timer_id);

	void		invalid();

	virtual void on_sockio_read();
	virtual void on_sockio_write();
	virtual SOCKET	get_fd() { return _fd; }

	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);
};

// ���ܴ��ڶ��̻߳�����
class tcp_client_callback : public std::enable_shared_from_this<tcp_client_callback>, public thread_safe_objbase
{
public:
	tcp_client_callback(){}
	virtual ~tcp_client_callback(){}

	//override------------------
	virtual void	on_connect(std::shared_ptr<tcp_client_channel> channel) = 0;
	virtual void	on_closed(std::shared_ptr<tcp_client_channel> channel) = 0;
	//�ο�TCP_ERROR_CODE����
	virtual void	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<tcp_client_channel> channel) = 0;
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len, std::shared_ptr<tcp_client_channel> channel) = 0;
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, std::shared_ptr<tcp_client_channel> channel) = 0;
};