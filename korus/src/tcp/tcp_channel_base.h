#pragma once

#include "korus/src/util/basic_defines.h"
#include "korus/src/util/buffer_thunk.h"

// ���ຯ�������ж�fd��Ч�ԣ��������ฺ��
// ����send���Դ��ڷǴ����̣߳������������ڵ�ǰ�̣߳�����recv���Բ�����
class tcp_channel_base
{
public:
	tcp_channel_base(SOCKET fd, const uint32_t self_read_size, const uint32_t self_write_size, const uint32_t sock_read_size, const uint32_t sock_write_size);
	virtual ~tcp_channel_base();

	// ���������������������ڶ��̻߳�����	
	virtual	int32_t		send_raw(const std::shared_ptr<buffer_thunk>& data);// �ⲿ���ݷ���
	virtual	void		close();
	virtual	void		shutdown(int32_t howto);
	virtual	int32_t		on_recv_buff(std::shared_ptr<buffer_thunk>& data, bool& left_partial_pkg) = 0;
	
	virtual bool		peer_addr(std::string& addr);
	virtual bool		local_addr(std::string& addr);

protected:
	int32_t				send_alone();								// �ڲ����ݷ���
	SOCKET				_fd;
	void				set_fd(SOCKET fd);
	int32_t				do_recv();	//������������/�ⲿ���սᴦ����,��Ϊ�����в�����check����״̬

private:
	std::shared_ptr<buffer_thunk> _read_thunk;
	std::shared_ptr<buffer_thunk> _write_thunk;

	uint32_t			_sock_read_size;
	uint32_t			_sock_write_size;

	int32_t				do_send(const std::shared_ptr<buffer_thunk>& data);	
};
