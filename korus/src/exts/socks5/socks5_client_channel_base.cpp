#include <assert.h>
#include <string.h>
#include "socks5_client_channel_base.h"

socks5_client_channel_base::socks5_client_channel_base()
{

}

socks5_client_channel_base::~socks5_client_channel_base()
{
}

//override------------------
void	socks5_client_channel_base::on_init()
{
}

void	socks5_client_channel_base::on_final()
{
}

void	socks5_client_channel_base::on_connected()
{
}

void	socks5_client_channel_base::on_closed()
{

}

//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
int32_t socks5_client_channel_base::on_recv_split(const void* buf, const size_t len)
{
	return 0;
}

//����һ���������������
void	socks5_client_channel_base::on_recv_pkg(const void* buf, const size_t len)
{

}