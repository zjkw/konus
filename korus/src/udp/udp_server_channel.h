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

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class udp_server_handler_base : public std::enable_shared_from_this<udp_server_handler_base>
{
public:
	udp_server_handler_base();
	virtual ~udp_server_handler_base();

	//override------------------
	virtual void	on_init();
	virtual void	on_final();
	virtual void	on_ready();
	virtual void	on_closed();
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	virtual int32_t	send(const void* buf, const size_t len, const sockaddr_in& peer_addr);
	virtual void	close();
	std::shared_ptr<reactor_loop>	reactor();
	virtual bool	can_delete(bool force, long call_ref_count);//forceΪ���ʾǿ�Ʋ�ѯ������ĸ���˳�

private:
	template<typename T> friend bool build_chain(std::shared_ptr<reactor_loop> reactor, T tail, const std::list<std::function<T()> >& chain);
	template<typename T> friend class udp_server;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_server_handler_base> tunnel_prev, std::shared_ptr<udp_server_handler_base> tunnel_next);
	void	inner_final();

	std::shared_ptr<reactor_loop>				_reactor;
	std::shared_ptr<udp_server_handler_base>	_tunnel_prev;
	std::shared_ptr<udp_server_handler_base>	_tunnel_next;
};

class udp_server_channel : public udp_channel_base, public udp_server_handler_base, public multiform_state
{
public:
	udp_server_channel(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_server_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	virtual int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	virtual void		close();
	bool				start();

private:
	template<typename T> friend class udp_server;
	virtual void	set_release();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr);
	virtual bool	can_delete(bool force, long call_ref_count);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
