#pragma once

#include "udp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/backend_poller.h"

class udp_server_callback;
//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���
class udp_server_channel : public std::enable_shared_from_this<udp_server_channel>, public thread_safe_objbase, public sockio_channel, public udp_channel_base
{
public:
	udp_server_channel(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, std::shared_ptr<udp_server_callback> cb,
						const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_server_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	void		close();	
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }
	bool		start();

private:
	std::shared_ptr<reactor_loop>	_reactor;
	std::shared_ptr<udp_server_callback>	_cb;

	void		invalid();

	virtual void on_sockio_read();
	virtual void on_sockio_write();
	virtual SOCKET	get_fd() { return _fd; }

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class udp_server_callback : public std::enable_shared_from_this<udp_server_callback>, public thread_safe_objbase
{
public:
	udp_server_callback(){}
	virtual ~udp_server_callback(){}

	//override------------------
	virtual void	on_ready(std::shared_ptr<udp_server_channel> channel) = 0;	//����
	virtual void	on_closed(std::shared_ptr<udp_server_channel> channel) = 0;
	//�ο�TCP_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<udp_server_channel> channel) = 0;
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr, std::shared_ptr<udp_server_channel> channel) = 0;
};