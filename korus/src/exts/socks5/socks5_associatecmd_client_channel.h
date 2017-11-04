#pragma once

#include "socks5_client_channel_base.h"

class socks5_associatecmd_client_channel : public socks5_client_channel_base
{
public:
	socks5_associatecmd_client_channel(const std::string& server_addr, const std::string& socks_user, const std::string& socks_psw, const udp_client_channel_factory_t& udp_factory);
	virtual ~socks5_associatecmd_client_channel();

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
