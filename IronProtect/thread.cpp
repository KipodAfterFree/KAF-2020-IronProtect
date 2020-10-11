#include "thread.h"
#include "anti_debugging.h"

Thread::Thread(HANDLE handle)
	: _handle(handle)
{
	if (!handle)
	{
		throw;
	}
}

Thread::~Thread()
{
	try {
		anti_debugging::check_for_debugger_4(::CloseHandle);
		::CloseHandle(_handle);
	}
	catch (...) {}
}

void Thread::suspend()
{
	anti_debugging::check_for_debugger_4(::SuspendThread);
	auto result = ::SuspendThread(_handle);

	if (result == -1)
	{
		throw;
	}
}

void Thread::resume()
{
	anti_debugging::check_for_debugger_4(::ResumeThread);
	auto result = ::ResumeThread(_handle);

	if (result == -1)
	{
		throw;
	}
}

void Thread::terminate()
{
	anti_debugging::check_for_debugger_4(::TerminateThread);
	auto success = ::TerminateThread(_handle, 0);

	if (!success)
	{
		throw;
	}
}

void Thread::wait()
{
	anti_debugging::check_for_debugger_4(::WaitForSingleObject);
	auto result = ::WaitForSingleObject(_handle, INFINITE);

	if (result == WAIT_FAILED)
	{
		throw;
	}
}

int Thread::get_tid() const
{
	anti_debugging::check_for_debugger_4(::GetThreadId);
	return ::GetThreadId(_handle);
}

CONTEXT Thread::get_context() const
{
	CONTEXT context = { 0 };
	context.ContextFlags = CONTEXT_ALL;

	anti_debugging::check_for_debugger_4(::GetThreadContext);
	auto success = ::GetThreadContext(
		_handle,
		&context
	);

	if (!success)
	{
		throw;
	}

	return context;
}

void Thread::set_context(const CONTEXT& context)
{
	anti_debugging::check_for_debugger_4(::SetThreadContext);
	auto success = ::SetThreadContext(
		_handle,
		&context
	);

	if (!success)
	{
		throw;
	}
}

Thread::operator HANDLE() const
{
	return _handle;
}
