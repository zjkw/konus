#include <string.h>
#include "korus/inc/korus.h"

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))  
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))  
#endif

class tcp_server_handler : public tcp_server_handler_base
{
public:
	tcp_server_handler(){}
	virtual ~tcp_server_handler(){}

	//override------------------
	virtual void	on_init()
	{
	}
	virtual void	on_final()
	{
	}
	virtual bool	can_delete(bool force, long call_ref_count)//forceΪ���ʾǿ�Ʋ�ѯ������ĸ���˳�
	{
		// ��Ϊû�б������������ã���������ڿ��Ҫ�����˳�,��forceΪ�棬������������ȥ�������
		return true;
	}
	virtual void	on_accept()	//�����Ѿ�����
	{
		char szTest[] = "hello client, i am server!";
		int32_t ret = send(szTest, strlen(szTest));
		printf("\nConnected/accepted, then Send %s, ret: %d\n", szTest, ret);
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
		char szTest[1024] = { 0 };
		memcpy(szTest, buf, min(len, 1023));
		printf("\non_recv_pkg: %s, len: %u\n", szTest, len);
	}
};

std::shared_ptr<tcp_server_handler_base> channel_factory()
{
	std::shared_ptr<tcp_server_handler> handler = std::make_shared<tcp_server_handler>();
	std::shared_ptr<tcp_server_handler_base> cb = std::dynamic_pointer_cast<tcp_server_handler_base>(handler);
	return cb;
}

int main(int argc, char* argv[]) 
{
	std::string	addr = "0.0.0.0:9099";

	if (argc != 2) 
	{
		printf("Usage: %s <port>\n", argv[0]);
		printf("  e.g: %s 9099\n", argv[0]);
		return 0;
	}

	addr = std::string("0.0.0.0:") + argv[1];
	
	std::shared_ptr<reactor_loop> reactor = std::make_shared<reactor_loop>();	
	tcp_server<reactor_loop> server(reactor, addr, channel_factory);
	server.start();
	reactor->run();

	return 0;
}