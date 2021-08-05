#include "operator\MonitorOperator.h"

int APIENTRY _tWinMain(_In_ HINSTANCE /*hInstance*/,
	_In_opt_ HINSTANCE /*hPrevInstance*/,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       /*nCmdShow*/)
{
	CMonitorOperator monitor;
	monitor.analyzeCommandLine(lpCmdLine);
	monitor.analyze();

	return 0;
}

//int main(int argc, char* argv[])
//{
//	CMonitorOperator monitor;
//	monitor.analyzeCommandLine(argc, argv);
//
//	if (argc >= 2)
//	{
//		if (!strcmp(argv[1], "-install"))
//		{
//			monitor.install();
//			return 0;
//		}
//		else if (!strcmp(argv[1], "-uninstall"))
//		{
//			monitor.uninstall();
//			return 0;
//		}
//		else if (!strcmp(argv[1], "-help")
//			|| !strcmp(argv[1], "-h")
//			|| !strcmp(argv[1], "/help")
//			|| !strcmp(argv[1], "/h"))
//		{
//			monitor.help();
//			return 0;
//		}
//
//		monitor.help();
//		return 0;
//	}
//	else
//	{
//		monitor.install();
//	}
//
//	return 0;
//}
