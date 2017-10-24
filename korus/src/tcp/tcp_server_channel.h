#pragma once

#include <assert.h>
#include "tcp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/backend_poller.h"

class tcp_server_handler_base;
//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���
class tcp_server_channel : public std::enable_shared_from_this<tcp_server_channel>, public thread_safe_objbase, public sockio_channel, public tcp_channel_base
{
public:
	tcp_server_channel(SOCKET fd, std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_handler_base> cb,
				const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_server_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	void		close();	
	void		shutdown(int32_t howto);// �����ο�ȫ�ֺ��� ::shutdown
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }

private:
	std::shared_ptr<reactor_loop>	_reactor;
	std::shared_ptr<tcp_server_handler_base>	_cb;

	friend class tcp_server_channel_creator;
	void		invalid();
	void		detach();

	virtual void on_sockio_read();
	virtual void on_sockio_write();
	virtual SOCKET	get_fd() { return _fd; }

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class tcp_server_handler_base : public std::enable_shared_from_this<tcp_server_handler_base>, public thread_safe_objbase
{
public:
	tcp_server_handler_base() : _reactor(nullptr), _channel(nullptr){}
	virtual ~tcp_server_handler_base(){ assert(!_reactor); assert(!_channel); }

	//override------------------
	virtual void	on_accept() = 0;	//�����Ѿ�����
	virtual void	on_closed() = 0;
	//�ο�TCP_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code) = 0;
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len) = 0;
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len) = 0;

protected:
	int32_t	send(const void* buf, const size_t len)	{ if (!_channel) return CEC_INVALID_SOCKET; return _channel->send(buf, len); }
	void	close()									{ if (_channel)_channel->close(); }
	void	shutdown(int32_t howto)					{ if (_channel)_channel->shutdown(howto); }
	std::shared_ptr<reactor_loop>	get_reactor()	{	return _reactor; }

private:
	friend class tcp_server_channel_creator;
	friend class tcp_server_channel;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<tcp_server_channel> channel)
	{
		_reactor = reactor;
		_channel = channel;
	}
	void	inner_uninit()
	{
		_reactor = nullptr;
		_channel = nullptr;
	}

	std::shared_ptr<reactor_loop>		_reactor;
	std::shared_ptr<tcp_server_channel> _channel;
};