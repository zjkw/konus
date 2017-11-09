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
void	socks5_bindcmd_integration_handler_base::on_init()
{
}

void	socks5_bindcmd_integration_handler_base::on_final()
{
}

//ctrl channel--------------
// �����ĸ��������������ڶ��̻߳�����	
int32_t	socks5_bindcmd_integration_handler_base::ctrl_send(const void* buf, const size_t len)			// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
{
	return 0;
}

void	socks5_bindcmd_integration_handler_base::ctrl_close()
{

}

void	socks5_bindcmd_integration_handler_base::ctrl_shutdown(int32_t howto)							// �����ο�ȫ�ֺ��� ::shutdown
{

}

void	socks5_bindcmd_integration_handler_base::ctrl_connect()
{

}


TCP_CLTCONN_STATE	socks5_bindcmd_integration_handler_base::ctrl_state()
{
	return CNS_CLOSED;
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
	return 0;
}

void	socks5_bindcmd_integration_handler_base::data_close()
{

}

void	socks5_bindcmd_integration_handler_base::data_shutdown(int32_t howto)							// �����ο�ȫ�ֺ��� ::shutdown
{

}

void	socks5_bindcmd_integration_handler_base::data_connect()
{

}

TCP_CLTCONN_STATE	socks5_bindcmd_integration_handler_base::data_state()
{
	return CNS_CLOSED;
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
