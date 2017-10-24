#include <assert.h>
#include "udp_server_channel.h"

udp_server_channel::udp_server_channel(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, std::shared_ptr<udp_server_callback> cb,
										const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
										: _reactor(reactor), _cb(cb), udp_channel_base(local_addr, self_read_size, self_write_size, sock_read_size, sock_write_size)
										
{
}

//������Ҫ�����ڲ����߳�
udp_server_channel::~udp_server_channel()
{
	_reactor->stop_async_task(this);
	_reactor->stop_sockio(this);
}

bool	udp_server_channel::start()
{
	if (!is_valid())
	{
		return false;
	}

	//�̵߳��ȣ����ڷ���˵����Ӷ��ԣ�close����ζ���������������������ӵĿ�����
	if (!_reactor->is_current_thread())
	{
		// udp_client_channel������һ���reactor�̣����Լ������ü���
		_reactor->start_async_task(std::bind(&udp_server_channel::start, this), this);
		return true;
	}
	bool ret = udp_channel_base::init_socket();
	if (ret)
	{
		_reactor->start_sockio(this, SIT_READ);
		_cb->on_ready(shared_from_this());
	}
	return ret;
}

// ��֤ԭ�ӣ����Ƕ��̻߳����£�buf�����һ�������������������ܴ�������/�쳣 on_error
int32_t	udp_server_channel::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_valid())
	{
		return 0;
	}
	
	int32_t ret = udp_channel_base::send(buf, len, peer_addr);
	return ret;
}

void	udp_server_channel::close()
{	
	if (!is_valid())
	{
		return;
	}

	//�̵߳��ȣ����ڷ���˵����Ӷ��ԣ�close����ζ���������������������ӵĿ�����
	if (!_reactor->is_current_thread())
	{
		// udp_server_channel������һ���reactor�̣����Լ������ü���
		_reactor->start_async_task(std::bind(&udp_server_channel::close, this), this);
		return;
	}	

	invalid();
}

//////////////////////////////////
void	udp_server_channel::on_sockio_write()
{
	assert(false); //������
}

void	udp_server_channel::on_sockio_read()
{
	if (!is_valid())
	{
		return;
	}
	int32_t ret = udp_channel_base::do_recv();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = _cb->on_error((CHANNEL_ERROR_CODE)ret, shared_from_this());
		handle_close_strategy(cms);
	}
}

void	udp_server_channel::invalid()
{
	if (!is_valid())
	{
		return;
	}
	thread_safe_objbase::invalid();
	_reactor->stop_sockio(this);
	udp_channel_base::close();
	_cb->on_closed(shared_from_this());
}

int32_t	udp_server_channel::on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_valid())
	{
		return 0;
	}

	_cb->on_recv_pkg((uint8_t*)buf, len, peer_addr, shared_from_this());
		
	return len;
}

void udp_server_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//�ڲ����Զ������Ч��
	}
}