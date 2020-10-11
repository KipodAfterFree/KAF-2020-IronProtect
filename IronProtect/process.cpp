#include "process.h"
#include "anti_debugging.h"

#include <system_error>
#include "process.h"

Process::Process(const std::string& command_line, unsigned int creation_flags)
	: _handle{ nullptr }
{
	STARTUPINFOA si = { 0 };
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi = { 0 };
	
	anti_debugging::check_for_debugger_4(::CreateProcessA);
	auto success = ::CreateProcessA(
		nullptr,
		const_cast<char*>(command_line.c_str()),
		nullptr,
		nullptr,
		false,
		creation_flags,
		nullptr,
		nullptr,
		&si,
		&pi
	);

	if (!success)
	{
		throw;
	}

	_handle = pi.hProcess;
	_main_thread = std::make_shared<Thread>(pi.hThread);
}

Process::~Process()
{
	try {
		anti_debugging::check_for_debugger_4(::CloseHandle);
		::CloseHandle(_handle);
	}
	catch (...) {}
}

void Process::wait_for_initialization()
{
	anti_debugging::check_for_debugger_4(::WaitForInputIdle);
	auto result = ::WaitForInputIdle(_handle, INFINITE);

	if (result == WAIT_FAILED)
	{
		throw;
	}
}

void Process::suspend()
{
	_main_thread->suspend();
}

void Process::resume()
{
	_main_thread->resume();
}

void Process::terminate()
{
	anti_debugging::check_for_debugger_4(::TerminateProcess);
	auto success = ::TerminateProcess(_handle, 0);

	if (!success)
	{
		throw;
	}
}

void Process::wait()
{
	anti_debugging::check_for_debugger_4(::WaitForSingleObject);
	auto result = ::WaitForSingleObject(_handle, INFINITE);

	if (result == WAIT_FAILED)
	{
		throw;
	}
}

void* Process::alloc(size_t size, Protection protection)
{
	anti_debugging::check_for_debugger_4(::VirtualAllocEx);
	auto address = ::VirtualAllocEx(
		_handle,
		nullptr,
		size,
		MEM_COMMIT | MEM_RESERVE,
		static_cast<DWORD>(protection)
	);

	if (!address)
	{
		throw;
	}

	return address;
}

void Process::free(void* address)
{
	anti_debugging::check_for_debugger_4(::VirtualFreeEx);
	auto success = ::VirtualFreeEx(
		_handle,
		address,
		0,
		MEM_RELEASE
	);

	if (!success)
	{
		throw;
	}
}

void Process::protect(void* address, size_t size, Protection protection)
{
	DWORD old_protection{ };

	anti_debugging::check_for_debugger_4(::VirtualProtectEx);
	auto success = ::VirtualProtectEx(
		_handle,
		address,
		size,
		static_cast<DWORD>(protection),
		&old_protection
	);

	if (!success)
	{
		throw;
	}
}

std::vector<byte> Process::read(const void* address, size_t size) const
{
	std::vector<byte> buffer(size);
	size_t bytes_read{ };

	anti_debugging::check_for_debugger_4(::ReadProcessMemory);
	auto success = ::ReadProcessMemory(
		_handle,
		address,
		buffer.data(),
		buffer.size(),
		&bytes_read
	);

	if (!success || size != bytes_read)
	{
		throw;
	}

	return buffer;
}

void Process::write(void* address, const std::vector<byte>& buffer)
{
	size_t bytes_written{ };

	anti_debugging::check_for_debugger_4(::WriteProcessMemory);
	auto success = ::WriteProcessMemory(
		_handle,
		address,
		buffer.data(),
		buffer.size(),
		&bytes_written
	);

	if (!success || buffer.size() != bytes_written)
	{
		throw;
	}
}

Process::operator HANDLE() const
{
	return _handle;
}