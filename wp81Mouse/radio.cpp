#include "pch.h"

// See https://github.com/fredericGette/wp81powertool/blob/main/wp81powertool/bluetooth.cpp

// {101C5B9F-C6F7-41C4-815B-69AAC1ECA0A9}
// 0x101C5B9F, 0x41C4C6F7, 0xAA695B81, 0xA9A0ECC1
DEFINE_GUID(CLSID_BtConnectionManager, 0x101C5B9F, 0xC6F7, 0x41C4, 0x81, 0x5B, 0x69, 0xAA, 0xC1, 0xEC, 0xA0, 0xA9);

// IID_IBtRadioController
// 0xBB431756, 0x46C34878, 0x95ECFCA8, 0x4DF52DFF
DEFINE_GUID(IID_IBtRadioController, 0xBB431756, 0x4878, 0x46C3, 0xA8, 0xFC, 0xEC, 0x95, 0xFF, 0x2D, 0xF5, 0x4D);

// IID_IBtConnectionObserver
// 0x83C91970, 0x48E31D3D, 0x6E439F85, 0xFD344FB3
DEFINE_GUID(IID_IBtConnectionObserver, 0x83C91970, 0x1D3D, 0x48E3, 0x85, 0x9F, 0x43, 0x6E, 0xB3, 0x4F, 0x34, 0xFD);

// Implements the interface of the callback to observe the state of the Bluetooth radio 
class ConnectionObserverCallback : public IBtConnectionObserverCallback
{
	LONG m_lRefCount;
	BLUETOOTH_RADIO_STATE m_radioState;
	HANDLE m_hRadioStateChanged;
	bool m_initializationCompleted;

public:
	//Constructor, Destructor
	ConnectionObserverCallback(HANDLE hRadioStateChanged)
	{
		m_lRefCount = 1;
		m_radioState = BRS_UNKNOWN;
		m_hRadioStateChanged = hRadioStateChanged;
		m_initializationCompleted = false;
	}
	~ConnectionObserverCallback() {}

	//IUnknown
	HRESULT __stdcall QueryInterface(REFIID riid, LPVOID *ppvObj)
	{
		if (riid == __uuidof(IUnknown) || riid == __uuidof(IBtConnectionObserverCallback))
		{
			*ppvObj = this;
		}
		else
		{
			*ppvObj = NULL;
			return E_NOINTERFACE;
		}

		AddRef();
		return NOERROR;
	}
	ULONG __stdcall AddRef()
	{
		return InterlockedIncrement(&m_lRefCount);
	}
	ULONG __stdcall Release()
	{
		ULONG  ulCount = InterlockedDecrement(&m_lRefCount);

		if (0 == ulCount)
		{
			delete this;
		}

		return ulCount;
	}

	//IBtConnectionObserverCallback methods
	HRESULT STDMETHODCALLTYPE RadioStateChanged(
		/* [in] */ BLUETOOTH_RADIO_STATE state)
	{
		m_radioState = state;
		SetEvent(m_hRadioStateChanged);

		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE RemoteDeviceChanged(
		/* [in] */ BTH_ADDR btAddr,
		/* [in] */ BLUETOOTH_DEVICE_STATE eState,
		/* [in] */ DWORD dwClassOfDevice,
		/* [in] */ __RPC__in LPCWSTR wszName)
	{
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE ProfileConnectionChanged(
		/* [in] */ BTH_ADDR btAddr,
		/* [in] */ __RPC__in REFGUID guidProfile,
		/* [in] */ BLUETOOTH_CONNECTION_STATE eState)
	{
		return S_OK;
	}
	HRESULT STDMETHODCALLTYPE InitializationComplete(void)
	{
		m_initializationCompleted = true;
		return S_OK;
	}

	// Getter
	BLUETOOTH_RADIO_STATE getRadioState(void)
	{
		return m_radioState;
	}
	bool isInitializationCompleted(void)
	{
		return m_initializationCompleted;
	}
};

// Debug helper
void debugRadio(CHAR* format, ...)
{
	va_list args;
	va_start(args, format);

	CHAR buffer[1000];
	_vsnprintf_s(buffer, sizeof(buffer), format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

int ChangeRadioState(BOOL TurnOn)
{
	int exit_status = EXIT_SUCCESS;

	HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(result))
	{
		debugRadio("CoInitializeEx failed 0x%X\n", result);
		return EXIT_FAILURE;
	}

	// Register a callback to observe the state of the Bluetooth radio.
	///////////////////////////////////////////////////////////////////

	HANDLE hRadioStateChanged = CreateEventW(
		NULL,
		TRUE,	// manually reset
		FALSE,	// initial state: nonsignaled
		L"WP81_RADIO_STATE_CHANGED"
	);

	IBtConnectionObserver* pIBtConnectionObserver;
	result = CoCreateInstance(CLSID_BtConnectionManager, NULL, CLSCTX_INPROC_SERVER,
		IID_IBtConnectionObserver, (void **)&pIBtConnectionObserver);
	if (FAILED(result))
	{
		debugRadio("CoCreateInstance IBtConnectionObserver failed 0x%X\n", result);
		return EXIT_FAILURE;
	}

	ConnectionObserverCallback* pConnectionObserverCallback = new ConnectionObserverCallback(hRadioStateChanged);
	INT registrationHandle;

	pIBtConnectionObserver->RegisterCallback(pConnectionObserverCallback, &registrationHandle);

	// Wait until we received all the messages describing the current Bluetooth state.
	while (!pConnectionObserverCallback->isInitializationCompleted())
	{
		Sleep(100);
	}

	// Change the state of the Bluetooth radio
	//////////////////////////////////////////

	IBtRadioController* pIBtRadioController;
	result = CoCreateInstance(CLSID_BtConnectionManager, NULL, CLSCTX_INPROC_SERVER,
		IID_IBtRadioController, (void **)&pIBtRadioController);
	if (FAILED(result))
	{
		debugRadio("CoCreateInstance IBtRadioController failed 0x%X\n", result);
		return EXIT_FAILURE;
	}

	debugRadio("%s Bluetooth Radio...\n", TurnOn ? "Enable" : "Disable");
	result = pIBtRadioController->EnableBluetoothRadio(TurnOn ? true : false, NULL, NULL);
	if (FAILED(result))
	{
		debugRadio("EnableBluetoothRadio failed 0x%X\n", result);
		return EXIT_FAILURE;
	}
	ResetEvent(hRadioStateChanged);

	// Wait for a stable radio state

	BLUETOOTH_RADIO_STATE radioState = BRS_UNKNOWN;
	while (radioState != BRS_DISABLED && radioState != BRS_ENABLED && radioState != BRS_NO_HARDWARE)
	{
		WaitForSingleObject(hRadioStateChanged, 1000);
		radioState = pConnectionObserverCallback->getRadioState();
	}

	if (TurnOn && radioState == BRS_DISABLED)
	{
		debugRadio("Failed to turn on Bluetooth radio. Radio is still disabled.\n");
		exit_status = EXIT_FAILURE;
	}
	else if (!TurnOn && radioState == BRS_ENABLED)
	{
		debugRadio("Failed to turn off Bluetooth radio. Radio is still enabled.\n");
		exit_status = EXIT_FAILURE;
	}
	else if (radioState == BRS_NO_HARDWARE)
	{
		debugRadio("Failed: No Bluetooth radio.\n");
		exit_status = EXIT_FAILURE;
	}

	// Clean up
	///////////

	result = pIBtConnectionObserver->UnregisterCallback(registrationHandle);
	if (FAILED(result))
	{
		debugRadio("UnregisterCallback failed 0x%X\n", result);
		exit_status = EXIT_FAILURE;
	}

	pConnectionObserverCallback->Release();

	result = pIBtRadioController->SynchronousShutdown(1000);
	if (FAILED(result))
	{
		debugRadio("SynchronousShutdown failed 0x%X\n", result);
		exit_status = EXIT_FAILURE;
	}

	CoUninitialize();

	return exit_status;
}