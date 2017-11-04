#pragma once

#include "string.h"
#include "korus/src/tcp/tcp_client.h"
#include "korus/src/udp/udp_client.h"
#include "socks5_connectcmd_client_channel.h"
#include "socks5_connectcmd_embedbind_client_channel.h"	//ȡ��bindcmd��Ϊ���
#include "socks5_associatecmd_client_channel.h"
/////////////////////////////////////// connect_cmd_mode

// ռ��
template<typename T>
class socks5_connectcmd_client
{
public:
	socks5_connectcmd_client(){}
	virtual ~socks5_connectcmd_client(){}
};

template <>
class socks5_connectcmd_client<uint16_t> : public tcp_client<uint16_t>
{
public:
	socks5_connectcmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// ����˺�Ϊ�գ����������룬��Ϊ�������Ȩ
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: tcp_client(thread_num, proxy_addr, factory, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size),
		_server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw)
	{
		_factory_chain.push_back(std::bind(&socks5_connectcmd_client::socks5_channel_factory, this));
	}
	virtual ~socks5_connectcmd_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()
	{
		std::shared_ptr<socks5_connectcmd_client_channel>	channel = std::make_shared<socks5_connectcmd_client_channel>(_server_addr, _socks_user, _socks_psw);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
};

template <>
class socks5_connectcmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr��ʽip:port
	socks5_connectcmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const tcp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// ����˺�Ϊ�գ����������룬��Ϊ�������Ȩ
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: tcp_client(reactor, proxy_addr, factory, connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size),
		_server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw)
	{
		_factory_chain.push_back(std::bind(&socks5_connectcmd_client::socks5_channel_factory, this));
	}

	virtual ~socks5_connectcmd_client()
	{
	}
private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()
	{
		std::shared_ptr<socks5_connectcmd_client_channel>	channel = std::make_shared<socks5_connectcmd_client_channel>(_server_addr, _socks_user, _socks_psw);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
};


///////////////////////////////////// bind_cmd_mode
// ռ��
template<typename T>
class socks5_bindcmd_client
{
public:
	socks5_bindcmd_client(){}
	virtual ~socks5_bindcmd_client(){}
};

template <>
class socks5_bindcmd_client<uint16_t> : public tcp_client<uint16_t>
{
public:
	// ��ƿ��ǣ���ʵ���Ҳ���Ե���connect + bind������ϲ�����������Ϊģ��̻��˶��̣߳�����һ����ô�����û�����Ҫ������������ԣ��������˹���Ͳ������ݵ��鷳������ʱ����connect��bind���޷��õ���֤
	socks5_bindcmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const tcp_client_channel_factory_t& ctrl_channel_factory, const tcp_client_channel_factory_t& data_channel_factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// ����˺�Ϊ�գ����������룬��Ϊ�������Ȩ
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _ctrl_channel_factory(ctrl_channel_factory), _data_channel_factory(data_channel_factory), _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw),
		tcp_client(thread_num, proxy_addr, std::bind(&socks5_bindcmd_client::socks5_channel_factory, this), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_bindcmd_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()
	{
		std::shared_ptr<socks5_connectcmd_embedbind_client_channel>	channel = std::make_shared<socks5_connectcmd_embedbind_client_channel>(_server_addr, _socks_user, _socks_psw);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}
	
	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	tcp_client_channel_factory_t _ctrl_channel_factory;	
	tcp_client_channel_factory_t _data_channel_factory;
};

template <>
class socks5_bindcmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr��ʽip:port
	socks5_bindcmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const tcp_client_channel_factory_t& ctrl_channel_factory, const tcp_client_channel_factory_t& data_channel_factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// ����˺�Ϊ�գ����������룬��Ϊ�������Ȩ
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _ctrl_channel_factory(ctrl_channel_factory), _data_channel_factory(data_channel_factory), _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw), 
		tcp_client(reactor, proxy_addr, std::bind(&socks5_bindcmd_client::socks5_channel_factory, this), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}

	virtual ~socks5_bindcmd_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()
	{
		std::shared_ptr<socks5_connectcmd_embedbind_client_channel>	channel = std::make_shared<socks5_connectcmd_embedbind_client_channel>(_server_addr, _socks_user, _socks_psw);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	tcp_client_channel_factory_t _ctrl_channel_factory;
	tcp_client_channel_factory_t _data_channel_factory;
};

///////////////////////////////////// assocate_cmd_mode

template<typename T>
class socks5_associatecmd_client
{
public:
	socks5_associatecmd_client(){}
	virtual ~socks5_associatecmd_client(){}
};

template <>
class socks5_associatecmd_client<uint16_t> : public tcp_client<uint16_t>
{
public:
	socks5_associatecmd_client(uint16_t thread_num, const std::string& proxy_addr, const std::string& server_addr, const udp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// ����˺�Ϊ�գ����������룬��Ϊ�������Ȩ
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw), _udp_client_channel_factory(factory),
		tcp_client(thread_num, proxy_addr, std::bind(&socks5_associatecmd_client::socks5_channel_factory, this), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_associatecmd_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()	//ԭ��channel
	{
		std::shared_ptr<socks5_associatecmd_client_channel>	channel = std::make_shared<socks5_associatecmd_client_channel>(_server_addr, _socks_user, _socks_psw, _udp_client_channel_factory);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}
	
	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	udp_client_channel_factory_t _udp_client_channel_factory;
};

template <>
class socks5_associatecmd_client<reactor_loop> : public tcp_client<reactor_loop>
{
public:
	// addr��ʽip:port
	socks5_associatecmd_client(std::shared_ptr<reactor_loop> reactor, const std::string& proxy_addr, const std::string& server_addr, const udp_client_channel_factory_t& factory,
		const std::string& socks_user = "", const std::string& socks_psw = "",	// ����˺�Ϊ�գ����������룬��Ϊ�������Ȩ
		std::chrono::seconds connect_timeout = std::chrono::seconds(0), std::chrono::seconds connect_retry_wait = std::chrono::seconds(-1),
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _server_addr(_server_addr), _socks_user(socks_user), _socks_psw(socks_psw), _udp_client_channel_factory(factory),
		tcp_client(reactor, proxy_addr, std::bind(&socks5_associatecmd_client::socks5_channel_factory, this), connect_timeout, connect_retry_wait, self_read_size, self_write_size, sock_read_size, sock_write_size)		
	{
	}

	virtual ~socks5_associatecmd_client()
	{
	}

private:
	std::shared_ptr<tcp_client_handler_base>	socks5_channel_factory()	//ԭ��channel
	{
		std::shared_ptr<socks5_associatecmd_client_channel>	channel = std::make_shared<socks5_associatecmd_client_channel>(_server_addr, _socks_user, _socks_psw, _udp_client_channel_factory);
		std::dynamic_pointer_cast<tcp_client_handler_base>(channel);
	}

	std::string _server_addr;
	std::string _socks_user;
	std::string _socks_psw;
	udp_client_channel_factory_t _udp_client_channel_factory;
};