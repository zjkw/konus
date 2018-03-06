#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mutex>
#include "udp_helper.h"
#include "udp_channel_base.h"

#define DEFAULT_ONCERECV_SIZE	(1024)

// ĿǰΪ�õ�self_write_size
udp_channel_base::udp_channel_base(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: _fd(INVALID_SOCKET), _local_addr(local_addr), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size), _read_thunk(new buffer_thunk)
{

}

//������Ҫ�����ڲ����߳�
udp_channel_base::~udp_channel_base()
{
}

bool	udp_channel_base::init_socket()
{
	// 1���
	if (INVALID_SOCKET != _fd)
	{
		return true;
	}

	SOCKET s = INVALID_SOCKET;
	if (_local_addr.empty())
	{
		s = create_udp_socket();
	}
	else
	{
		struct sockaddr_in	si;
		if (!sockaddr_from_string(_local_addr, si))
		{
			return false;
		}

		// 2����listen socket
		s = create_udp_socket();
		if (INVALID_SOCKET != s)
		{
			if (!bind_sock(s, si))
			{
				::close(s);
				return false;
			}
		}
	}
	if (INVALID_SOCKET == s)
	{
		return false;
	}

	// 3����
	set_fd(s);

	return true;
}

void	udp_channel_base::set_fd(SOCKET fd)
{
	if (fd != _fd)
	{
		if (INVALID_SOCKET != fd)
		{
			assert(INVALID_SOCKET == _fd);
			if (_sock_write_size)
			{
				set_socket_sendbuflen(fd, _sock_write_size);
			}
			if (_sock_read_size)
			{
				set_socket_recvbuflen(fd, _sock_read_size);
			}
		}
		_fd = fd;
	}
}

// ��֤ԭ�ӣ����Ƕ��̻߳����£�buf�����һ�������������������ܴ�������/�쳣 on_error
int32_t		udp_channel_base::send_raw(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	return do_send(data, peer_addr);
}

int32_t		udp_channel_base::connect(const sockaddr_in& server_addr)
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	int32_t ret = ::connect(_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	else
	{
		return 0;
	}
}

int32_t		udp_channel_base::send_raw(const std::shared_ptr<buffer_thunk>& data)
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	return do_send(data);
}

int32_t		udp_channel_base::do_send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr)
{
	while (1)
	{
		int32_t ret = ::sendto(_fd, data->ptr(), data->used(), 0, (struct sockaddr *)&peer_addr, sizeof(peer_addr));
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
#if 0
#if EAGAIN == EWOULDBLOCK
			else if (errno == EAGAIN)
#else
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				break;
			}
#endif
			return (int32_t)CEC_WRITE_FAILED;
		}
		else
		{
			break;
		}
	}
	return data->used();
}

// tbd check udp_bind mode
int32_t		udp_channel_base::do_send(const std::shared_ptr<buffer_thunk>& data)
{
	while (1)
	{
		int32_t ret = ::send(_fd, data->ptr(), data->used(), MSG_NOSIGNAL);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
#if 0
#if EAGAIN == EWOULDBLOCK
			else if (errno == EAGAIN)
#else
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				break;
			}
#endif
			return (int32_t)CEC_WRITE_FAILED;
		}
		else
		{
			break;
		}
	}
	return data->used();
}

void udp_channel_base::close()
{
	if (INVALID_SOCKET != _fd)
	{
		::close(_fd);
		_fd = INVALID_SOCKET;
	}
}

// ѭ���п���fd�ᱻ�ı䣬��Ϊon_recv_buff���ܴ����ϲ�ִ��close������Чfd��һ������ֵ��recv����ʶ�������
int32_t	udp_channel_base::do_recv()
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	int32_t total_read = 0;
	
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	while (true)
	{
		_read_thunk->prepare(DEFAULT_ONCERECV_SIZE);
		int32_t ret = ::recvfrom(_fd, (char*)_read_thunk->ptr(), DEFAULT_ONCERECV_SIZE, 0, (struct sockaddr*)&addr, &addr_len);
		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
#if EAGAIN == EWOULDBLOCK
			else if (errno == EAGAIN)
#else
			else if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				break;
			}
			assert(errno != EFAULT);
			return (int32_t)CEC_READ_FAILED;		
		}
		else if (ret == 0)
		{
			break;
		}
		else
		{
			total_read += ret;
			_read_thunk->incr(ret);
		}

		ret = on_recv_buff(_read_thunk, addr);
		if(ret < 0)
		{
			return ret;
		}
	}

	return total_read;
}

bool	udp_channel_base::peer_addr(std::string& addr)
{
	return peeraddr_from_fd(_fd, addr);
}
bool	udp_channel_base::local_addr(std::string& addr)
{
	return localaddr_from_fd(_fd, addr);
}