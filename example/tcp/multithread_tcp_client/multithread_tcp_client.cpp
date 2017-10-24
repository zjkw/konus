#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class tcp_client_handler : public tcp_client_handler_base
{
public:
	tcp_client_handler(){}
	virtual ~tcp_client_handler(){}

	//override------------------
	virtual void	on_connect()	//�����Ѿ�����
	{
		char szTest[] = "hello server, i am client!";
		int32_t ret = send(szTest, strlen(szTest));
		printf("\nConnected, then Send %s, ret: %d\n", szTest, ret);
	}

	virtual void	on_closed()
	{
		printf("\nClosed\n");
	}

	//�ο�CHANNEL_ERROR_CODE����
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code)
	{
		printf("\nError code: %d\n", (int32_t)code);
		return CMS_INNER_AUTO_CLOSE;
	}

	//��ȡ���ݰ�������ֵ =0 ��ʾ���������� >0 �����İ�(��)
	virtual int32_t on_recv_split(const void* buf, const size_t len)
	{
		printf("\non_recv_split len: %u\n", len);
		return (int32_t)len;
	}

	//����һ���������������
	virtual void	on_recv_pkg(const void* buf, const size_t len)
	{
		char szTest[1024] = {0};
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

std::shared_ptr<tcp_client_handler_base> channel_factory()
{
	std::shared_ptr<tcp_client_handler> handler = std::make_shared<tcp_client_handler>();
	std::shared_ptr<tcp_client_handler_base> cb = std::dynamic_pointer_cast<tcp_client_handler_base>(handler);
	return cb;
}

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

	addr = std::string("127.0.0.1:") + argv[1];
	thread_num = (uint16_t)atoi(argv[2]);
		
	tcp_client<uint16_t> client(thread_num, addr, channel_factory);
	client.start();
	for (;;)
	{
		sleep(1);
	}
	return 0;
}