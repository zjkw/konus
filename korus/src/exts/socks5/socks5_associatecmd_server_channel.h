#pragma once

#include "korus/src/exts/domain/domain_async_resolve_helper.h"
#include "socks5_client_channel_base.h"

// �߼�ʱ��
// 1��socks5_associatecmd_client_channel ����������ִ��associate_cmd������udp����Ŀ�������

class socks5_associatecmd_server_channel
{
public:
	socks5_associatecmd_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_associatecmd_server_channel();
};
