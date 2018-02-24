#pragma once

#include <vector>
#include "korus/src/tcp/tcp_server.h"
#include "socks5_bindcmd_tunnel_server_channel.h"

class socks5_bindcmd_server_channel : public tcp_server_handler_terminal
{
public:
	socks5_bindcmd_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_bindcmd_server_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_accept();
	virtual void	on_closed();

	virtual void	init(const std::string& addr, const std::string& listen_ip);

	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t size);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t size);
	
	void	on_tunnel_accept(std::shared_ptr<socks5_bindcmd_tunnel_server_channel> channel);
	
private:	
	tcp_server<reactor_loop>*									_bindcmd_server;
	std::weak_ptr<socks5_bindcmd_tunnel_server_channel>			_bindcmd_tunnel_server_channel;	
	bool														_is_valid;//�ܷ���������
	
	complex_ptr<tcp_server_handler_base> binccmd_channel_factory(std::shared_ptr<reactor_loop> reactor);
};