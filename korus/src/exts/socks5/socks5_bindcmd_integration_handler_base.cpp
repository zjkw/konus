#include <assert.h>
#include "korus/src/tcp/tcp_client_channel.h"
#include "socks5_bindcmd_integration_handler_base.h"

socks5_bindcmd_integration_handler_base::socks5_bindcmd_integration_handler_base()
{

}

socks5_bindcmd_integration_handler_base::~socks5_bindcmd_integration_handler_base()
{
}

//override------------------
void	socks5_bindcmd_integration_handler_base::on_chain_init()
{
}

void	socks5_bindcmd_integration_handler_base::on_chain_final()
{
}

//ctrl channel--------------
// �����ĸ��������������ڶ��̻߳�����	
int32_t	socks5_bindcmd_integration_handler_base::ctrl_send(const void* buf, const size_t len)			// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
{
	if (!_ctrl_channel)
	{
		assert(false);
		return 0;
	}
	return _ctrl_channel->send(buf, len);
}

void	socks5_bindcmd_integration_handler_base::ctrl_close()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->close();
}

void	socks5_bindcmd_integration_handler_base::ctrl_shutdown(int32_t howto)							// �����ο�ȫ�ֺ��� ::shutdown
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->shutdown(howto);
}

void	socks5_bindcmd_integration_handler_base::ctrl_connect()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return;
	}
	_ctrl_channel->connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_integration_handler_base::ctrl_state()
{
	if (!_ctrl_channel)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _ctrl_channel->state();
}

void	socks5_bindcmd_integration_handler_base::on_ctrl_connected()
{

}

void	socks5_bindcmd_integration_handler_base::on_ctrl_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_integration_handler_base::on_ctrl_error(CHANNEL_ERROR_CODE code)		//�ο�CHANNEL_ERROR_CODE����	
{
	return CMS_INNER_AUTO_CLOSE;

}

int32_t socks5_bindcmd_integration_handler_base::on_ctrl_recv_split(const void* buf, const size_t len)	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)	
{
	return 0;
}

void	socks5_bindcmd_integration_handler_base::on_ctrl_recv_pkg(const void* buf, const size_t len)	//����һ���������������
{

}

//data channel--------------
// �����ĸ��������������ڶ��̻߳�����	
int32_t	socks5_bindcmd_integration_handler_base::data_send(const void* buf, const size_t len)			// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
{
	if (!_data_channel)
	{
		assert(false);
		return 0;
	}
	return _data_channel->send(buf, len);
}

void	socks5_bindcmd_integration_handler_base::data_close()
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->close();
}

void	socks5_bindcmd_integration_handler_base::data_shutdown(int32_t howto)							// �����ο�ȫ�ֺ��� ::shutdown
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->shutdown(howto);
}

void	socks5_bindcmd_integration_handler_base::data_connect()
{
	if (!_data_channel)
	{
		assert(false);
		return;
	}
	_data_channel->connect();
}

TCP_CLTCONN_STATE	socks5_bindcmd_integration_handler_base::data_state()
{
	if (!_data_channel)
	{
		assert(false);
		return CNS_CLOSED;
	}
	return _data_channel->state();
}

void	socks5_bindcmd_integration_handler_base::on_data_connected()
{

}

void	socks5_bindcmd_integration_handler_base::on_data_closed()
{

}

CLOSE_MODE_STRATEGY	socks5_bindcmd_integration_handler_base::on_data_error(CHANNEL_ERROR_CODE code)		//�ο�CHANNEL_ERROR_CODE����
{
	return CMS_INNER_AUTO_CLOSE;

}

int32_t socks5_bindcmd_integration_handler_base::on_data_recv_split(const void* buf, const size_t len)	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
{
	return 0;
}

void	socks5_bindcmd_integration_handler_base::on_data_recv_pkg(const void* buf, const size_t len)	//����һ���������������
{

}

void	socks5_bindcmd_integration_handler_base::chain_init(std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl_channel, std::shared_ptr<socks5_bindcmd_client_channel>	_data_channel)
{

}

void	socks5_bindcmd_integration_handler_base::chain_final()
{

}