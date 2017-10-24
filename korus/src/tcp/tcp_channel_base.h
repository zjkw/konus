#pragma once

#include <mutex>
#include "korus/src/util/basic_defines.h"

// ���ຯ�������ж�fd��Ч�ԣ��������ฺ��
// ����send���Դ��ڷǴ����̣߳������������ڵ�ǰ�̣߳�����recv���Բ�����
class tcp_channel_base
{
public:
	tcp_channel_base(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_channel_base();

	// ���������������������ڶ��̻߳�����	
	virtual	int32_t		send(const void* buf, const size_t len);// �ⲿ���ݷ���
	virtual	void		close();
	virtual	void		shutdown(int32_t howto);
	virtual	int32_t		on_recv_buff(const void* buf, const size_t len, bool& left_partial_pkg) = 0;
protected:
	//>0 ��ʾ�����Լ���recv,=0��ʾ��ȡ�����ˣ�<0��ʾ����
	int32_t				do_recv();									// ��������_self_read_buff������on_after_recv
	int32_t				send_alone();								// �ڲ����ݷ���
	SOCKET				_fd;
	void				set_fd(SOCKET fd);

private:
	std::mutex			_mutex_write;

	bool				_recving;
	uint32_t			_self_read_size;
	uint32_t			_self_read_pos;
	uint8_t*			_self_read_buff;
	uint32_t			_self_write_size;
	uint32_t			_self_write_pos;
	uint8_t*			_self_write_buff;

	uint32_t			_sock_read_size;
	uint32_t			_sock_write_size;

	int32_t				do_send_inlock(const void* buf, uint32_t len);
	int32_t				do_recv_nolock();
	
};
