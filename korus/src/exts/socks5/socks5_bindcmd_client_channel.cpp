#include <assert.h>
#include "socks5_bindcmd_client_channel.h"

socks5_bindcmd_client_channel::socks5_bindcmd_client_channel()
{

}

socks5_bindcmd_client_channel::~socks5_bindcmd_client_channel()
{
}

//override------------------
void	socks5_bindcmd_client_channel::on_init()
{
}

void	socks5_bindcmd_client_channel::on_final()
{
}

void	socks5_bindcmd_client_channel::on_connected()
{
}

void	socks5_bindcmd_client_channel::on_closed()
{

}

//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
int32_t socks5_bindcmd_client_channel::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//����һ���������������
void	socks5_bindcmd_client_channel::on_recv_pkg(const void* buf, const size_t len)
{

}