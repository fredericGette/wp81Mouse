#include "pch.h"

Win32Api win32Api;

// Debug helper
void debugService(CHAR* format, ...)
{
	va_list args;
	va_start(args, format);

	CHAR buffer[1000];
	_vsnprintf_s(buffer, sizeof(buffer), format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

int startService()
{
	int result = EXIT_FAILURE;
	SC_HANDLE hSCM = NULL;
	SC_HANDLE hService = NULL;
	const char* serviceName = "wp81controldevice";

	// Open a handle to the SCM database
	hSCM = win32Api.OpenSCManagerA(
		NULL,                 // Local computer
		NULL,                 // ServicesActive database
		SC_MANAGER_CONNECT);  // Standard access rights

	if (hSCM == NULL) {
		debugService("OpenSCManagerA failed! 0x%08X\n", GetLastError());
		goto exit;
	}

	// Open a handle to the service
	hService = win32Api.OpenServiceA(
		hSCM,              // SCM database
		serviceName,       // Name of service
		SERVICE_START);    // Right to start the service
	if (hService == NULL) {
		debugService("OpenServiceA failed! 0x%08X\n", GetLastError());
		win32Api.CloseServiceHandle(hSCM);
		goto exit;
	}

	// Start the service
	if (win32Api.StartServiceA(
		hService,     // Handle to service
		0,            // No arguments
		NULL))        // No arguments
	{
		debugService("Service '%s' started successfully.\n", serviceName);
	}
	else
	{
		DWORD lastError = GetLastError();
		if (lastError == ERROR_SERVICE_ALREADY_RUNNING)
		{
			debugService("Service '%s' is already started.\n", serviceName);
		}
		else
		{
			debugService("StartServiceA failed! 0x%08X\n", GetLastError());
			win32Api.CloseServiceHandle(hService);
			win32Api.CloseServiceHandle(hSCM);
			goto exit;
		}
	}

	// Close handles
	win32Api.CloseServiceHandle(hService);
	win32Api.CloseServiceHandle(hSCM);

	result = EXIT_SUCCESS;
exit:
	return result;
}