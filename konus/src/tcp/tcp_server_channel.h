#pragma once

#include "tcp_channel_base.h"
#include "..\reactor\reactor_loop.h"
#include "..\reactor\backend_poller.h"

class tcp_server_callback;
//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ����������close����ܻ���send/recv

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���
class tcp_server_channel : public std::enable_shared_from_this<tcp_server_channel>, public thread_safe_objbase, public sockio_channel, public tcp_channel_base
{
public:
	tcp_server_channel(SOCKET fd, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_callback> cb,
				const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_server_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������
	void		close();	
	void		shutdown(int32_t howto);// �����ο�ȫ�ֺ��� ::shutdown
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	std::shared_ptr<reactor_loop>	_reactor;
	std::shared_ptr<tcp_server_callback>	_cb;

	friend class tcp_listen;
	void		invalid();

	virtual void on_sockio_read();
	virtual void on_sockio_write();
	virtual SOCKET	get_fd() { return _fd; }

	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);
};

// ���ܴ��ڶ��̻߳�����
class tcp_server_callback : public std::enable_shared_from_this<tcp_server_callback>, public thread_safe_objbase
{
public:
	tcp_server_callback(){}
	virtual ~tcp_server_callback(){}

	//override------------------
	virtual void	on_accept(std::shared_ptr<tcp_server_channel> channel) = 0;	//�����Ѿ�����
	virtual void	on_closed(std::shared_ptr<tcp_server_channel> channel) = 0;
	//�ο�TCP_ERROR_CODE����
	virtual void	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<tcp_server_channel> channel) = 0;
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len, std::shared_ptr<tcp_server_channel> channel) = 0;
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, std::shared_ptr<tcp_server_channel> channel) = 0;
};