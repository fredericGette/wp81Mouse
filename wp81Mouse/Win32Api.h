#pragma once

#define WIN32API_TOSTRING(x) #x

// Link exported function
#define WIN32API_INIT_PROC(Module, Name)  \
  Name(reinterpret_cast<decltype(&::Name)>( \
      ::GetProcAddress((Module), WIN32API_TOSTRING(Name))))

// Convenientmacro to declare function
#define WIN32API_DEFINE_PROC(Name) const decltype(&::Name) Name

class Win32Api {

private:
	// Returns a base address of KernelBase.dll
	static HMODULE GetKernelBase() {
		return GetBaseAddress(&::DisableThreadLibraryCalls);
	}

	// Returns a base address of the given address
	static HMODULE GetBaseAddress(const void *Address) {
		MEMORY_BASIC_INFORMATION mbi = {};
		if (!::VirtualQuery(Address, &mbi, sizeof(mbi))) {
			return nullptr;
		}
		const auto mz = *reinterpret_cast<WORD *>(mbi.AllocationBase);
		if (mz != IMAGE_DOS_SIGNATURE) {
			return nullptr;
		}
		return reinterpret_cast<HMODULE>(mbi.AllocationBase);
	}

public:
	const HMODULE m_Kernelbase;
	WIN32API_DEFINE_PROC(LoadLibraryExW);
	WIN32API_DEFINE_PROC(GetModuleHandleW);
	const HMODULE m_SecHost;
	WIN32API_DEFINE_PROC(OpenSCManagerA);
	WIN32API_DEFINE_PROC(CloseServiceHandle);
	WIN32API_DEFINE_PROC(OpenServiceA);
	WIN32API_DEFINE_PROC(StartServiceA);

	Win32Api()
		: m_Kernelbase(GetKernelBase()),
		WIN32API_INIT_PROC(m_Kernelbase, LoadLibraryExW),
		WIN32API_INIT_PROC(m_Kernelbase, GetModuleHandleW),
		m_SecHost(GetModuleHandleW(L"SECHOST.DLL")),
		WIN32API_INIT_PROC(m_SecHost, OpenSCManagerA),
		WIN32API_INIT_PROC(m_SecHost, CloseServiceHandle),
		WIN32API_INIT_PROC(m_SecHost, OpenServiceA),
		WIN32API_INIT_PROC(m_SecHost, StartServiceA)
	{};

};