#include <assert.h>
#include "tcp_server_channel.h"

///////////////////////////base
tcp_server_handler_base::tcp_server_handler_base(std::shared_ptr<reactor_loop> reactor)
	: _reactor(reactor)
{
}

tcp_server_handler_base::~tcp_server_handler_base()
{
	// ����ִ��chain_final
	assert(!_tunnel_next);
	assert(!_tunnel_prev);
}

//override------------------
void	tcp_server_handler_base::on_chain_init()
{
}

void	tcp_server_handler_base::on_chain_final() 
{
}

void	tcp_server_handler_base::on_chain_zomby()
{

}

void	tcp_server_handler_base::on_accept()	
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_accept();
}

void	tcp_server_handler_base::on_closed()
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_closed();
}

//�ο�CHANNEL_ERROR_CODE����
CLOSE_MODE_STRATEGY	tcp_server_handler_base::on_error(CHANNEL_ERROR_CODE code)	
{
	if (!_tunnel_next)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _tunnel_next->on_error(code);
}

//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
int32_t tcp_server_handler_base::on_recv_split(const void* buf, const size_t len)	
{
	if (!_tunnel_next)
	{
		assert(false);
		return CEC_SPLIT_FAILED;
	}

	return _tunnel_next->on_recv_split(buf, len);
}

//����һ���������������
void	tcp_server_handler_base::on_recv_pkg(const void* buf, const size_t len)
{
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	return _tunnel_next->on_recv_pkg(buf, len);
}


int32_t	tcp_server_handler_base::send(const void* buf, const size_t len)	
{
	if (!_tunnel_prev)
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	return _tunnel_prev->send(buf, len);
}

void	tcp_server_handler_base::close()								
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->close();
}

void	tcp_server_handler_base::shutdown(int32_t howto)				
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->shutdown(howto);
}

std::shared_ptr<reactor_loop>	tcp_server_handler_base::reactor()
{
	return _reactor;
}

///////////////////////////channel
tcp_server_channel::tcp_server_channel(std::shared_ptr<reactor_loop> reactor, SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: tcp_server_handler_base(reactor), tcp_channel_base(fd, self_read_size, self_write_size, sock_read_size, sock_write_size)
										
{
	assert(INVALID_SOCKET != fd);
	_sockio_helper.set(fd);
	_sockio_helper.bind(std::bind(&tcp_server_channel::on_sockio_read, this, std::placeholders::_1), std::bind(&tcp_server_channel::on_sockio_write, this, std::placeholders::_1));

	set_prepare();
}

//������Ҫ�����ڲ����߳�
tcp_server_channel::~tcp_server_channel()
{
}

bool tcp_server_channel::start()
{
	if (!is_prepare())
	{
		assert(false);
		return false;
	}
	if (!reactor())
	{
		assert(false);
		return false;
	}

	_sockio_helper.reactor(reactor().get());
	_sockio_helper.start(SIT_READWRITE);

	set_normal();

	return true;
}

// ��֤ԭ�ӣ����Ƕ��̻߳����£�buf�����һ�������������������ܴ�������/�쳣 on_error
int32_t	tcp_server_channel::send(const void* buf, const size_t len)
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}
	
	int32_t ret = tcp_channel_base::send(buf, len);
	return ret;
}

void	tcp_server_channel::close()
{	
	if (is_dead())
	{
		return;
	}

	//�̵߳��ȣ����ڷ���˵����Ӷ��ԣ�close����ζ���������������������ӵĿ�����
	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_server_channel::close, this), this);
		return;
	}	

	set_release();
}

// �����ο�ȫ�ֺ��� ::shutdown
void	tcp_server_channel::shutdown(int32_t howto)
{
	if (!is_prepare() && !is_normal())
	{
		return;
	}

	if (!reactor()->is_current_thread())
	{
		reactor()->start_async_task(std::bind(&tcp_server_channel::shutdown, this, howto), this);
		return;
	}
	tcp_channel_base::shutdown(howto);
}

//////////////////////////////////
void	tcp_server_channel::on_sockio_write(sockio_helper* sockio_id)
{
	if (!is_normal())
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

void	tcp_server_channel::on_sockio_read(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}
	int32_t ret = tcp_channel_base::do_recv();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

void	tcp_server_channel::set_release()
{
	if (is_dead())
	{
		return;
	}

	multiform_state::set_release();

	_sockio_helper.clear();
	reactor()->stop_async_task(this);
	tcp_channel_base::close();
	on_closed();
}

int32_t	tcp_server_channel::on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg)
{
	if (!is_normal())
	{
		return CEC_INVALID_SOCKET;
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

void tcp_server_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//�ڲ����Զ������Ч��
	}
}

