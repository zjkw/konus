#pragma once

#include <unistd.h>
#include <memory>
#include <atomic>
#include <chrono>
#include "korus/src/util/thread_safe_objbase.h"
#include "korus/src/reactor/timer_helper.h"
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_channel_base.h"

//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���
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
	int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
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
	virtual SOCKET	get_fd() { return CNS_CONNECTING == _conn_state ? _conn_fd : _fd; }

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class tcp_client_callback : public std::enable_shared_from_this<tcp_client_callback>, public thread_safe_objbase
{
public:
	tcp_client_callback(){}
	virtual ~tcp_client_callback(){}

	//override------------------
	virtual void	on_connect(std::shared_ptr<tcp_client_channel> channel) = 0;
	virtual void	on_closed(std::shared_ptr<tcp_client_channel> channel) = 0;
	//�ο�TCP_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<tcp_client_channel> channel) = 0;
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len, std::shared_ptr<tcp_client_channel> channel) = 0;
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, std::shared_ptr<tcp_client_channel> channel) = 0;
};