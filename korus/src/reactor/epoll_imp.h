#pragma once

#include <vector>
#include "backend_poller.h"

class epoll_imp : public backend_poller
{
public:
	epoll_imp();
	virtual ~epoll_imp();

protected:
	virtual int32_t poll(int32_t mill_sec);
	// ��ʼ����
	virtual void add_sock(sockio_channel* channel, SOCKIO_TYPE type);
	// ���ڸ���
	virtual void upt_type(sockio_channel* channel, SOCKIO_TYPE type);
	// ɾ��
	virtual void del_sock(sockio_channel* channel);
	virtual void clear();

private:
	int								_fd;
	int32_t							_current_handler_index;//��ǰepoll_wait���ڴ���������������趨epoll_wait�������ظ���fd����
	std::vector<struct epoll_event> _epoll_event;

	uint32_t	convert_event_flag(SOCKIO_TYPE type);
};

