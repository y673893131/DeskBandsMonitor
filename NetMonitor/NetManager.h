#pragma once

#include <vector>
#include <map>
#include <string>

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

/* 4 bytes IP address */
typedef struct ip_address{
	u_char byte1;
	u_char byte2;
	u_char byte3;
	u_char byte4;
}ip_address;

typedef struct _port_net_t
{
	u_short srcPort;
	u_short destPort;
	ip_address source;
	ip_address dest;
	u_int len;
	time_t tm;
}port_net_t, *p_port_net_t;

typedef struct _net_pid_t
{
	_net_pid_t()
		:pid(0), len(0)
	{

	}
	_net_pid_t(u_long pid)
		:pid(pid), len(0)
	{
	}
	u_long pid;
	u_int len;
}net_pid_t;

typedef struct _net_pid_len_t
{
	_net_pid_len_t()
		:send(0), recv(0)
	{

	}

	u_int send;
	u_int recv;
}net_pid_len_t;

typedef struct _net_stream_t
{
	std::map<u_short, net_pid_t> port_pid;
	std::map<u_int, net_pid_len_t> pid_len;
	void clear()
	{
		port_pid.clear();
		pid_len.clear();
	}

}net_stream_t;

typedef struct _proccess_net_t
{
	net_stream_t stream;
	std::map<u_int, std::wstring> pid_name;
}proccess_net_t, *p_proccess_net_t;

//
struct _net_info_t
{
	_net_info_t()
		:up(0), down(0)
	{

	}
	u_int up;
	u_int down;
};

struct _stat_net_info_t
{
	_stat_net_info_t()
	{

	}
	_net_info_t stream;
	u_int pid;
	std::wstring name;
};

struct _monotor_info_t
{
	_net_info_t total;
	std::vector<_stat_net_info_t> details;
};
//

class CNetManager
{
public:
	CNetManager();
	~CNetManager();

	std::vector<std::string> enumNetInterface();
	bool init(const std::string& sTarget);
	void stop();
	_monotor_info_t getStatInfo();

public:
	void getNetThreadFunc();
	void statThreadFunc();

private:
	std::string iptos(u_long in);
	
	void packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data);
	void statisticNet(const port_net_t& t, bool isSend);
	
	bool __GetUdpConnect(int targetPort = 0, int nLen = 0, bool bSend = false);
	bool __GetTcpConnect(int targetPort = 0, int nLen = 0, bool bSend = false);
	void __GetPidInfo(int targetPid = 0);

private:
	u_char m_srcIP[4];
	std::string m_sTarget;
	proccess_net_t m_nets;
	_monotor_info_t m_stat;
	
	bool m_bStop;
};

