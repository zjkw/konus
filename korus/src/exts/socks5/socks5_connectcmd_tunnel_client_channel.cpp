#include <assert.h>
#include "socks5_server_channel.h"
#include "socks5_connectcmd_tunnel_client_channel.h"

socks5_connectcmd_tunnel_client_channel::socks5_connectcmd_tunnel_client_channel(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<socks5_server_channel> server_channel)
: _server_channel(server_channel), tcp_client_handler_base(reactor)
{
}

socks5_connectcmd_tunnel_client_channel::~socks5_connectcmd_tunnel_client_channel()
{
}

//override------------------
void	socks5_connectcmd_tunnel_client_channel::on_chain_init()
{

}

void	socks5_connectcmd_tunnel_client_channel::on_chain_final()
{
	_server_channel = nullptr;
}

void	socks5_connectcmd_tunnel_client_channel::on_chain_zomby()
{

}

long	socks5_connectcmd_tunnel_client_channel::chain_refcount()
{
	long ref = 0;
	if (_server_channel)
	{
		ref++;
	}

	return ref + tcp_client_handler_base::chain_refcount();
}

void	socks5_connectcmd_tunnel_client_channel::on_connected()
{
	assert(_server_channel);

	std::string addr;
	if (!peer_addr(addr))
	{
		return;
	}

	_server_channel->on_connectcmd_tunnel_connect(addr);
}

void	socks5_connectcmd_tunnel_client_channel::on_closed()
{
	assert(_server_channel);

	_server_channel->on_connectcmd_tunnel_close();
}

//�ο�CHANNEL_ERROR_CODE����
CLOSE_MODE_STRATEGY	socks5_connectcmd_tunnel_client_channel::on_error(CHANNEL_ERROR_CODE code)
{
	assert(_server_channel);

	return _server_channel->on_connectcmd_tunnel_error(code);
}

//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
int32_t socks5_connectcmd_tunnel_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return len;
}

//����һ���������������
void	socks5_connectcmd_tunnel_client_channel::on_recv_pkg(const void* buf, const size_t len)
{
	assert(_server_channel);

	_server_channel->send(buf, len);
}

