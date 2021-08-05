#pragma once
#include<Windows.h>
#include<winternl.h>
#include<map>

#include<string>
#include<tchar.h>

#include <iphlpapi.h>
#include <vector>
#include <set>
#include <stdint.h>

class CNetUsage
{
	typedef struct _net_info_t {
		std::string strDescr;
		IF_INDEX dwIndex;
		IFTYPE dwType;
		DWORD dwInOctets;
		DWORD dwOutOctets;
		DWORD dwAdminStatus;
		DWORD dwOperStatus;
		DWORD dwSpeed;
	}net_info_t;

	typedef std::vector<net_info_t> v_net_info_t;
	typedef std::set<std::string> set_string;

public:
	CNetUsage();
	~CNetUsage();
	
	void start();
	void pause();
	void continue_();
	void stop();

	DWORD getUp();
	DWORD getDown();

	void cb();
private:
	v_net_info_t getNetinfos(const set_string& names);
	set_string CNetUsage::getAdapterNames();
	bool exist(const CNetUsage::set_string& arr, const std::string& target);

	void loadUpDown(const CNetUsage::set_string& names, DWORD&, DWORD&);
private:

	DWORD m_up;
	DWORD m_down;

	HANDLE m_hThread;
};
