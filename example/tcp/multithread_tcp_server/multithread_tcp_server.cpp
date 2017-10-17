#include "konus/inc/konus.h"

class tcp_server_handler : public tcp_server_callback
{
public:
	tcp_server_handler(){}
	virtual ~tcp_server_handler(){}

	//override------------------
	virtual void	on_accept(std::shared_ptr<tcp_server_channel> channel)	//�����Ѿ�����
	{

	}

	virtual void	on_closed(std::shared_ptr<tcp_server_channel> channel)
	{

	}

	//�ο�TCP_ERROR_CODE����
	virtual void	on_error(CHANNEL_ERROR_CODE code, std::shared_ptr<tcp_server_channel> channel)
	{

	}

	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len, std::shared_ptr<tcp_server_channel> channel)
	{
		return 0;
	}

	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len, std::shared_ptr<tcp_server_channel> channel)
	{

	}
};

int main(int argc, char* argv[]) 
{
	std::string	addr = "0.0.0.0:9099";
	uint16_t		thread_num = 4;

	if (argc != 3) 
	{
		printf("Usage: %s <port> <thread-num>\n", argv[0]);
		printf("  e.g: %s 9099 12\n", argv[0]);
		return 0;
	}

	addr = std::string("0.0.0.0:") + argv[1];
	thread_num = (uint16_t)atoi(argv[2]);
		
	std::shared_ptr<tcp_server_handler> handler = std::make_shared<tcp_server_handler>();
	std::shared_ptr<tcp_server_callback> cb = std::dynamic_pointer_cast<tcp_server_callback>(handler);
	tcp_server<uint16_t> server(thread_num, addr, cb);
	for (;;)
	{

	}
	return 0;
}