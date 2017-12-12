#pragma once

#include <strings.h>
#include <map>
#include "socks5_server_channel.h"

//��Ա�����tcp/udp server���е� addr<--->object��
//�̲߳���ȫ

class socks5_server_data_mgr
{
public:
	socks5_server_data_mgr(){}
	virtual ~socks5_server_data_mgr(){}

	void	set_bindcmd_line(const std::string& addr, std::shared_ptr<socks5_server_channel> channel);
	std::shared_ptr<socks5_server_channel>	gac_bindcmd_line(const std::string& addr);	//��ȡ�����
	void	clr_bindcmd_line(const std::string& addr);
	void	clr_bindcmd_line(std::shared_ptr<socks5_server_channel> channel);

private:
	struct str_comp
	{
		bool operator() (const std::string& lhs, const std::string& rhs) const
		{
			return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};
	struct channel_ptr_cmp
	{
		bool operator()(const std::shared_ptr<socks5_server_channel> &l, const std::shared_ptr<socks5_server_channel> &r) const
		{
			return l.get() < r.get();
		}
	};
	std::map<std::string, std::shared_ptr<socks5_server_channel>, str_comp>			_forward;
	std::map<std::shared_ptr<socks5_server_channel>, std::string, channel_ptr_cmp>	_inverte;
};

//interface
void	set_bindcmd_line(const std::string& addr, std::shared_ptr<socks5_server_channel> channel);
std::shared_ptr<socks5_server_channel>	gac_bindcmd_line(const std::string& addr);	//��ȡ�����
void	clr_bindcmd_line(const std::string& addr);
void	clr_bindcmd_line(std::shared_ptr<socks5_server_channel> channel);