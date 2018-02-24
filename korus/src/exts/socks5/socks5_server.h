#pragma once

#include "korus/src/tcp/tcp_server.h"
#include "socks5_bindcmd_tunnel_server_channel.h"
#include "socks5_server_init_channel.h"

// ռ��
template<typename T>
class socks5_server
{
public:
	socks5_server(){}
	virtual ~socks5_server(){}
};

// һ��tcp_serverӵ�ж��߳�
template <>
class socks5_server<uint16_t> : public tcp_server<uint16_t>
{
public:
	socks5_server(uint16_t thread_num, const std::string& tcp_listen_addr, const std::string& bindcmd_bindcmd_tcp_listen_ip, const std::string& assocaitecmd_udp_listen_ip, const std::shared_ptr<socks5_server_auth> auth, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _bindcmd_tcp_listen_ip(bindcmd_bindcmd_tcp_listen_ip), _assocaitecmd_udp_listen_ip(assocaitecmd_udp_listen_ip), _auth(auth),
		tcp_server(thread_num, tcp_listen_addr, std::bind(&socks5_server::channel_factory, this, std::placeholders::_1), backlog, defer_accept,
		self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_server()
	{
	}

private:
	std::string							_bindcmd_tcp_listen_ip;
	std::string							_assocaitecmd_udp_listen_ip;
	std::shared_ptr<socks5_server_auth> _auth;

	complex_ptr<tcp_server_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
	{
		socks5_server_init_channel* channel = new socks5_server_init_channel(reactor, _bindcmd_tcp_listen_ip, _assocaitecmd_udp_listen_ip, _auth);
		return (tcp_server_handler_base*)channel;
	}
};

template <>
class socks5_server<reactor_loop> : public tcp_server<reactor_loop>
{
public:
	// addr��ʽip:port
	socks5_server(std::shared_ptr<reactor_loop> reactor, const std::string& tcp_listen_addr, const std::string& bindcmd_tcp_listen_ip, const std::string& assocaitecmd_udp_listen_ip, const std::shared_ptr<socks5_server_auth> auth, uint32_t backlog = DEFAULT_LISTEN_BACKLOG, uint32_t defer_accept = DEFAULT_DEFER_ACCEPT,
		const uint32_t self_read_size = DEFAULT_READ_BUFSIZE, const uint32_t self_write_size = DEFAULT_WRITE_BUFSIZE, const uint32_t sock_read_size = 0, const uint32_t sock_write_size = 0)
		: _bindcmd_tcp_listen_ip(bindcmd_tcp_listen_ip), _assocaitecmd_udp_listen_ip(assocaitecmd_udp_listen_ip), _auth(auth),
		tcp_server(reactor, tcp_listen_addr, std::bind(&socks5_server::channel_factory, this, std::placeholders::_1), backlog, defer_accept,
		self_read_size, self_write_size, sock_read_size, sock_write_size)
	{
	}
	virtual ~socks5_server()
	{
	}

private:
	std::string							_bindcmd_tcp_listen_ip;
	std::string							_assocaitecmd_udp_listen_ip;
	std::shared_ptr<socks5_server_auth> _auth;

	complex_ptr<tcp_server_handler_base> channel_factory(std::shared_ptr<reactor_loop> reactor)
	{
		socks5_server_init_channel* channel = new socks5_server_init_channel(reactor, _bindcmd_tcp_listen_ip, _assocaitecmd_udp_listen_ip, _auth);
		return (tcp_server_handler_base*)channel;
	}
};