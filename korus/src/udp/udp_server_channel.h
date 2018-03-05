#pragma once

#include "udp_channel_base.h"
#include "korus/src/util/chain_object.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

class udp_server_handler_base;

using udp_server_channel_factory_t = std::function<complex_ptr<udp_server_handler_base>(std::shared_ptr<reactor_loop> reactor)>;

//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//�ⲿ���ȷ��udp_server�ܰ���channel/handler�����ڣ������ܱ�֤��Դ���գ����������õ�����(channel��handler)û�п���ƵĽ�ɫ����������ʱ���ĺ�������check_detach_relation

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class udp_server_handler_base : public chain_object_linkbase<udp_server_handler_base>, public obj_refbase<udp_server_handler_base>
{
public:
	udp_server_handler_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~udp_server_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_ready();
	virtual void	on_closed();
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//����һ���������������
	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);

	virtual bool	start();
	virtual void	send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);
	virtual int32_t	connect(const sockaddr_in& server_addr);
	virtual void	send(const std::shared_ptr<buffer_thunk>& data);								// �ⲿ���ݷ���
	virtual void	close();
	virtual bool	local_addr(std::string& addr);

	std::shared_ptr<reactor_loop>	reactor();

private:
	std::shared_ptr<reactor_loop>	_reactor;
};

class udp_server_handler_origin : public udp_channel_base, public udp_server_handler_base, public multiform_state
{
public:
	udp_server_handler_origin(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0);
	virtual ~udp_server_handler_origin();

	// �����ĸ��������������ڶ��̻߳�����	
	virtual bool	start();
	virtual void	send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	virtual int32_t	connect(const sockaddr_in& server_addr);
	virtual void	send(const std::shared_ptr<buffer_thunk>& data);								// �ⲿ���ݷ���
	virtual void	close();
	virtual bool	local_addr(std::string& addr);

private:
	template<typename T> friend class udp_server;
	virtual void	set_release();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);

	virtual	void	on_recv_buff(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};

//terminalʵ��
class udp_server_handler_terminal : public udp_server_handler_base, public std::enable_shared_from_this<udp_server_handler_terminal>
{
public:
	udp_server_handler_terminal(std::shared_ptr<reactor_loop> reactor);
	virtual ~udp_server_handler_terminal();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_ready();
	virtual void	on_closed();
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	virtual bool	start();
	virtual int32_t	send(const void* buf, const size_t len, const sockaddr_in& peer_addr);
	virtual int32_t	connect(const sockaddr_in& server_addr);
	virtual int32_t	send(const void* buf, const size_t len);								// �ⲿ���ݷ���
	virtual void	close();
	virtual bool	local_addr(std::string& addr);

	//override------------------
	virtual void	chain_inref();
	virtual void	chain_deref();

protected:
	virtual void	on_release();

	virtual void	on_recv_pkg(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);
	virtual void	send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);
	virtual	void	send(const std::shared_ptr<buffer_thunk>& data);
};