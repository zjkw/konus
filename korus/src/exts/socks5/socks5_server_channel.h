#pragma once

#include "korus/src/tcp/tcp_server_channel.h"
#include "socks5_connectcmd_tunnel_client_channel.h"
#include "socks5_bindcmd_tunnel_server_channel.h"
#include "socks5_associatecmd_server_channel.h"

//connectcmd:
//	source_client													socks5_proxy														target_server
//		|																|																|					
//		|-----------------------tunnel_req----------------------------->|																|
//		|																|----															|					
//		|																|	|����socks5_server_channel									|					
//		|																|<---															|					
//		|																|----����socks5_connectcmd_tunnel_client_channel����connect---->|					
//		|																|----															|					
//		|																|	|connect���												|					
//		|																|<---															|					
//		|<----------------------tunnel_ack------------------------------|																|					
//		|																|																|		
//		|-----------------------data_trans----------------------------->|																|
//		|																|----															|					
//		|																|	|socks5_server_channel����									|					
//		|																|<---															|		
//		|																|----socks5_connectcmd_tunnel_client_channel	data_trans----->|	
//		|																|																|		
//		|																|																|		
//		|																|<---data_trans-------------------------------------------------|	
//		|																|----															|					
//		|																|	|socks5_connectcmd_tunnel_client_channel����				|					
//		|																|<---															|		
//		|<----------------------socks5_server_channel	data_trans------|																|

//connectcmd:(����connectcmd���ϣ������Ϊʵ������ͨ����bindcmd)
//	source_client													socks5_proxy														target_server
//		|																|																|					
//		|-----------------------tunnel_req----------------------------->|																|
//		|																|----															|					
//		|																|	|����socks5_server_channel									|					
//		|																|<---															|					
//		|																|----															|					
//		|																|	|����socks5_bindcmd_tunnel_listen��ִ��listen				|					
//		|																|<---															|	
//		|<----------------------tunnel_ack_1----------------------------|																|	
//		|																|<---����connect	socks5_bindcmd_tunnel_server_channel--------|	
//		|																|----															|					
//		|																|	|����socks5_bindcmd_tunnel_server_channel					|					
//		|																|<---															|					
//		|<----------------------tunnel_ack_2----------------------------|																|					
//		|																|																|		
//		|-----------------------data_trans----------------------------->|																|
//		|																|----															|					
//		|																|	|socks5_server_channel����									|					
//		|																|<---															|		
//		|																|----socks5_bindcmd_tunnel_server_channel	data_trans--------->|	
//		|																|																|		
//		|																|																|		
//		|																|<---data_trans-------------------------------------------------|	
//		|																|----															|					
//		|																|	|socks5_bindcmd_tunnel_server_channel����					|					
//		|																|<---															|		
//		|<----------------------socks5_server_channel	data_trans------|																|

//associatecmd:
//	source_client													socks5_proxy												target_server
//		|																|																|					
//		|-----------------------tunnel_req----------------------------->|																|
//		|																|----															|					
//		|																|	|����socks5_server_channel									|					
//		|																|<---															|					
//		|																|----															|					
//		|																|	|����socks5_associatecmd_server_channel��ִ��bind			|					
//		|																|<---															|	
//		|<----------------------tunnel_ack------------------------------|																|					
//		|																|																|					
//		|-----------------------data_trans(udp)------------------------>|																|
//		|																|----															|					
//		|																|	|socks5_associatecmd_server_channel����						|					
//		|																|<---															|		
//		|																|----socks5_associatecmd_server_channel		data_trans--------->|	
//		|																|																|		
//		|																|																|		
//		|																|<---data_trans-------------------------------------------------|	
//		|																|----															|					
//		|																|	|socks5_associatecmd_server_channel����						|					
//		|																|<---															|		
//		|<-----------socks5_associatecmd_server_channel	data_trans------|																|

class socks5_server_channel : public tcp_server_handler_base
{
public:
	socks5_server_channel(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_server_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();
	virtual long	chain_refcount();
	virtual void	on_accept();
	virtual void	on_closed();

	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len);

private:
	enum TUNNEL_CHANNEL_TYPE
	{
		TCT_NONE = 0,
		TCT_CONNECT = 1,
		TCT_BIND = 2,
		TCT_ASSOCIATE = 3
	};
	TUNNEL_CHANNEL_TYPE		_tunnel_channel_type;
	std::shared_ptr<socks5_connectcmd_tunnel_client_channel>	_connectcmd_tunnel_client_channel;	//����_tunnel_channel_type = TCT_CONNECT��Ч�����������ں�����ת��
	std::shared_ptr<socks5_bindcmd_tunnel_server_channel>		_bindcmd_tunnel_server_channel;		//����_tunnel_channel_type = TCT_BIND��Ч�����������ں�����ת��
	std::shared_ptr<socks5_associatecmd_server_channel>			_associatecmd_server_channel;		//����_tunnel_channel_type = TCT_ASSOCIATE��Ч����������������
};