#include <sys/epoll.h>
#include "epoll_imp.h"

#define DEFAULT_EPOLL_SIZE	(2000)

epoll_imp::epoll_imp()
{
	_epoll_event.resize(DEFAULT_EPOLL_SIZE);
	_fd = epoll_create1(EPOLL_CLOEXEC);	//tbd �쳣����
}

epoll_imp::~epoll_imp()
{
	clear();
}

int32_t epoll_imp::poll(int32_t mill_sec)
{
	int32_t ret = epoll_wait(_fd, &_epoll_event[0], _epoll_event.size(), mill_sec);
	if (ret < 0)
	{
		if (EINTR != errno)
		{
			// tbd failed:
		}
	}
	else
	{
		for (int32_t i = 0; i < ret; ++i)
		{
			sockio_channel* channel = (channel)_epoll_event[i].data.ptr;
			assert(channel);

			if (_epoll_event[i].events & (EPOLLERR | EPOLLHUP))
			{			
				_epoll_event[i].events |= EPOLLIN | EPOLLOUT;
			}

#if 0
			if (_epoll_event[i].events & ~(EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP)) 
			{
				// log
			}
#endif
			// �Ա�nginx�����Ǵ���[i]��ʱ����Ҫ�����Ƿ�Ӱ��[i]����Ķ����Ƿ���ڣ���һ���߳��еĶ����Ƿ������listen��idle/server_client�������ھ���
			if (_epoll_event[i].events & (POLLERR | EPOLLIN)) 
			{	
				channel->on_sockio_read();
			}
			if (events & EPOLLOUT)
			{
				channel->on_sockio_write();
			}			
		}
		// �ο�muduo���ݣ��´�poll�����и���fd����
		if (ret == (int32_t)_epoll_event.size())
		{
			_epoll_event.resize(_epoll_event.size() * 2);
		}
	}

	return ret;
}

// ��ʼ����
void epoll_imp::add_sock(sockio_channel* channel, SOCKIO_TYPE type)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = convert_event_flag(type);
	ev.data.ptr = channel;
	int32_t ret = epoll_ctl(channel->get_fd(), EPOLL_CTL_ADD, fd, &ev);
}

// ���ڸ���
void epoll_imp::upt_type(sockio_channel* channel, SOCKIO_TYPE type)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = convert_event_flag(type);
	ev.data.ptr = channel;
	int32_t ret = epoll_ctl(channel->get_fd(), EPOLL_CTL_MOD, fd, &ev);
}

// ɾ��
void epoll_imp::del_sock(sockio_channel* channel)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = 0;
	ev.data.ptr = channel;
	int32_t ret = epoll_ctl(channel->get_fd(), EPOLL_CTL_MOD, fd, &ev);
}

void epoll_imp::clear()
{
	if (INVALID_SOCKET != _fd)
	{
		::close(_fd);
		_fd = INVALID_SOCKET;
	}
}

inline uint32_t	epoll_imp::convert_event_flag(SOCKIO_TYPE type)
{
	// Ĭ��ETģʽ����Ҫ��recv/send���ʹ��
	uint32_t flag = EPOLLET;
	switch (type)
	{
	case SIT_NONE:
		assert(false);
		break;
	case SIT_READ:
		flag |= EPOLLIN;
		break;
	case SIT_WRITE:
		flag |= EPOLLOUT;
		break;
	case SIT_READWRITE:
		flag |= EPOLLIN | EPOLLOUT;
		break;
	default:
		assert(false);
		break;
	}

	return flag;
}