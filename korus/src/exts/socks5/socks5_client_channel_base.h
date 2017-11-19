#pragma once

#include "socks5_proto.h"
#include "korus/src/tcp/tcp_client_channel.h"
#include "korus/src/udp/udp_client_channel.h"
#include "korus/src/reactor/timer_helper.h"

class socks5_client_channel_base : public tcp_client_handler_base, public multiform_state
{
public:
	socks5_client_channel_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_client_channel_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_connected();
	virtual void	on_closed();
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len);

protected:
	enum SOCKS_CLIENT_STATE
	{
		SCS_NONE = 0,
		SCS_METHOD = 1,		//�ȴ���������ӦMETHOD==
		SCS_AUTH = 2,
		SCS_TUNNEL = 3,
		SCS_NORMAL = 4,
	};
	SOCKS_CLIENT_STATE	_shakehand_state;
};
