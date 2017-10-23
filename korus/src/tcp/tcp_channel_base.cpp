#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <mutex>
#include "tcp_helper.h"
#include "tcp_channel_base.h"

tcp_channel_base::tcp_channel_base(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: _fd(INVALID_SOCKET), _recving(false),
	_self_read_size(self_read_size), _self_write_size(self_write_size), _self_read_pos(0), _self_write_pos(0), _sock_read_size(sock_read_size), _sock_write_size(sock_write_size),
	_self_read_buff(new uint8_t[self_read_size]),
	_self_write_buff(new uint8_t[self_write_size])
{
	set_fd(fd);
}

//������Ҫ�����ڲ����߳�
tcp_channel_base::~tcp_channel_base()
{
	delete[]_self_read_buff;
	delete[]_self_write_buff;
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
		std::unique_lock <std::mutex> lck(_mutex_write);
		_fd = fd;
	}
}

// ��֤ԭ�ӣ����Ƕ��̻߳����£�buf�����һ�������������������ܴ�������/�쳣 on_error
int32_t		tcp_channel_base::send(const void* buf, const size_t len)
{
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	// ��ϣ�����˵��û��������ݷ��ͣ����Ӧ�õ���send_alone
	if (_self_write_buff <= (uint8_t*)buf && _self_write_buff + _self_write_size > (uint8_t*)buf)
	{
		assert(false);
		return (int32_t)CEC_WRITE_FAILED;
	}
	
	//���ǵ�::sendʧ�ܣ������µ�ճ��������Ҫ��ʣ��ռ���빻
	if (len + _self_write_pos >= _self_write_size)
	{
		return (int32_t)CEC_SENDBUF_FULL;
	}
	//��Ϊһ���Ż��������������������±��� ���뱾�ػ�����ִ��send�������ݡ�	
	int32_t real_write = 0;
	if (!_self_write_pos)
	{
		int32_t ret = do_send_inlock(buf, len);
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
	memcpy(_self_write_buff + _self_write_pos, (uint8_t*)buf + real_write, len - real_write);
	_self_write_pos += len - real_write;
	
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
			int32_t ret = do_send_inlock((const void*)_self_write_buff, _self_write_pos);
			if (ret >= 0)
			{
				_self_write_pos -= ret;
				memmove(_self_write_buff, _self_write_buff + ret, _self_write_pos);
			}
			else
			{
				return ret;	//С��0�������Ƿ�洢�ڱ��أ�һ����Ϊ��ʧ��
			}
		}
	}

	return len;
}

int32_t		tcp_channel_base::do_send_inlock(const void* buf, uint32_t	len)
{
	int32_t real_write = 0;
	while (real_write != len)
	{
		int32_t ret = ::send(_fd, (const char*)buf + real_write, len - real_write, MSG_NOSIGNAL);
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
	std::unique_lock <std::mutex> lck(_mutex_write);
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	int32_t ret = do_send_inlock(_self_write_buff, _self_write_pos);
	if (ret > 0)
	{
		_self_write_pos -= ret;
		memmove(_self_write_buff, _self_write_buff + ret, _self_write_pos);
	}
	return ret;
}

void tcp_channel_base::close()
{
	send_alone();
	std::unique_lock <std::mutex> lck(_mutex_write);
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

int32_t	tcp_channel_base::do_recv()
{
	if (INVALID_SOCKET == _fd)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	//������������on_recv_buff��ֱ�ӻ��ӵ������е��ñ�����
	if (_recving)
	{
		assert(false);
		return CEC_READ_FAILED;
	}
	_recving = true;
	int32_t	ret = do_recv_nolock();	
	_recving = false;
	return ret;
}

// ѭ���п���fd�ᱻ�ı䣬��Ϊon_recv_buff���ܴ����ϲ�ִ��close������Чfd��һ������ֵ��recv����ʶ�������
int32_t	tcp_channel_base::do_recv_nolock()
{
	int32_t total_read = 0;
	
	bool not_again = true;
	bool peer_close = false;
	while (not_again)
	{
		int32_t real_read = 0;
		while (_self_read_size > _self_read_pos)
		{
			int32_t ret = ::recv(_fd, (char*)_self_read_buff + _self_read_pos, _self_read_size - _self_read_pos, MSG_NOSIGNAL);
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
				_self_read_pos += (uint32_t)ret;
			}
		}
		if (!real_read)
		{
			break;
		}
	
		bool left_partial_pkg = false;
		int32_t ret = on_recv_buff(_self_read_buff, _self_read_pos, left_partial_pkg);
		if (ret > 0)
		{
			memmove(_self_read_buff, _self_read_buff + ret, _self_read_pos - ret);
			_self_read_pos -= ret;

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
	if (_self_read_pos == _self_read_size)
	{
		return CEC_RECVBUF_FULL;
	}

	return total_read;
}
