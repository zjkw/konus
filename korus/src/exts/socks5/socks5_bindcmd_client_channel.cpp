#include <assert.h>
#include "socks5_bindcmd_integration_handler_base.h"
#include "socks5_bindcmd_client_channel.h"

socks5_bindcmd_client_channel::socks5_bindcmd_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw)
	: socks5_client_channel_base(reactor)
{

}

socks5_bindcmd_client_channel::~socks5_bindcmd_client_channel()
{
}

//override------------------
void	socks5_bindcmd_client_channel::on_chain_init()
{
}

void	socks5_bindcmd_client_channel::on_chain_final()
{
}

void	socks5_bindcmd_client_channel::on_connected()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_connected();
}

void	socks5_bindcmd_client_channel::on_closed()
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_closed();
}

CLOSE_MODE_STRATEGY	socks5_bindcmd_client_channel::on_error(CHANNEL_ERROR_CODE code)		//�ο�CHANNEL_ERROR_CODE����	
{
	if (!_integration)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _integration->on_ctrl_error(code);
}

//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
int32_t socks5_bindcmd_client_channel::on_recv_split(const void* buf, const size_t len)
{
	if (!_integration)
	{
		assert(false);
		return 0;
	}

	return _integration->on_ctrl_recv_split(buf, len);
}

//����һ���������������
void	socks5_bindcmd_client_channel::on_recv_pkg(const void* buf, const size_t len)
{
	if (!_integration)
	{
		assert(false);
		return;
	}

	_integration->on_ctrl_recv_pkg(buf, len);
}

long	socks5_bindcmd_client_channel::chain_refcount()
{
	long ref = 0;
	if (_integration)
	{
		ref++;
	}

	return ref + socks5_client_channel_base::chain_refcount();
}
