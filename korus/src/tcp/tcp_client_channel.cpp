#include <assert.h>
#include <functional>
#include "tcp_helper.h"
#include "tcp_client_channel.h"

tcp_client_channel::tcp_client_channel(std::shared_ptr<reactor_loop> reactor, const std::string& server_addr, std::shared_ptr<tcp_client_handler_base> cb,
	std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
	const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: tcp_channel_base(INVALID_SOCKET, self_read_size, self_write_size, sock_read_size, sock_write_size), 
	_conn_fd(INVALID_SOCKET), _server_addr(server_addr), _conn_state(CNS_CLOSED), _reactor(reactor), _cb(cb), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait),
	_timer_connect_timeout(reactor.get()), _timer_connect_retry_wait(reactor.get())
{
	_timer_connect_timeout.bind(std::bind(&tcp_client_channel::on_timer_connect_timeout, this, std::placeholders::_1));
	_timer_connect_retry_wait.bind(std::bind(&tcp_client_channel::on_timer_connect_retry_wait, this, std::placeholders::_1));
}

tcp_client_channel::~tcp_client_channel()
{
	assert(!_reactor);
	assert(!_cb);
}

// ��֤ԭ�ӣ����Ƕ��̻߳����£�buf�����һ�������������������ܴ�������/�쳣 on_error
int32_t	tcp_client_channel::send(const void* buf, const size_t len)
{
	if (!is_valid())
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	if (CNS_CONNECTED != _conn_state)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}

	return tcp_channel_base::send(buf, len);
}

void	tcp_client_channel::close()
{
	if (!is_valid())
	{
		return;
	}
	if (CNS_CLOSED == _conn_state)
	{
		return;
	}
	//�̵߳���
	if (!_reactor->is_current_thread())
	{
		_reactor->start_async_task(std::bind(&tcp_client_channel::close, this), this);
		return;
	}

	printf("stop_sockio, %d\n", __LINE__);
	_reactor->stop_sockio(this);
	printf("close fd, %d, Line: %d\n", _fd, __LINE__);
	tcp_channel_base::close();
	if (INVALID_SOCKET != _conn_fd)
	{
		printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
		::close(_conn_fd);
		_conn_fd = INVALID_SOCKET;
	}
	_conn_state = CNS_CLOSED;

	_cb->on_closed();
}

// �����ο�ȫ�ֺ��� ::shutdown
void	tcp_client_channel::shutdown(int32_t howto)
{
	if (!is_valid())
	{
		return;
	}
	if (CNS_CONNECTED != _conn_state)
	{
		return;
	}
	//�̵߳���
	if (!_reactor->is_current_thread())
	{
		_reactor->start_async_task(std::bind(&tcp_client_channel::shutdown, this, howto), this);
		return;
	}

	tcp_channel_base::shutdown(howto);
}

void	tcp_client_channel::connect()
{
	if (!is_valid())
	{
		return;
	}
	if (CNS_CLOSED != _conn_state)
	{
		return;
	}
	//�̵߳���
	if (!_reactor->is_current_thread())
	{
		// tcp_client_channel������һ���reactor�̣����Լ������ü���
		_reactor->start_async_task(std::bind(&tcp_client_channel::connect, this), this);
		return;
	}

	_timer_connect_retry_wait.stop();

	struct sockaddr_in	si;
	if (!sockaddr_from_string(_server_addr, si))
	{
		return;
	}
	assert(INVALID_SOCKET == _conn_fd);
	_conn_fd = create_tcp_socket(si);
	if (INVALID_SOCKET == _conn_fd)
	{
		_timer_connect_retry_wait.start(_connect_retry_wait, _connect_retry_wait);
		return;
	}

	//��ʼ����
	int32_t ret = ::connect(_conn_fd, (struct sockaddr *)&si, sizeof(struct sockaddr));
	if (!ret)
	{
		set_fd(_conn_fd);
		_conn_fd = INVALID_SOCKET;
		_conn_state = CNS_CONNECTED;
		printf("start_sockio, %d\n", __LINE__);
		_reactor->start_sockio(this, SIT_READWRITE);
		_cb->on_connect();
	}
	else
	{
		if (errno == EINPROGRESS)
		{
			_conn_state = CNS_CONNECTING;	//start_sockio�����ȡfd
			printf("start_sockio, %d\n", __LINE__);
			_reactor->start_sockio(this, SIT_WRITE);
			if (_connect_timeout.count())
			{
				_timer_connect_timeout.start(_connect_timeout, _connect_timeout);
			}
		}
		else
		{
			printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
			::close(_conn_fd);
			_conn_fd = INVALID_SOCKET;

			if (_connect_retry_wait != std::chrono::seconds(-1))
			{
				_timer_connect_retry_wait.start(_connect_retry_wait, _connect_retry_wait);
			}
		}
	}
}

//����ʱ����ִ��connect�ҵȴ�״̬�£���connect����ڳ�ʱǰ���������ص���ʱ��������������ǿ���л���CLOSED
void	tcp_client_channel::on_timer_connect_timeout(timer_helper* timer_id)
{
	printf("stop_sockio, %d\n", __LINE__);
	_reactor->stop_sockio(this);
	_timer_connect_timeout.stop();

	printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
	::close(_conn_fd);
	_conn_fd = INVALID_SOCKET;
	_conn_state = CNS_CLOSED;

	_timer_connect_retry_wait.start(_connect_retry_wait, _connect_retry_wait);
}

//����ʱ�����Ѿ���ȷconnectʧ��/��ʱ�������������connectʱ��ص���ʱ���������������Զ�ִ��connect
void	tcp_client_channel::on_timer_connect_retry_wait(timer_helper* timer_id)
{
	_timer_connect_retry_wait.stop();

	connect();
}

//���������ӳɹ��󴥷���Ҳ������һ��д����
void	tcp_client_channel::on_sockio_write()
{
	if (!is_valid())
	{
		return;
	}
	printf("on_sockio_write _conn_state %d, Line: %d\n", (int)_conn_state, __LINE__);
	switch (_conn_state)
	{
	case CNS_CONNECTING:
		{
			printf("stopt_sockio, %d\n", __LINE__);
			_reactor->stop_sockio(this);
			_timer_connect_timeout.stop();

			int32_t err = 0;
			socklen_t len = sizeof(int32_t);
			if (getsockopt(_conn_fd, SOL_SOCKET, SO_ERROR, (void *)&err, &len) < 0 || err != 0)
			{
				printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
				::close(_conn_fd);
				_conn_fd = INVALID_SOCKET;
				_conn_state = CNS_CLOSED;
			}
			else
			{
				set_fd(_conn_fd);
				_conn_fd = INVALID_SOCKET;
				_conn_state = CNS_CONNECTED;
				printf("start_sockio, %d\n", __LINE__);
				_reactor->start_sockio(this, SIT_READWRITE);
				_cb->on_connect();
			}
		}
		break;
	case CNS_CONNECTED:
		{
			int32_t ret = tcp_channel_base::send_alone();
			if (ret < 0)
			{
				CLOSE_MODE_STRATEGY cms = _cb->on_error((CHANNEL_ERROR_CODE)ret);
				handle_close_strategy(cms);
			}
		}
		break;
	case CNS_CLOSED:
	default:
		assert(false);
		break;
	}
}

void	tcp_client_channel::on_sockio_read()
{
	if (!is_valid())
	{
		return;
	}
	printf("on_sockio_read _conn_state %d, Line: %d\n", (int)_conn_state, __LINE__);
	if (CNS_CONNECTED != _conn_state)
	{
		return;
	}
	int32_t ret = tcp_channel_base::do_recv();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = _cb->on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

bool	tcp_client_channel::check_detach_relation(long call_ref_count)
{
	if (!is_valid())
	{
		if (_cb)
		{
			if (_cb.unique() && shared_from_this().use_count() == 1 + 1 + call_ref_count) //1 for shared_from_this, 1 for _cb
			{
				_cb->inner_final();
				_cb = nullptr;
				
				_reactor = nullptr;

				return true;
			}
		}
		else
		{
			assert(false);
		}
	}

	return false;
}

void	tcp_client_channel::invalid()
{
	if (!is_valid())
	{
		return;
	}
	thread_safe_objbase::invalid();

	_timer_connect_timeout.stop();
	_timer_connect_retry_wait.stop();
	printf("stopt_sockio, %d\n", __LINE__);
	_reactor->stop_sockio(this);
	_reactor->stop_async_task(this);

	close();
}

int32_t	tcp_client_channel::on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg)
{
	if (!is_valid())
	{
		return 0;
	}
	if (CNS_CONNECTED != _conn_state)
	{
		return (int32_t)CEC_INVALID_SOCKET;
	}
	left_partial_pkg = false;
	int32_t size = 0;
	while (len > size)
	{
		int32_t ret = _cb->on_recv_split((uint8_t*)buf + size, len - size);
		if (ret == 0)
		{
			left_partial_pkg = true;	//ʣ�µĲ���һ�������İ�
			break;
		}
		else if (ret < 0)
		{
			CLOSE_MODE_STRATEGY cms = _cb->on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);
			break;
		}
		else if (ret + size > len)
		{
			CLOSE_MODE_STRATEGY cms = _cb->on_error(CEC_RECVBUF_SHORT);
			handle_close_strategy(cms);
			break;
		}

		_cb->on_recv_pkg((uint8_t*)buf + size, ret);
		size += ret;
	}

	return size;
}

void tcp_client_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//�ڲ����Զ������Ч��
	}
}