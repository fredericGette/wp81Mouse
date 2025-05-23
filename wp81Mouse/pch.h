﻿//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <collection.h>
#include <ppltasks.h>
#include "App.xaml.h"

#include <windows.h>
#include <stdlib.h>
#include <stdint.h>
#include "aes128.h"

#include "radio.h"
#include <initguid.h>
#include <combaseapi.h>
#include "BtConnectionManager.h"
#include "service.h"

extern "C" {
	WINBASEAPI HMODULE WINAPI LoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
	WINBASEAPI HMODULE WINAPI GetModuleHandleW(LPCWSTR lpModuleName);

	WINBASEAPI HANDLE WINAPI CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
	char *getenv(const char *varname);
	WINBASEAPI BOOL WINAPI DeviceIoControl(HANDLE hDevice, DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize, LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped);
	WINBASEAPI HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);
	WINBASEAPI DWORD WINAPI WaitForMultipleObjectsEx(DWORD nCount, CONST HANDLE * lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable);
	WINBASEAPI VOID WINAPI Sleep(DWORD dwMilliseconds);
	WINBASEAPI HANDLE WINAPI CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
	WINBASEAPI DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
	WINBASEAPI DWORD WINAPI GetTickCount(VOID);
	WINADVAPI SC_HANDLE	WINAPI OpenSCManagerA(LPCSTR lpMachineName, LPCSTR lpDatabaseName, DWORD dwDesiredAccess);
	WINADVAPI BOOL WINAPI CloseServiceHandle(SC_HANDLE hSCObject);
	WINADVAPI SC_HANDLE WINAPI OpenServiceA(SC_HANDLE hSCManager, LPCSTR lpServiceName, DWORD dwDesiredAccess);
	WINADVAPI BOOL WINAPI StartServiceA(SC_HANDLE hService, DWORD dwNumServiceArgs, _In_reads_opt_(dwNumServiceArgs) LPCSTR*lpServiceArgVectors);
}

#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3

#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define CONTROL_DEVICE 0x8000
#define IOCTL_CONTROL_WRITE_HCI CTL_CODE(CONTROL_DEVICE, 0x800, METHOD_NEITHER, FILE_ANY_ACCESS) // 0x80002003
#define IOCTL_CONTROL_READ_HCI	CTL_CODE(CONTROL_DEVICE, 0x801, METHOD_NEITHER, FILE_ANY_ACCESS) // 0x80002007
#define IOCTL_CONTROL_CMD		CTL_CODE(CONTROL_DEVICE, 0x802, METHOD_NEITHER, FILE_ANY_ACCESS) // 0x8000200B

#define IOCTL_BTH_GET_LOCAL_INFO 0x410000

enum ConnectionStatus {
	NOT_CONNECTED,
	STARTING,
	ADVERTISING,
	PAIRING,
	SERVING_ATTRIBUTES,
	SENDING_NOTIFICATIONS,
	STOPPING
};

#include "bluetooth.h"
#include "Win32Api.h"
