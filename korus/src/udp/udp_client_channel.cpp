#include <assert.h>
#include "udp_client_channel.h"

/////////////////////////base
udp_client_handler_base::udp_client_handler_base() 
	: _reactor(nullptr), _tunnel_prev(nullptr), _tunnel_next(nullptr)
{
	set_prepare();
}

udp_client_handler_base::~udp_client_handler_base()
{ 
	// ����ִ��inner_final
	assert(!_tunnel_prev); 
	assert(!_tunnel_next); 
}	

//override------------------
void	udp_client_handler_base::on_init()
{
}

void	udp_client_handler_base::on_final()
{
}

void	udp_client_handler_base::on_ready()	
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	
	_tunnel_prev->on_ready(); 
}

void	udp_client_handler_base::on_closed()	
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->on_closed();
}

//�ο�CHANNEL_ERROR_CODE����
CLOSE_MODE_STRATEGY	udp_client_handler_base::on_error(CHANNEL_ERROR_CODE code)
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _tunnel_prev->on_error(code); 
}

//����һ���������������
void	udp_client_handler_base::on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}

	_tunnel_prev->on_recv_pkg(buf, len, peer_addr); 
}

int32_t	udp_client_handler_base::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}
	
	return _tunnel_next->send(buf, len, peer_addr);
}

void	udp_client_handler_base::close()
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}
	
	_tunnel_next->close();
}

std::shared_ptr<reactor_loop>	udp_client_handler_base::reactor()	
{ 
	return _reactor; 
}

bool	udp_client_handler_base::can_delete(long call_ref_count)			//��ͷ����б�Ķ������ôˣ���Ҫ����
{
	// ��������Ϊrelease����,Ҫ���ⲿ���������ڹ���
	if (!is_release())
	{
		return false;
	}

	long ref = 0;
	if (_tunnel_prev)
	{
		ref++;
	}
	if (_tunnel_next)
	{
		ref++;
	}
	if (call_ref_count + ref + 1 == shared_from_this().use_count())
	{
		// �������������ϲ�ѯ
		if (_tunnel_prev)
		{
			return _tunnel_prev->can_delete(0);
		}
		else
		{
			return true;
		}
	}

	return false;
}

void	udp_client_handler_base::inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_client_handler_base> tunnel_prev, std::shared_ptr<udp_client_handler_base> tunnel_next)
{
	if (!is_prepare())
	{
		assert(false);
		return;
	}

	_reactor = reactor;
	_tunnel_prev = tunnel_prev;
	_tunnel_next = tunnel_next;

	on_init();
}
void	udp_client_handler_base::inner_final()
{
	// ����ǰ��is_release�жϣ����ŵ�����
	if (_tunnel_prev)
	{
		_tunnel_prev->inner_final();
	}
	_reactor = nullptr;
	_tunnel_prev = nullptr;
	_tunnel_next = nullptr;

	on_final();
}
////////////////////////channel

udp_client_channel::udp_client_channel(const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size)
	: udp_channel_base("", self_read_size, self_write_size, sock_read_size, sock_write_size)

{
	_sockio_helper.bind(std::bind(&udp_client_channel::on_sockio_read, this, std::placeholders::_1), nullptr);
	set_prepare();
}

//������Ҫ�����ڲ����߳�
udp_client_channel::~udp_client_channel()
{
}

bool	udp_client_channel::start()
{
	if (!is_prepare())
	{
		return false;
	}

	if (!reactor())
	{
		assert(false);
		return false;
	}

	//�̵߳��ȣ����ڷ���˵����Ӷ��ԣ�close����ζ���������������������ӵĿ�����
	if (!reactor()->is_current_thread())
	{
		// udp_client_channel������һ���reactor�̣����Լ������ü���
		reactor()->start_async_task(std::bind(&udp_client_channel::start, this), this);
		return true;
	}
	
	_sockio_helper.reactor(reactor().get());

	bool ret = udp_channel_base::init_socket();
	if (ret)
	{
		set_normal();
		_sockio_helper.set(_fd);
		_sockio_helper.start(SIT_READ);
		on_ready();
	}
	return ret;
}

// ��֤ԭ�ӣ����Ƕ��̻߳����£�buf�����һ�������������������ܴ�������/�쳣 on_error
int32_t	udp_client_channel::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return 0;
	}

	int32_t ret = udp_channel_base::send(buf, len, peer_addr);
	return ret;
}

void	udp_client_channel::close()
{
	if (!is_normal())
	{
		assert(false);
		return;
	}

	if (!reactor())
	{
		assert(false);
		return;
	}

	//�̵߳��ȣ����ڷ���˵����Ӷ��ԣ�close����ζ���������������������ӵĿ�����
	if (!reactor()->is_current_thread())
	{
		// udp_client_channel������һ���reactor�̣����Լ������ü���
		reactor()->start_async_task(std::bind(&udp_client_channel::close, this), this);
		return;
	}

	set_release();
}

void	udp_client_channel::on_sockio_read(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}

	int32_t ret = udp_channel_base::do_recv();
	if (ret < 0)
	{
		CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
		handle_close_strategy(cms);
	}
}

void	udp_client_channel::set_release()
{
	if (is_release() || is_dead())
	{
		assert(false);
		return;
	}

	multiform_state::set_release();
	_sockio_helper.clear();
	reactor()->stop_async_task(this);
	udp_channel_base::close();
	on_closed();
}

int32_t	udp_client_channel::on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return 0;
	}

	on_recv_pkg((uint8_t*)buf, len, peer_addr);

	return len;
}

void udp_client_channel::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//�ڲ����Զ������Ч��
	}
}