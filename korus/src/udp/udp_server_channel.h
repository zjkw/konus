#pragma once

#include <list>
#include "udp_channel_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

class udp_server_handler_base;

using udp_server_channel_factory_t = std::function<std::shared_ptr<udp_server_handler_base>()>;
using udp_server_channel_factory_chain_t = std::list<udp_server_channel_factory_t>;

//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//�ⲿ���ȷ��udp_server�ܰ���channel/handler�����ڣ������ܱ�֤��Դ���գ����������õ�����(channel��handler)û�п���ƵĽ�ɫ����������ʱ���ĺ�������check_detach_relation

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class udp_server_handler_base : public std::enable_shared_from_this<udp_server_handler_base>, public thread_safe_objbase
{
public:
	udp_server_handler_base() : _reactor(nullptr), _tunnel_prev(nullptr), _tunnel_next(nullptr){}
	virtual ~udp_server_handler_base(){ assert(!_tunnel_prev); assert(!_tunnel_next); }	// ����ִ��inner_final

	//override------------------
	virtual void	on_init(){}
	virtual void	on_final(){}
	virtual void	on_ready()	{ if (_tunnel_prev)_tunnel_prev->on_ready(); }
	virtual void	on_closed()	{ if (_tunnel_prev)_tunnel_prev->on_closed(); }
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code)	{ if (!_tunnel_prev) return CMS_INNER_AUTO_CLOSE; return _tunnel_prev->on_error(code); }
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, const sockaddr_in& peer_addr){ if (_tunnel_prev) _tunnel_prev->on_recv_pkg(buf, len, peer_addr); }

	int32_t	send(const void* buf, const size_t len, const sockaddr_in& peer_addr)	{ if (!_tunnel_next) return CEC_INVALID_SOCKET;  return _tunnel_next->send(buf, len, peer_addr); }
	void	close()									{ if (_tunnel_next)_tunnel_next->close(); }
	std::shared_ptr<reactor_loop>	reactor()		{ return _reactor; }
	virtual bool	can_delete(long call_ref_count)			//��ͷ����б�Ķ������ôˣ���Ҫ����
	{
		if (is_valid())
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

private:
	template<typename T> friend bool build_chain(std::shared_ptr<reactor_loop> reactor, T tail, const std::list<std::function<T()> >& chain);
	template<typename T> friend class udp_server;
	void	inner_init(std::shared_ptr<reactor_loop> reactor, std::shared_ptr<udp_server_handler_base> tunnel_prev, std::shared_ptr<udp_server_handler_base> tunnel_next)
	{
		_reactor = reactor;
		_tunnel_prev = tunnel_prev;
		_tunnel_next = tunnel_next;

		on_init();
	}
	void	inner_final()
	{
		if (_tunnel_prev)
		{
			_tunnel_prev->inner_final();
		}
		_reactor = nullptr;
		_tunnel_prev = nullptr;
		_tunnel_next = nullptr;

		on_final();
	}

	std::shared_ptr<reactor_loop>		_reactor;
	std::shared_ptr<udp_server_handler_base> _tunnel_prev;
	std::shared_ptr<udp_server_handler_base> _tunnel_next;
};

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���
class udp_server_channel : public udp_channel_base, public udp_server_handler_base
{
public:
	udp_server_channel(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_server_channel();

	// �����ĸ��������������ڶ��̻߳�����	
	int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	void		close();	
	bool		start();

private:
	template<typename T> friend class udp_server;
	void		invalid();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
