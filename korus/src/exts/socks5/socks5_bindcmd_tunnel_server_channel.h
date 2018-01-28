#pragma once

#include "korus/src/tcp/tcp_server_channel.h"

class socks5_server_channel;
class socks5_bindcmd_tunnel_server_channel : public tcp_server_handler_terminal
{
public:
	socks5_bindcmd_tunnel_server_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_channel> channel);
	virtual ~socks5_bindcmd_tunnel_server_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_accept();
	virtual void	on_closed();

	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t size);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t size);

private:
	std::shared_ptr<socks5_server_channel>	_origin_channel;
};