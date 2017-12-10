#pragma once

#include <assert.h>
#include "tcp_channel_base.h"
#include "korus/src/util/chain_sharedobj_base.h"
#include "korus/src/reactor/reactor_loop.h"
#include "korus/src/reactor/sockio_helper.h"

//
//			<---prev---			<---prev---
// origin				....					terminal	
//			----next-->			----next-->
//

class tcp_server_handler_base;

using tcp_server_channel_factory_t = std::function<std::shared_ptr<tcp_server_handler_base>(std::shared_ptr<reactor_loop> reactor)>;

//������û����Բ����������ǿ��ܶ��̻߳����²�����������shared_ptr����Ҫ��֤�̰߳�ȫ
//���ǵ�send�����ڹ����̣߳�close�����̣߳�Ӧ�ų�ͬʱ���в��������Խ��������������˻��⣬�����Ļ�����
//1����send�����������½����ǿ���������Ļ��棬���ǵ����ں˻��棬��ͬ��::send������
//2��close/shudown�������ǿ��̵߳ģ������ӳ���fd�����߳�ִ�У���������޷�����ʵʱЧ���������ⲿ��close����ܻ���send

//�ⲿ���ȷ��tcp_server�ܰ���channel/handler�����ڣ������ܱ�֤��Դ���գ����������õ�����(channel��handler)û�п���ƵĽ�ɫ����������ʱ���ĺ�������check_detach_relation

// ���ܴ��ڶ��̻߳�����
// on_error���ܴ��� tbd������closeĬ�ϴ���
class tcp_server_handler_base : public chain_sharedobj_base<tcp_server_handler_base>
{
public:
	tcp_server_handler_base(std::shared_ptr<reactor_loop> reactor);
	virtual ~tcp_server_handler_base();

	//override------------------
	virtual void	on_chain_init();
	virtual void	on_chain_final();	
	virtual void	on_chain_zomby();
	virtual void	on_accept();
	virtual void	on_closed();
	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code);
	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len);
	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len);

	virtual int32_t	send(const void* buf, const size_t len);
	virtual void	close();
	virtual void	shutdown(int32_t howto);
	std::shared_ptr<reactor_loop>	reactor();

	virtual bool	peer_addr(std::string& addr);
	virtual bool	local_addr(std::string& addr);
	
private:
	std::shared_ptr<reactor_loop>		_reactor;
};

//��Ч�����ȼ���is_valid > INVALID_SOCKET,�����к����������ж�is_valid���Ǹ�ԭ�Ӳ���
class tcp_server_channel : public tcp_channel_base, public tcp_server_handler_base, public multiform_state
{
public:
	tcp_server_channel(std::shared_ptr<reactor_loop> reactor, SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_server_channel();

	virtual bool		start();
	// ���������������������ڶ��̻߳�����	
	virtual int32_t		send(const void* buf, const size_t len);// ��֤ԭ��, ��Ϊ������������ֵ��<0�ο�CHANNEL_ERROR_CODE
	virtual void		close();
	virtual void		shutdown(int32_t howto);// �����ο�ȫ�ֺ��� ::shutdown
	virtual bool		peer_addr(std::string& addr);
	virtual bool		local_addr(std::string& addr);

private:
	friend class tcp_server_channel_creator;
	virtual void		set_release();

	sockio_helper	_sockio_helper;
	virtual void on_sockio_read(sockio_helper* sockio_id);
	virtual void on_sockio_write(sockio_helper* sockio_id);

	virtual	int32_t	on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg);

	void handle_close_strategy(CLOSE_MODE_STRATEGY cms);
};
