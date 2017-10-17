#pragma once

#include "korus/src/util/tcp_util.h"

class sockio_channel
{
public:
	sockio_channel(){}
	virtual ~sockio_channel(){}
	virtual void on_sockio_read(){}
	virtual void on_sockio_write(){}
	virtual SOCKET	get_fd() = 0;
};

enum SOCKIO_TYPE
{
	SIT_NONE = 0,
	SIT_READ = 1,
	SIT_WRITE = 2,
	SIT_READWRITE = 3
};

class backend_poller
{
public:
	backend_poller();
	virtual ~backend_poller();

	virtual int32_t poll(int32_t mill_sec) = 0;
	// ��ʼ����
	virtual void add_sock(sockio_channel* channel, SOCKIO_TYPE type) = 0;
	// ���ڸ���
	virtual void upt_type(sockio_channel* channel, SOCKIO_TYPE type) = 0;
	// ɾ��
	virtual void del_sock(sockio_channel* channel) = 0;
	virtual void clear() = 0;
};

