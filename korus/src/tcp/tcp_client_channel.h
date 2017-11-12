#pragma once

#include <unistd.h>
#include <memory>
#include <atomic>
#include <chrono>
#include "korus/src/util/object_state.h"
#include "korus/src/util/chain_sharedobj_base.h"
#include "korus/src/reactor/timer_helper.h"
#include "korus/src/reactor/sockio_helper.h"
#include "korus/src/reactor/reactor_loop.h"
#include "tcp_channel_base.h"

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

class tcp_client_handler_base;

using tcp_client_channel_factory_t = std::function<std::shared_ptr<tcp_client_handler_base>(std::shared_ptr<reactor_loop>)>;

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
class tcp_client_handler_base : public chain_sharedobj_base<tcp_client_handler_base>
{
public:
	tcp_client_handler_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~tcp_client_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();

	virtual void	on_connected();
	virtual void	on_closed();
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len);

	virtual int32_t	send(const void* buf, const size_t len);
	virtual void	close();
	virtual void	shutdown(int32_t howto);
	virtual void	connect();
	virtual TCP_CLTCONN_STATE	state();
	virtual std::shared_ptr<reactor_loop>	reactor();
		
private:
	std::shared_ptr<reactor_loop>		_reactor;
};

//�ⲿ���ȷ��tcp_client�ܰ���channel/handler�����ڣ������ܱ�֤��Դ���գ����������õ�����(channel��handler)û�п���ƵĽ�ɫ����������ʱ���ĺ�������check_detach_relation

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���

class tcp_client_channel : public tcp_channel_base, public tcp_client_handler_base, public multiform_state
{
public:
	tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
		const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_client_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	virtual int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	virtual void		close();
	virtual void		shutdown(int32_t howto);// �����ο�ȫ�ֺ��� ::shutdown
	virtual void		connect();
	virtual TCP_CLTCONN_STATE	state()	{ return _conn_state; }

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
	virtual void		set_release();

	sockio_helper	_sockio_helper_connect;
	sockio_helper	_sockio_helper;
	virtual void on_sockio_write_connect(sockio_helper* sockio_id);
	virtual void on_sockio_read(sockio_helper* sockio_id);
	virtual void on_sockio_write(sockio_helper* sockio_id);
	
	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
