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
	virtual void	on_init()
	{
	}
	virtual void	on_final()
	{
	}
	virtual void	on_connect()	//连接已经建立
	{
		char szTest[] = "hello server, i am client!";
		int32_t ret = send(szTest, strlen(szTest));
		printf("\nConnected, then Send %s, ret: %d\n", szTest, ret);
	}

	virtual void	on_closed()
	{
		printf("\nClosed\n");
	}

	//参考CHANNEL_ERROR_CODE定义
	virtual CLOSE_MODE_STRATEGY	on_error(CHANNEL_ERROR_CODE code)
	{
		printf("\nError code: %d\n", (int32_t)code);
		return CMS_INNER_AUTO_CLOSE;
	}

	//提取数据包：返回值 =0 表示包不完整； >0 完整的包(长)
	virtual int32_t on_recv_split(const void* buf, const size_t len)
	{
		printf("\non_recv_split len: %u\n", len);
		return (int32_t)len;
	}

	//这是一个待处理的完整包
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

	if (argc != 2) 
	{
		printf("Usage: %s <port>\n", argv[0]);
		printf("  e.g: %s 9099\n", argv[0]);
		return 0;
	}

	addr = std::string("127.0.0.1:") + argv[1];
			
	std::shared_ptr<reactor_loop> reactor = std::make_shared<reactor_loop>();

	tcp_client<reactor_loop> client(reactor, addr, channel_factory);

	reactor->run();

	return 0;
}