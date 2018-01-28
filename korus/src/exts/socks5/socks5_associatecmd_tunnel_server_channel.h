#pragma once

#include "korus/src/udp/udp_server_channel.h"
#include "korus/src/exts/domain/domain_async_resolve_helper.h"
#include "socks5_client_channel_base.h"

// �߼�ʱ��
// 1��socks5_associatecmd_client_channel ����������ִ��associate_cmd������udp����Ŀ�������
class socks5_server_channel;
class socks5_associatecmd_tunnel_server_channel : public udp_server_handler_base
{
public:
	socks5_associatecmd_tunnel_server_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_channel> channel, uint32_t ip_digit, uint16_t client_port);
	virtual ~socks5_associatecmd_tunnel_server_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_ready();
	virtual void	on_closed();
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t size, const sockaddr_in& peer_addr);
	
private:
	std::shared_ptr<socks5_server_channel>	_origin_channel;
	uint32_t	_client_ip;
	uint16_t	_client_port;
};
