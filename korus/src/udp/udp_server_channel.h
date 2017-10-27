#pragma once

#include <list>
#include "udp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

class udp_server_handler_base;

using udp_server_channel_factory_t = std::function<std::shared_ptr<udp_server_handler_base>()>;
using udp_server_channel_factory_chain_t = std::list<udp_server_channel_factory_t>;

//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//�ⲿ���ȷ��udp_server�ܰ���channel/handler�����ڣ������ܱ�֤��Դ���գ����������õ�����(channel��handler)û�п���ƵĽ�ɫ����������ʱ���ĺ�������check_detach_relation

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���
class udp_server_channel : public std::enable_shared_from_this<udp_server_channel>, public thread_safe_objbase, public udp_channel_base
{
public:
	udp_server_channel(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, std::shared_ptr<udp_server_handler_base> cb,
						const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_server_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	void		close();	
	std::shared_ptr<reactor_loop>	get_reactor() { return _reactor; }
	bool		start();

private:
	std::shared_ptr<reactor_loop>	_reactor;
	std::shared_ptr<udp_server_handler_base>	_cb;

	template<typename T> friend class udp_server;
	// channelҪ����/����Ҫ��handler�Ƿ�û���������������
	bool		check_detach_relation(long call_ref_count);	//true��ʾ�Ѿ���������ϵ
	void		invalid();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class udp_server_handler_base : public std::enable_shared_from_this<udp_server_handler_base>, public thread_safe_objbase
{
public:
	udp_server_handler_base() : _reactor(nullptr), _channel(nullptr){}
	virtual ~udp_server_handler_base(){ assert(!_reactor); assert(!_channel); }

	//override------------------
	virtual void	on_init() = 0;
	virtual void	on_final() = 0;
	virtual void	on_ready() = 0;	//����
	virtual void	on_closed() = 0;
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code) = 0;
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr) = 0;
	
protected:
	int32_t	send(const void* buf, const size_t len, const sockaddr_in& peer_addr)	{ if (!_channel) return CEC_INVALID_SOCKET;  return _channel->send(buf, len, peer_addr); }
	void	close()									{ if (_channel)_channel->close(); }
	std::shared_ptr<reactor_loop>	get_reactor()	{ return _reactor; }

private:
	template<typename T> friend class udp_server;
	friend class udp_server_channel;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_server_channel> channel)
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
	std::shared_ptr<udp_server_channel> _channel;
};