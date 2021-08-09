#ifndef _EXPORT_H_
#define _EXPORT_H_
#include <Windows.h>

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);
STDAPI DllCanUnloadNow();
STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();
STDAPI DllShowMonitor(BOOL bShow);
STDAPI DLLIsRegster();
STDAPI DllIsShow();
STDAPI DllEnableLog(BOOL bEnableLog);

#endif