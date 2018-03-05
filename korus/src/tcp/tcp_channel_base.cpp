#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "tcp_helper.h"
#include "tcp_channel_base.h"

#define DEFAULT_BUFFER_SIZE		(64)
#define DEFAULT_ONCERECV_SIZE	(1024)

tcp_channel_base::tcp_channel_base(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: _fd(INVALID_SOCKET), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size), _read_thunk(new buffer_thunk), _write_thunk(new buffer_thunk)
{
	set_fd(fd);
}

//������Ҫ�����ڲ����߳�
tcp_channel_base::~tcp_channel_base()
{
}

void	tcp_channel_base::set_fd(SOCKET fd)
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
int32_t		tcp_channel_base::send_raw(const std::shared_ptr<buffer_thunk>& data)
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	
	//��Ϊһ���Ż��������������������±��� ���뱾�ػ�����ִ��send�������ݡ�	
	int32_t real_write = 0;
	if (!_write_thunk->used())
	{
		int32_t ret = do_send(data);
		if (ret < 0)
		{
			return ret;
		}
		else
		{
			real_write += ret;
		}
	}
	int32_t errno_old = errno;	

	_write_thunk->push_back(data->ptr() + real_write, data->used() - real_write);
	
	if (!real_write)		//���û���ù�send������һ��
	{
#if EAGAIN == EWOULDBLOCK
		if (errno_old == EAGAIN)
#else
		if (errno_old == EAGAIN || errno_old == EWOULDBLOCK)
#endif
		{
		}
		else
		{
			int32_t ret = do_send(_write_thunk);
			if (ret >= 0)
			{
				_write_thunk->pop_front(ret);
			}
			else
			{
				return ret;	//С��0�������Ƿ�洢�ڱ��أ�һ����Ϊ��ʧ��
			}
		}
	}

	return (int32_t)data->used();
}

int32_t		tcp_channel_base::do_send(const std::shared_ptr<buffer_thunk>& data)
{
	int32_t real_write = 0;
	while (real_write != data->used())
	{
		int32_t ret = ::send(_fd, data->ptr() + real_write, data->used() - real_write, MSG_NOSIGNAL);
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

			return (int32_t)CEC_WRITE_FAILED;
		}
		else
		{
			real_write += ret;
		}
	}
	return real_write;
}

int32_t	tcp_channel_base::send_alone()
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	int32_t ret = do_send(_write_thunk);
	if (ret > 0)
	{
		_write_thunk->pop_front(ret);
	}
	return ret;
}

void tcp_channel_base::close()
{
	send_alone();
	if (INVALID_SOCKET != _fd)
	{
		::close(_fd);
		_fd = INVALID_SOCKET;
	}
}

void tcp_channel_base::shutdown(int32_t howto)
{
	if (INVALID_SOCKET != _fd)
	{
		::shutdown(_fd, howto);
	}
}

// ѭ���п���fd�ᱻ�ı䣬��Ϊon_recv_buff���ܴ����ϲ�ִ��close������Чfd��һ������ֵ��recv����ʶ�������
int32_t	tcp_channel_base::do_recv()
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	
	int32_t total_read = 0;
	
	bool not_again = true;
	bool peer_close = false;
	while (not_again)
	{
		int32_t real_read = 0;
		while (true)
		{
			_read_thunk->prepare(DEFAULT_ONCERECV_SIZE);
			int32_t ret = ::recv(_fd, (char*)_read_thunk->ptr(), DEFAULT_ONCERECV_SIZE, MSG_NOSIGNAL);
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
					not_again = false;
					break;
				}
				assert(errno != EFAULT);
				return (int32_t)CEC_READ_FAILED;		//maybe Connection reset by peer / bad file desc
			}
			else if (ret == 0)
			{
				peer_close = true;
				not_again = false;
				break;
			}
			else
			{
				real_read += ret;
				_read_thunk->incr(ret);
			}
		}
		if (!real_read)
		{
			break;
		}
	
		bool left_partial_pkg = false;
		int32_t ret = on_recv_buff(_read_thunk, left_partial_pkg);
		if (ret > 0)
		{
			_read_thunk->pop_front(ret);

			if (left_partial_pkg && not_again)	//��û�������ݣ��ֽ��������°����ǾͲ��ٳ���
			{
				break;
			}
		}
	}

	if (peer_close)
	{
		return CEC_CLOSE_BY_PEER;
	}

	return total_read;
}

bool	tcp_channel_base::peer_addr(std::string& addr)
{
	return peeraddr_from_fd(_fd, addr);
}
bool	tcp_channel_base::local_addr(std::string& addr)
{
	return localaddr_from_fd(_fd, addr);
}
