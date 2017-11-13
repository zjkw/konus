#pragma once

#include <functional>
#include "korus/src/util/chain_sharedobj_base.h"
#include "socks5_bindcmd_client_channel.h"
#include "socks5_connectcmd_embedbind_client_channel.h"

class socks5_bindcmd_integration_handler_base : public chain_sharedobj_base<socks5_bindcmd_integration_handler_base>
{
public:
	socks5_bindcmd_integration_handler_base();
	virtual ~socks5_bindcmd_integration_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();
	virtual long	chain_refcount();
	virtual void	chain_init(std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl_channel, std::shared_ptr<socks5_bindcmd_client_channel> data_channel);
	virtual void	chain_final();	
	virtual void	chain_zomby();

	//ctrl channel--------------
	// ��������������������ڶ��̻߳�����	
	virtual int32_t	ctrl_send(const void* buf, const size_t len);			// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	virtual void	ctrl_close();
	virtual void	ctrl_shutdown(int32_t howto);							// �����ο�ȫ�ֺ��� ::shutdown
	virtual void	ctrl_connect();
	virtual TCP_CLTCONN_STATE	ctrl_state();
	virtual void	on_ctrl_connected();
	virtual void	on_ctrl_closed();	
	CLOSE_MODE_STRATEGY	on_ctrl_error(CHANNEL_ERROR_CODE code);				//�ο�CHANNEL_ERROR_CODE����	
	virtual int32_t on_ctrl_recv_split(const void* buf, const size_t len);	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)	
	virtual void	on_ctrl_recv_pkg(const void* buf, const size_t len);	//����һ���������������

	//data channel--------------
	// ��������������������ڶ��̻߳�����	
	virtual int32_t	data_send(const void* buf, const size_t len);			// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	virtual void	data_close();
	virtual void	data_shutdown(int32_t howto);							// �����ο�ȫ�ֺ��� ::shutdown
	virtual void	data_connect();
	virtual TCP_CLTCONN_STATE	data_state();
	virtual void	on_data_connected();
	virtual void	on_data_closed();
	virtual CLOSE_MODE_STRATEGY	on_data_error(CHANNEL_ERROR_CODE code);		//�ο�CHANNEL_ERROR_CODE����
	virtual int32_t on_data_recv_split(const void* buf, const size_t len);	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual void	on_data_recv_pkg(const void* buf, const size_t len);	//����һ���������������

private:		
	std::shared_ptr<reactor_loop>								_reactor;
	std::shared_ptr<socks5_connectcmd_embedbind_client_channel>	_ctrl_channel;
	std::shared_ptr<socks5_bindcmd_client_channel>				_data_channel;
};

using socks5_bindcmd_integration_handler_factory_t = std::function<std::shared_ptr<socks5_bindcmd_integration_handler_base>()>;

bool build_relation(std::shared_ptr<socks5_connectcmd_embedbind_client_channel> ctrl, std::shared_ptr<socks5_bindcmd_client_channel> data, std::shared_ptr<socks5_bindcmd_integration_handler_base> integration);

