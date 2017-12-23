#pragma once

#include <functional>
#include "korus/src/util/chain_sharedobj_base.h"
#include "socks5_bindcmd_originalbind_client_channel.h"
#include "socks5_connectcmd_embedbind_client_channel.h"

// �߼�ʱ��
// 1��socks5_connectcmd_embedbind_client_channel ����������ִ��connect_cmd������tcp����Ŀ�������
// 2��socks5_bindcmd_originalbind_client_channel ��������������bind���������ش��������Ŀ���������connect����ip�Ͷ˿� X
// 3��socks5_connectcmd_embedbind_client_channel ��������Ҫ��Ŀ����������� X
// 4��������������� Ŀ����������ӵ�ip�Ͷ˿�

class socks5_bindcmd_client_handler_base : public chain_sharedobj_base<socks5_bindcmd_client_handler_base>
{
public:
	socks5_bindcmd_client_handler_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~socks5_bindcmd_client_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();

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
	virtual int32_t on_ctrl_recv_split(const void* buf, const size_t size);
	virtual void	on_ctrl_recv_pkg(const void* buf, const size_t size);	//����һ���������������

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
	virtual int32_t on_data_recv_split(const void* buf, const size_t size);
	virtual void	on_data_recv_pkg(const void* buf, const size_t size);	//����һ���������������
	virtual void	on_data_bindcmd_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr);		//������������ڼ�����Ŀ����������������ӡ���ַ
	
private:
	std::shared_ptr<reactor_loop>		_reactor;
};

class socks5_bindcmd_client_channel : public socks5_bindcmd_client_handler_base
{
public:
	socks5_bindcmd_client_channel(std::shared_ptr<reactor_loop>	reactor, const std::string& proxy_addr, const std::string& server_addr,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// ����˺�Ϊ�գ����������룬��Ϊ�������Ȩ
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0);
	virtual ~socks5_bindcmd_client_channel();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();
	virtual void	on_chain_zomby();
	virtual long	chain_refcount();
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
	virtual int32_t on_ctrl_recv_split(const void* buf, const size_t size);
	virtual void	on_ctrl_recv_pkg(const void* buf, const size_t size);	//����һ���������������

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
	virtual int32_t on_data_recv_split(const void* buf, const size_t size);
	virtual void	on_data_recv_pkg(const void* buf, const size_t size);	//����һ���������������

	virtual void	on_data_bindcmd_result(const CHANNEL_ERROR_CODE code, const std::string& proxy_listen_target_addr);		//������������ڼ�����Ŀ����������������ӡ���ַ

private:		
	std::shared_ptr<socks5_connectcmd_embedbind_client_channel>	_ctrl_channel;
	std::shared_ptr<socks5_bindcmd_originalbind_client_channel>	_data_channel;
};

using socks5_bindcmd_client_channel_factory_t = std::function<std::shared_ptr<socks5_bindcmd_client_handler_base>(std::shared_ptr<reactor_loop>)>;


