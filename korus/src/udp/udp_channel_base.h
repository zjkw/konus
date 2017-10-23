#pragma once

#include <mutex>
#include "korus/src/util/basic_defines.h"

// ���ຯ�������ж�fd��Ч�ԣ��������ฺ��
// ����send���Դ��ڷǴ����̣߳������������ڵ�ǰ�̣߳�����recv���Բ�����
class udp_channel_base
{
public:
	udp_channel_base(const std::string& local_addr, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~udp_channel_base();

	// �����ĸ��������������ڶ��̻߳�����	
	virtual	int32_t		send(const void* buf, const size_t len, const sockaddr_in& peer_addr);// �ⲿ���ݷ���
	virtual	void		close();
	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, const sockaddr_in& peer_addr) = 0;
protected:
	//>0 ��ʾ�����Լ���recv,=0��ʾ��ȡ�����ˣ�<0��ʾ����
	int32_t				do_recv();									
	int32_t				send_alone();								// �ڲ����ݷ���
	SOCKET				_fd;
	void				set_fd(SOCKET fd);
	bool				bind_local_addr();

private:
	std::mutex			_mutex_write;

	std::string			_local_addr;
	uint32_t			_self_read_size;
	uint8_t*			_self_read_buff;
	uint32_t			_sock_read_size;
	uint32_t			_sock_write_size;

	int32_t				do_send_inlock(const void* buf, uint32_t len, const sockaddr_in& peer_addr);
	int32_t				do_recv_nolock();
	
};
