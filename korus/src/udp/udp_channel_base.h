#pragma once

#include "korus/src/util/basic_defines.h"
#include "korus/src/util/buffer_thunk.h"

// ���ຯ�������ж�fd��Ч�ԣ��������ฺ��
// ����send���Դ��ڷǴ����̣߳������������ڵ�ǰ�̣߳�����recv���Բ�����
class udp_channel_base
{
public:
	udp_channel_base(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_channel_base();

	// ���������������������ڶ��̻߳�����	
	virtual	int32_t		send_raw(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);	// �ⲿ���ݷ���
	virtual int32_t		connect(const sockaddr_in& server_addr);								
	virtual int32_t		send_raw(const std::shared_ptr<buffer_thunk>& data);								// �ⲿ���ݷ���
	virtual	void		close();
	virtual	int32_t		on_recv_buff(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr) = 0;
	virtual bool		peer_addr(std::string& addr);
	virtual bool		local_addr(std::string& addr);

protected:
	//>0 ��ʾ�����Լ���recv,=0��ʾ��ȡ�����ˣ�<0��ʾ����			
	SOCKET				_fd;
	void				set_fd(SOCKET fd);
	bool				init_socket();
	int32_t				do_recv();

private:
	std::shared_ptr<buffer_thunk> _read_thunk;

	std::string			_local_addr;	
	uint32_t			_sock_read_size;
	uint32_t			_sock_write_size;

	int32_t				do_send(const std::shared_ptr<buffer_thunk>& data, const sockaddr_in& peer_addr);
	int32_t				do_send(const std::shared_ptr<buffer_thunk>& data);	
};
