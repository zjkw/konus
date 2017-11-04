#pragma once

#include "socks5_client_channel_base.h"

class socks5_bindcmd_client_channel : public socks5_client_channel_base
{
public:
	socks5_bindcmd_client_channel();
	virtual ~socks5_bindcmd_client_channel();

	//override------------------
	virtual void	on_init();
	virtual void	on_final();
	virtual void	on_connected();
	virtual void	on_closed();
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len);
};
