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

#define DEFAULT_READ_BUFSIZE	(100 * 1024)
#define DEFAULT_WRITE_BUFSIZE	(100 * 1024)

//�����ر�˵�����ڲ���������close/shutdown
enum CHANNEL_ERROR_CODE
{
	CEC_NONE = 0,
	CEC_CLOSE_BY_PEER = -1,	//�Զ˹ر�
	CEC_RECVBUF_SHORT = -2,	//���ջ���������
	CEC_SENDBUF_FULL = -3,	//���ͻ���������
	CEC_RECVBUF_FULL = -4,	//���ջ���������
	CEC_SPLIT_FAILED = -5,	//���ջ�����������ʧ��
	CEC_WRITE_FAILED = -6,	//д�쳣
	CEC_READ_FAILED = -7,	//���쳣
	CEC_INVALID_SOCKET = -8,//��Чsocket

	CEC_SOCKS5_METHOD_NOSUPPORTED = -9,//û��֧�ֵķ���
	CEC_SOCKS5_AUTH_FAILED = -10,//��֤ʧ��
	CEC_SOCKS5_REQUEST_FAILED = -11,//��ͨ��SOCKS����������ʧ��
	CEC_SOCKS5_REGUAL_DISALLOW = -12,//���еĹ������������
	CEC_SOCKS5_NET_UNREACHABLE = -13,//���粻�ɴ�
	CEC_SOCKS5_HOST_UNREACHABLE = -14,//�������ɴ�
	CEC_SOCKS5_CONN_REFUSE = -15,//���ӱ���
	CEC_SOCKS5_TTL_TIMEOUT = -16,//TTL��ʱ
	CEC_SOCKS5_CMD_NOSUPPORTED = -17,//��֧�ֵ����ʱ
	CEC_SOCKS5_ADDR_NOSUPPORTED = -18,//��֧�ֵĵ�ַ����
	CEC_SOCKS5_UNKNOWN = -19,//δ����
};

enum CLOSE_MODE_STRATEGY
{
	CMS_MANUAL_CONTROL = 0,	//�ⲿ�û��ֹ�����
	CMS_INNER_AUTO_CLOSE = 1,//�ڲ��Զ��ر�
};
