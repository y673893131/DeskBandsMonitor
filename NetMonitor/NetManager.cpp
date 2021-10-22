#include "NetManager.h"
#include <thread>
extern "C"
{
#define HAVE_REMOTE
#define WPCAP
	#include "pcap.h"
}

#pragma comment(lib, "wpcap.lib")

#include <TlHelp32.h>

#include <WinSock.h>
#pragma comment(lib, "ws2_32.lib") 

///* 4 bytes IP address */
//typedef struct ip_address{
//	u_char byte1;
//	u_char byte2;
//	u_char byte3;
//	u_char byte4;
//}ip_address;

/* IPv4 header */
typedef struct ip_header{
	u_char	ver_ihl;		// Version (4 bits) + Internet header length (4 bits)
	u_char	tos;			// Type of service 
	u_short tlen;			// Total length 
	u_short identification; // Identification
	u_short flags_fo;		// Flags (3 bits) + Fragment offset (13 bits)
	u_char	ttl;			// Time to live
	u_char	proto;			// Protocol
	u_short crc;			// Header checksum
	ip_address	saddr;		// Source address
	ip_address	daddr;		// Destination address
	u_int	op_pad;			// Option + Padding
}ip_header;

/* UDP header*/
typedef struct udp_header{
	u_short sport;			// Source port
	u_short dport;			// Destination port
	u_short len;			// Datagram length
	u_short crc;			// Checksum
}udp_header;

// TCP首部
typedef struct tcp_header{
	u_short  source_port;       // (16 bits)                         Winsock 内置函数 ntohs（），主要作用将大端转换为小端！
	u_short  destination_port;  // (16 bits)                         Winsock 内置函数 ntohs（），主要作用将大端转换为小端！
	u_long	 seq_number;        // Sequence Number (32 bits)         大小端原因，高低位4个8bit的存放顺序是反的，intel使用小端模式
	u_long	 ack_number;        // Acknowledgment Number (32 bits)     大小端原因，高低位4个8bit的存放顺序是反的，intel使用小端模式
	u_short  info_ctrl;         // Data Offset (4 bits), Reserved (6 bits), Control bits (6 bits)                intel使用小端模式
	u_short  window;            // (16 bits)
	u_short  checksum;          // (16 bits)
	u_short  urgent_pointer;    // (16 bits)
} tcp_header;

CRITICAL_SECTION m_lock;
HANDLE m_hStatThread = NULL;
HANDLE m_hFilterThread = NULL;

CNetManager::CNetManager()
	:m_bStop(false)
{
	InitializeCriticalSection(&m_lock);
	auto s = enumNetInterface();
	init(s[0]);
}

CNetManager::~CNetManager()
{
	DeleteCriticalSection(&m_lock);

	if (m_hStatThread)
	{
		CloseHandle(m_hStatThread);
		m_hStatThread = NULL;
	}

	if (m_hFilterThread)
	{
		CloseHandle(m_hFilterThread);
		m_hFilterThread = NULL;
	}
}

std::string CNetManager::iptos(u_long in)
{
	struct in_addr net_ip_address;
	net_ip_address.s_addr = in;
	std::string sIP = inet_ntoa(net_ip_address);
	return sIP;
}

std::vector<std::string> CNetManager::enumNetInterface()
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	int i = 0;
	char errbuf[PCAP_ERRBUF_SIZE];
	char packet_filter[] = "ip and (udp or tcp)";

	/* Retrieve the device list */
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		return std::vector<std::string>();
	}

	std::vector<std::string> dess;
	/* Print the list */
	for (d = alldevs; d; d = d->next)
	{
		dess.push_back(d->description);
	}

	pcap_freealldevs(alldevs);
	return dess;
}

DWORD WINAPI statCallback(LPVOID lpThreadParameter)
{
	auto pThis = reinterpret_cast<CNetManager*>(lpThreadParameter);
	pThis->statThreadFunc();
	return 0;
}

DWORD WINAPI filterCallback(LPVOID lpThreadParameter)
{
	auto pThis = reinterpret_cast<CNetManager*>(lpThreadParameter);
	pThis->getNetThreadFunc();
	return 0;
}

bool CNetManager::init(const std::string& sTarget)
{
	m_sTarget = sTarget;
	DWORD statThreadId = 0;
	DWORD filterThreadId = 0;
	m_hStatThread = CreateThread(nullptr, 0, statCallback, this, 0, &statThreadId);
	m_hFilterThread = CreateThread(nullptr, 0, filterCallback, this, 0, &filterThreadId);

	return true;
}

void CNetManager::stop()
{
	m_bStop = true;

	if (m_hStatThread)
	{
		TerminateThread(m_hStatThread, 2);
		WaitForSingleObject(m_hStatThread, INFINITE);
	}

	if (m_hFilterThread)
	{
		TerminateThread(m_hFilterThread, 2);
		WaitForSingleObject(m_hFilterThread, INFINITE);
	}
}

void CNetManager::getNetThreadFunc()
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	int inum;
	int i = 0;
	pcap_t *adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];
	u_int netmask;
	char packet_filter[] = "ip and (udp or tcp)";
	struct bpf_program fcode;

	/* Retrieve the device list */
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		return ;
	}

	/* Print the list */
	for (d = alldevs; d; d = d->next)
	{
		//ifprint(d);
		if (d->description && d->description != m_sTarget)
			continue;

		for (auto address = d->addresses; address; address = address->next)
		{
			if (address->addr->sa_family != AF_INET)
				continue;

			auto sIP = iptos(((struct sockaddr_in *)address->addr)->sin_addr.s_addr);
			auto ip = ((struct sockaddr_in *)address->addr)->sin_addr;
			m_srcIP[0] = ip.S_un.S_un_b.s_b1;
			m_srcIP[1] = ip.S_un.S_un_b.s_b2;
			m_srcIP[2] = ip.S_un.S_un_b.s_b3;
			m_srcIP[3] = ip.S_un.S_un_b.s_b4;
			printf(" IP :%s\n", sIP.c_str());
			auto sMask = iptos(((struct sockaddr_in *)address->netmask)->sin_addr.s_addr);
			printf("mask:%s\n", sMask.c_str());
		}
		printf("%d. %s", ++i, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}

	if (i == 0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
		return ;
	}

	inum = 1;
	/* Jump to the selected adapter */
	for (d = alldevs, i = 0; i< inum - 1; d = d->next, i++);

	/* Open the adapter */
	if ((adhandle = pcap_open(d->name,	// name of the device
		65536,		// portion of the packet to capture. 
		// 65536 grants that the whole packet will be captured on all the MACs.
		PCAP_OPENFLAG_PROMISCUOUS,			// promiscuous mode
		1000,		// read timeout
		NULL,		// remote authentication
		errbuf		// error buffer
		)) == NULL)
	{
		fprintf(stderr, "\nUnable to open the adapter. %s is not supported by WinPcap\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return ;
	}

	/* Check the link layer. We support only Ethernet for simplicity. */
	if (pcap_datalink(adhandle) != DLT_EN10MB)
	{
		fprintf(stderr, "\nThis program works only on Ethernet networks.\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return ;
	}

	if (d->addresses != NULL)
		/* Retrieve the mask of the first address of the interface */
		netmask = ((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	else
		/* If the interface is without addresses we suppose to be in a C class network */
		netmask = 0xffffff;


	//compile the filter
	if (pcap_compile(adhandle, &fcode, packet_filter, 1, netmask) <0)
	{
		fprintf(stderr, "\nUnable to compile the packet filter. Check the syntax.\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return ;
	}

	//set the filter
	if (pcap_setfilter(adhandle, &fcode)<0)
	{
		fprintf(stderr, "\nError setting the filter.\n");
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return ;
	}

	printf("\nlistening on %s...\n", d->description);

	/* At this point, we don't need any more the device list. Free it */
	pcap_freealldevs(alldevs);

	/* start the capture */
	//pcap_loop(adhandle, 0, packet_handler, NULL);

	int res;
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	while ((res = pcap_next_ex(adhandle, &header, &pkt_data)) >= 0){
		if (m_bStop)
			break;

		if (res == 0)
			/* Timeout elapsed */
			continue;

		packet_handler(NULL, header, pkt_data);
	}

	pcap_close(adhandle);

	return ;
}

void CNetManager::packet_handler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	struct tm ltime;
	char timestr[16];
	ip_header *ih;
	udp_header *uh;
	u_int ip_len;
	u_short sport, dport;
	time_t local_tv_sec;
	port_net_t t;

	/* retireve the position of the ip header */
	ih = (ip_header *)(pkt_data +
		14); //length of ethernet header

	//printf("proto: %d ", ih->proto);

	/* convert the timestamp to readable format */
	local_tv_sec = header->ts.tv_sec;
	localtime_s(&ltime, &local_tv_sec);
	strftime(timestr, sizeof timestr, "%H:%M:%S", &ltime);

	/* print timestamp and length of the packet */
	//printf("%s.%.6d len:%d ", timestr, header->ts.tv_usec, header->len);

	/* retireve the position of the udp header */
	ip_len = (ih->ver_ihl & 0xf) * 4;
	uh = (udp_header *)((u_char*)ih + ip_len);

	/* convert from network byte order to host byte order */
	sport = ntohs(uh->sport);//source port
	dport = ntohs(uh->dport);//dest port

	switch (ih->proto)
	{
	case IPPROTO_TCP:
		break;
	case IPPROTO_UDP:
		break;
	default:
		break;
	}

	/* print ip addresses and udp ports */
	//printf("%d.%d.%d.%d.%d -> %d.%d.%d.%d.%d\n",
	//	ih->saddr.byte1,
	//	ih->saddr.byte2,
	//	ih->saddr.byte3,
	//	ih->saddr.byte4,
	//	sport,
	//	ih->daddr.byte1,
	//	ih->daddr.byte2,
	//	ih->daddr.byte3,
	//	ih->daddr.byte4,
	//	dport);

	t.srcPort = sport;
	t.destPort = dport;
	t.source = ih->saddr;
	t.dest = ih->daddr;
	t.tm = local_tv_sec;
	t.len = header->len;

	bool isSend = (m_srcIP[0] == ih->saddr.byte1 && m_srcIP[1] == ih->saddr.byte2 &&
		m_srcIP[2] == ih->saddr.byte3 && m_srcIP[3] == ih->saddr.byte4);
	bool isRecv = (m_srcIP[0] == ih->daddr.byte1 && m_srcIP[1] == ih->daddr.byte2 &&
		m_srcIP[2] == ih->daddr.byte3 && m_srcIP[3] == ih->daddr.byte4);
	if (isSend || isRecv)
	{
		statisticNet(t, isSend);
	}
}

void CNetManager::statisticNet(const port_net_t& t, bool isSend)
{
	EnterCriticalSection(&m_lock);

	u_int port = isSend ? t.srcPort : t.destPort;
	auto it = m_nets.stream.port_pid.find(port);
	if (it != m_nets.stream.port_pid.end())
	{
		auto itPid = m_nets.stream.pid_len.find(it->second.pid);
		if (itPid != m_nets.stream.pid_len.end())
		{
			if (isSend)
				itPid->second.send += t.len;
			else
				itPid->second.recv += t.len;
		}
	}
	else
	{
		//printf(" send/recv %d len:%d[%d]\n", port, t.len, isSend);
		__GetUdpConnect(port, t.len);
		__GetTcpConnect(port, t.len);
	}

	LeaveCriticalSection(&m_lock);
}

//#include <Iprtrmib.h>
#include <IPHlpApi.h>

#pragma comment(lib, "Iphlpapi.lib")

bool CNetManager::__GetUdpConnect(int targetPort, int nLen, bool bSend)
{
	PMIB_UDPTABLE_OWNER_PID pUdpTable(NULL);
	DWORD dwSize(0);
	if (ERROR_INSUFFICIENT_BUFFER == GetExtendedUdpTable(pUdpTable, &dwSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0))
	{

	}

	pUdpTable = (MIB_UDPTABLE_OWNER_PID *)new char[dwSize];//重新分配缓冲区

	if (GetExtendedUdpTable(pUdpTable, &dwSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0) != NO_ERROR)
	{
		delete pUdpTable;
		return false;
	}

	int nNum = (int)pUdpTable->dwNumEntries; //TCP连接的数目
	for (int i = 0; i<nNum; i++)
	{
		auto pid = pUdpTable->table[i].dwOwningPid;
		if (!pid)
			continue;

		std::string sLocalIp = inet_ntoa(*(in_addr*)& pUdpTable->table[i].dwLocalAddr);
		auto nLocalPort = htons((u_short)pUdpTable->table[i].dwLocalPort);

		if (targetPort)
		{
			if (targetPort != nLocalPort)
				continue;

			m_nets.stream.port_pid[nLocalPort] = net_pid_t(pid);
			if (bSend)
			{
				m_nets.stream.pid_len[pid].send = nLen;
			}
			else
			{
				m_nets.stream.pid_len[pid].recv = nLen;
			}

			__GetPidInfo(pid);
			//printf("      %d len:%d ok\n", nLocalPort, nLen);
			break;
		}
		else
		{
			m_nets.stream.port_pid[nLocalPort] = net_pid_t((u_long)pUdpTable->table[i].dwOwningPid);
			m_nets.stream.pid_len[pUdpTable->table[i].dwOwningPid] = net_pid_len_t();
		}
	}

	delete pUdpTable;
	return true;
}

bool CNetManager::__GetTcpConnect(int targetPort, int nLen, bool bSend)
{
	PMIB_TCPTABLE_OWNER_PID pTcpTable(NULL);
	DWORD dwSize(0);
	if (ERROR_INSUFFICIENT_BUFFER == GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0))
	{

	}

	pTcpTable = (MIB_TCPTABLE_OWNER_PID *)new char[dwSize];//重新分配缓冲区

	if (GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0) != NO_ERROR)
	{
		delete pTcpTable;
		return false;
	}

	int nNum = (int)pTcpTable->dwNumEntries; //TCP连接的数目
	for (int i = 0; i<nNum; i++)
	{
		auto pid = pTcpTable->table[i].dwOwningPid;
		if (!pid)
			continue;
		std::string sLocalIp = inet_ntoa(*(in_addr*)& pTcpTable->table[i].dwLocalAddr);
		std::string sRemoteIp = inet_ntoa(*(in_addr*)& pTcpTable->table[i].dwRemoteAddr);

		auto nLocalPort = htons((u_short)pTcpTable->table[i].dwLocalPort);
		if (targetPort && targetPort == nLocalPort)
		{
			__GetPidInfo(pid);
			m_nets.stream.port_pid[nLocalPort] = net_pid_t(pid);
			if (bSend)
			{
				m_nets.stream.pid_len[pid].send = nLen;
			}
			else
			{
				m_nets.stream.pid_len[pid].recv = nLen;
			}
			
			//printf("      %d len:%d ok\n", nLocalPort, nLen);
			break;
		}
		
		m_nets.stream.port_pid[nLocalPort] = net_pid_t((u_long)pTcpTable->table[i].dwOwningPid);
		m_nets.stream.pid_len[pTcpTable->table[i].dwOwningPid] = net_pid_len_t();
	}

	delete pTcpTable;
	return true;
}

void CNetManager::__GetPidInfo(int targetPid)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, targetPid);
	PROCESSENTRY32 pe;
	
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapshot, &pe)) 
	{
		CloseHandle(hSnapshot);
		return ;
	}

	while (Process32Next(hSnapshot, &pe)) 
	{
		if (targetPid) 
		{
			if (pe.th32ProcessID == targetPid)
			{
				m_nets.pid_name[pe.th32ProcessID] = pe.szExeFile;
				m_nets.stream.pid_len[pe.th32ProcessID] = net_pid_len_t();
				CloseHandle(hSnapshot);
				return;
			}
		}
		else
		{
			m_nets.pid_name[pe.th32ProcessID] = pe.szExeFile;
			m_nets.stream.pid_len[pe.th32ProcessID] = net_pid_len_t();
		}
	}

	CloseHandle(hSnapshot);
	return ;
}

void CNetManager::statThreadFunc()
{
	do
	{
		{
			if (m_bStop)
				break;
			EnterCriticalSection(&m_lock);

			m_stat = _monotor_info_t();
			for (auto it : m_nets.stream.pid_len)
			{
				//if (it.second.recv <= 0 && it.second.send <= 0)
				//	continue;

				m_stat.total.up += it.second.send;
				m_stat.total.down += it.second.recv;

				_stat_net_info_t pidInfo;
				pidInfo.pid = it.first;
				pidInfo.stream.up = it.second.send;
				pidInfo.stream.down = it.second.recv;
				auto nameIt = m_nets.pid_name.find(it.first);
				if (nameIt != m_nets.pid_name.end())
					pidInfo.name = nameIt->second;

				m_stat.details.push_back(pidInfo);
			}

			m_nets.stream.clear();
			__GetUdpConnect();
			__GetTcpConnect();

			m_nets.pid_name.clear();
			__GetPidInfo();

			LeaveCriticalSection(&m_lock);
		}
		
		Sleep(1000);
	} while (true);
}

_monotor_info_t CNetManager::getStatInfo()
{
	_monotor_info_t infos;
	EnterCriticalSection(&m_lock);
	infos = m_stat;
	LeaveCriticalSection(&m_lock);
	return std::move(infos);
}