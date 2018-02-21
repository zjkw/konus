#include <assert.h>
#include "udp_server_channel.h"

//////////////////////////////////base
udp_server_handler_base::udp_server_handler_base(std::shared_ptr<reactor_loop> reactor)
	: _reactor(reactor)
{
}

udp_server_handler_base::~udp_server_handler_base()
{ 
}	

//override------------------
void	udp_server_handler_base::on_chain_init()
{
}

void	udp_server_handler_base::on_chain_final()
{
}

void	udp_server_handler_base::on_ready()
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_ready(); 
}

void	udp_server_handler_base::on_closed()
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_closed(); 
}

//�ο�CHANNEL_ERROR_CODE����
CLOSE_MODE_STRATEGY	udp_server_handler_base::on_error(CHANNEL_ERROR_CODE code)
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return CMS_INNER_AUTO_CLOSE;
	}

	return _tunnel_next->on_error(code);
}

//����һ���������������
void	udp_server_handler_base::on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{ 
	if (!_tunnel_next)
	{
		assert(false);
		return;
	}

	_tunnel_next->on_recv_pkg(buf, len, peer_addr);
}

bool	udp_server_handler_base::start()
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}
	
	return _tunnel_prev->start();
}

int32_t	udp_server_handler_base::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)	
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}
	
	return _tunnel_prev->send(buf, len, peer_addr);
}

int32_t	udp_server_handler_base::connect(const sockaddr_in& server_addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return 0;
	}

	return _tunnel_prev->connect(server_addr);
}

int32_t	udp_server_handler_base::send(const void* buf, const size_t len)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return 0;
	}

	return _tunnel_prev->send(buf, len);
}

void	udp_server_handler_base::close()
{ 
	if (!_tunnel_prev)
	{
		assert(false);
		return;
	}
	
	_tunnel_prev->close();
}

bool	udp_server_handler_base::local_addr(std::string& addr)
{
	if (!_tunnel_prev)
	{
		assert(false);
		return false;
	}

	return _tunnel_prev->local_addr(addr);
}

std::shared_ptr<reactor_loop>	udp_server_handler_base::reactor()		
{ 
	return _reactor; 
}

//////////////////////////////////channel
udp_server_handler_origin::udp_server_handler_origin(std::shared_ptr<reactor_loop> reactor, const std::string& local_addr, const uint32_t self_read_size/* = DEFAULT_READ_BUFSIZE*/, const uint32_t self_write_size/* = DEFAULT_WRITE_BUFSIZE*/, const uint32_t sock_read_size/* = 0*/, const uint32_t sock_write_size/* = 0*/)
	: udp_server_handler_base(reactor), 
	udp_channel_base(local_addr, self_read_size, self_write_size, sock_read_size, sock_write_size)
										
{
	_sockio_helper.bind(std::bind(&udp_server_handler_origin::on_sockio_read, this, std::placeholders::_1), nullptr);
	set_prepare();
}

//������Ҫ�����ڲ����߳�
udp_server_handler_origin::~udp_server_handler_origin()
{
}

bool	udp_server_handler_origin::start()
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
		// udp_server_handler_origin������һ���reactor�̣����Լ������ü���
		reactor()->start_async_task(std::bind(&udp_server_handler_origin::start, this), this);
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
int32_t	udp_server_handler_origin::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}
	
	int32_t ret = udp_channel_base::send(buf, len, peer_addr);
	return ret;
}

int32_t	udp_server_handler_origin::connect(const sockaddr_in& server_addr)								// ��֤ԭ��, ����ֵ��<0�ο�CHANNEL_ERROR_CODE
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	int32_t ret = udp_channel_base::connect(server_addr);
	return ret;
}

int32_t	udp_server_handler_origin::send(const void* buf, const size_t len)								// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	int32_t ret = udp_channel_base::send(buf, len);
	return ret;
}

void	udp_server_handler_origin::close()
{	
	if (!reactor())
	{
		assert(false);
		return;
	}

	//�̵߳��ȣ����ڷ���˵����Ӷ��ԣ�close����ζ���������������������ӵĿ�����
	if (!reactor()->is_current_thread())
	{
		// udp_server_handler_origin������һ���reactor�̣����Լ������ü���
		reactor()->start_async_task(std::bind(&udp_server_handler_origin::close, this), this);
		return;
	}	

	set_release();
}

bool	udp_server_handler_origin::local_addr(std::string& addr)
{
	if (!is_normal())
	{
		return false;
	}
	if (!reactor()->is_current_thread())
	{
		return false;
	}
	return udp_channel_base::local_addr(addr);
}

void	udp_server_handler_origin::on_sockio_read(sockio_helper* sockio_id)
{
	if (!is_normal())
	{
		assert(false);
		return;
	}

	int32_t ret = udp_channel_base::do_recv();
	if (ret < 0)
	{
		if (is_normal())
		{
			CLOSE_MODE_STRATEGY cms = on_error((CHANNEL_ERROR_CODE)ret);
			handle_close_strategy(cms);
		}
	}
}

void	udp_server_handler_origin::set_release()
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

int32_t	udp_server_handler_origin::on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	if (!is_normal())
	{
		assert(false);
		return CEC_INVALID_SOCKET;
	}

	on_recv_pkg((uint8_t*)buf, len, peer_addr);
		
	return len;
}

void udp_server_handler_origin::handle_close_strategy(CLOSE_MODE_STRATEGY cms)
{
	if (CMS_INNER_AUTO_CLOSE == cms)
	{
		close();	//�ڲ����Զ������Ч��
	}
}

///////////////
udp_server_handler_terminal::udp_server_handler_terminal(std::shared_ptr<reactor_loop> reactor) : udp_server_handler_base(reactor)
{

}

udp_server_handler_terminal::~udp_server_handler_terminal()
{
	chain_final();
}

//override------------------
void	udp_server_handler_terminal::on_chain_init()
{

}

void	udp_server_handler_terminal::on_chain_final()
{

}

void	udp_server_handler_terminal::on_ready()
{

}

void	udp_server_handler_terminal::on_closed()
{

}

//�ο�CHANNEL_ERROR_CODE����
CLOSE_MODE_STRATEGY	udp_server_handler_terminal::on_error(CHANNEL_ERROR_CODE code)
{
	return CMS_INNER_AUTO_CLOSE;
}

//����һ���������������
void	udp_server_handler_terminal::on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{

}

bool	udp_server_handler_terminal::start()
{
	return udp_server_handler_base::start();
}

int32_t	udp_server_handler_terminal::send(const void* buf, const size_t len, const sockaddr_in& peer_addr)
{
	return udp_server_handler_base::send(buf, len, peer_addr);
}

int32_t	udp_server_handler_terminal::connect(const sockaddr_in& server_addr)								// ��֤ԭ��, ����ֵ��<0�ο�CHANNEL_ERROR_CODE
{
	return udp_server_handler_base::connect(server_addr);
}

int32_t	udp_server_handler_terminal::send(const void* buf, const size_t len)
{
	return udp_server_handler_base::send(buf, len);
}

void	udp_server_handler_terminal::close()
{
	udp_server_handler_base::close();
}

bool	udp_server_handler_terminal::local_addr(std::string& addr)
{
	return udp_server_handler_base::local_addr(addr);
}

//override------------------
void	udp_server_handler_terminal::chain_inref()
{
	this->shared_from_this().inref();
}

void	udp_server_handler_terminal::chain_deref()
{
	this->shared_from_this().deref();
}

void	udp_server_handler_terminal::on_release()
{
	//Ĭ��ɾ��,����֮
}