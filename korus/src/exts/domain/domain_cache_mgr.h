#pragma once

//����ʽ��������������ݹ������

#include <unistd.h>
#include <strings.h>
#include <string>
#include <vector>
#include <map>

class domain_cache_mgr
{
public:
	domain_cache_mgr();
	~domain_cache_mgr();

	bool ip_by_domain(const std::string& domain, std::string& ip);
	void update_cache(const std::string& domain, const std::vector<std::string>& iplist);

private:
	struct strcompr 
	{
		bool operator() (const std::string& lhs, const std::string& rhs) const 
		{
			return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
		}
	};

	struct domain_data
	{
		std::vector<std::string>	iplist;
		size_t						last_pos;
	};

	std::map<std::string, domain_data, strcompr>	_domain_list;
};

//������ڣ��ڶ��̻߳�������ζ���溯����Ҫ����
//����֧��ipv4
bool query_ip_by_domain(const std::string& domain, std::string& ip);
void update_domain_cache(const std::string& domain, const std::vector<std::string>& iplist);
