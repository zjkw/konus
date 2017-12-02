#include "socks5_server_channel.h"

socks5_server_channel::socks5_server_channel(std::shared_ptr<reactor_loop> reactor)
	: _tunnel_channel_type(TCT_NONE),
	tcp_server_handler_base(reactor)
{
}

socks5_server_channel::~socks5_server_channel()
{
}

//override------------------
void	socks5_server_channel::on_chain_init()
{
}

void	socks5_server_channel::on_chain_final()
{
}

void	socks5_server_channel::on_chain_zomby()
{
	// ��Ϊû�б������������ã���������ڿ��Ҫ�����˳���������������ȥ�������
}

long	socks5_server_channel::chain_refcount()
{
	return tcp_server_handler_base::chain_refcount();
}

void	socks5_server_channel::on_accept()	//�����Ѿ�����
{

}

void	socks5_server_channel::on_closed()
{

}

//�ο�CHANNEL_ERROR_CODE����
CLOSE_MODE_STRATEGY	socks5_server_channel::on_error(CHANNEL_ERROR_CODE code)
{
	return CMS_INNER_AUTO_CLOSE;
}

//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
int32_t socks5_server_channel::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//����һ���������������
void	socks5_server_channel::on_recv_pkg(const void* buf, const size_t len)
{

}