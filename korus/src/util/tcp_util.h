#pragma once

#include <stdint.h>
#include <string>

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef WIN32
#include <winsock2.h>
#else
typedef int SOCKET;
#endif

#define INVALID_SOCKET			(-1)

#define DEFAULT_READ_BUFSIZE	(100 * 1025)
#define DEFAULT_WRITE_BUFSIZE	(100 * 1025)

//�����ر�˵�����ڲ���������close/shutdown
enum CHANNEL_ERROR_CODE
{
	CEC_NONE = 0,
	CEC_CLOSE_BY_PEER = -1,	//�Զ˹ر�
	CEC_RECVBUF_SHORT = -2,	//���ջ���������
	CEC_SENDBUF_SHORT = -3,	//���ͻ���������
	CEC_SPLIT_FAILED = -4,	//���ջ�����������ʧ��
	CEC_WRITE_FAILED = -5,	//д�쳣
	CEC_READ_FAILED = -6,	//���쳣
	CEC_INVALID_SOCKET = -7,//��Чsocket
};

bool set_reuse_port_sock(SOCKET fd, int reuse);

bool set_reuse_addr_sock(SOCKET fd, int reuse);

bool set_linger_sock(SOCKET fd, int onoff, int linger);

bool sockaddr_from_string(const std::string& address, struct sockaddr_in& si);

bool listen_sock(SOCKET fd, int backlog);

bool bind_sock(SOCKET fd, const struct sockaddr_in& addr);

bool set_defer_accept_sock(SOCKET fd, int32_t defer);

SOCKET listen_nonblock_reuse_socket(uint32_t backlog, uint32_t defer_accept, const struct sockaddr_in& addr);

SOCKET accept_sock(SOCKET fd, struct sockaddr_in* addr);

SOCKET create_general_tcp_socket(const struct sockaddr_in& addr);

bool set_nonblock_sock(SOCKET fd, unsigned int nonblock);

bool set_socket_sendbuflen(SOCKET fd, uint32_t buf_size);

bool set_socket_recvbuflen(SOCKET fd, uint32_t buf_size);