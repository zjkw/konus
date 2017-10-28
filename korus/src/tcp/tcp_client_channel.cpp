#include <assert.h>
#include <functional>
#include "tcp_helper.h"
#include "tcp_client_channel.h"

tcp_client_channel::tcp_client_channel(const std::string& server_addr, std::chrono::seconds connect_timeout, std::chrono::seconds connect_retry_wait,
	const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: tcp_channel_base(INVALID_SOCKET, self_read_size, self_write_size, sock_read_size, sock_write_size),
	_conn_fd(INVALID_SOCKET), _server_addr(server_addr), _conn_state(CNS_CLOSED), _connect_timeout(connect_timeout), _connect_retry_wait(connect_retry_wait)
{
	_timer_connect_timeout.bind(std::bind(&tcp_client_channel::on_timer_connect_timeout, this, std::placeholders::_1));
	_timer_connect_retry_wait.bind(std::bind(&tcp_client_channel::on_timer_connect_retry_wait, this, std::placeholders::_1));

	_sockio_helper_connect.bind(nullptr, std::bind(&tcp_client_channel::on_sockio_write_connect, this, std::placeholders::_1));
	_sockio_helper.bind(std::bind(&tcp_client_channel::on_sockio_read, this, std::placeholders::_1), std::bind(&tcp_client_channel::on_sockio_write, this, std::placeholders::_1));
}

tcp_client_channel::~tcp_client_channel()
{
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
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_client_channel::close, this), this);
		return;
	}

	printf("stop_sockio, %d\n", __LINE__);
	_sockio_helper_connect.stop();
	_sockio_helper_connect.set(INVALID_SOCKET);
	_sockio_helper.stop();
	_sockio_helper.set(INVALID_SOCKET);
	printf("close fd, %d, Line: %d\n", _fd, __LINE__);
	tcp_channel_base::close();
	if (INVALID_SOCKET != _conn_fd)
	{
		printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
		::close(_conn_fd);
		_conn_fd = INVALID_SOCKET;
	}
	_conn_state = CNS_CLOSED;

	on_closed();
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
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_client_channel::shutdown, this, howto), this);
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
	if (!reactor())
	{
		assert(false);
		return;
	}
	if (CNS_CLOSED != _conn_state)
	{
		return;
	}
	//�̵߳���
	if (!reactor()->is_current_thread())
	{
		// tcp_client_channel������һ���reactor�̣����Լ������ü���
		reactor()->start_async_task(std::bind(&tcp_client_channel::connect, this), this);
		return;
	}
	_sockio_helper_connect.reactor(reactor().get());
	_sockio_helper.reactor(reactor().get());
	_timer_connect_timeout.reactor(reactor().get());
	_timer_connect_retry_wait.reactor(reactor().get());

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
		_sockio_helper.set(_conn_fd);
		_conn_fd = INVALID_SOCKET;
		_conn_state = CNS_CONNECTED;
		printf("start_sockio, %d\n", __LINE__);
		_sockio_helper.start(SIT_READWRITE);
		on_connect();
	}
	else
	{
		if (errno == EINPROGRESS)
		{
			_conn_state = CNS_CONNECTING;	//start_sockio�����ȡfd
			printf("start_sockio, %d\n", __LINE__);
			_sockio_helper_connect.set(_conn_fd);
			_sockio_helper_connect.start(SIT_WRITE);
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
	_sockio_helper_connect.stop();
	_timer_connect_timeout.stop();

	printf("close fd, %d, Line: %d\n", _conn_fd, __LINE__);
	::close(_conn_fd);
	_conn_fd = INVALID_SOCKET;
	_sockio_helper_connect.set(INVALID_SOCKET);
	_conn_state = CNS_CLOSED;

	_timer_connect_retry_wait.start(_connect_retry_wait, _connect_retry_wait);
}

//����ʱ�����Ѿ���ȷconnectʧ��/��ʱ�������������connectʱ��ص���ʱ���������������Զ�ִ��connect
void	tcp_client_channel::on_timer_connect_retry_wait(timer_helper* timer_id)
{
	_timer_connect_retry_wait.stop();

	connect();
}

void tcp_client_channel::on_sockio_write_connect(sockio_helper* sockio_id)
{
	if (!is_valid())
	{
		return;
	}
	printf("on_sockio_write _conn_state %d, Line: %d\n", (int)_conn_state, __LINE__);
	if (CNS_CONNECTING != _conn_state)
	{
		assert(false);
		return;
	}

	printf("stopt_sockio, %d\n", __LINE__);
	_sockio_helper_connect.stop();
	_sockio_helper_connect.set(INVALID_SOCKET);
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
		_sockio_helper.set(_conn_fd);
		_conn_fd = INVALID_SOCKET;
		_conn_state = CNS_CONNECTED;
		printf("start_sockio, %d\n", __LINE__);

		_sockio_helper.start(SIT_READWRITE);
		on_connect();
	}
}

void	tcp_client_channel::on_sockio_write(sockio_helper* sockio_id)
{
	if (!is_valid())
	{
		return;
	}
	printf("on_sockio_write _conn_state %d, Line: %d\n", (int)_conn_state, __LINE__);
	if (CNS_CONNECTED != _conn_state)
	{
		assert(false);
		return;
	}

	int32_t ret = tcp_channel_base::send_alone();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

void	tcp_client_channel::on_sockio_read(sockio_helper* sockio_id)
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
		CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

void	tcp_client_channel::invalid()
{
	if (!is_valid())
	{
		return;
	}
	double_state::invalid();

	_timer_connect_timeout.stop();
	_timer_connect_retry_wait.stop();
	printf("stopt_sockio, %d\n", __LINE__);
	_sockio_helper_connect.clear();
	_sockio_helper.clear();
	reactor()->stop_async_task(this);

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
		int32_t ret = on_recv_split((uint8_t*)buf + size, len - size);
		if (ret == 0)
		{
			left_partial_pkg = true;	//ʣ�µĲ���һ�������İ�
			break;
		}
		else if (ret < 0)
		{
			CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);
			break;
		}
		else if (ret + size > len)
		{
			CLOSE_MODE_STRATEGY cms = on_error(CEC_RECVBUF_SHORT);
			handle_close_strategy(cms);
			break;
		}

		on_recv_pkg((uint8_t*)buf + size, ret);
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