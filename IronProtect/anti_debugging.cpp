#include "anti_debugging.h"

#include <windows.h>

void anti_debugging::check_for_debugger_1()
{
	if (::IsDebuggerPresent())
	{
		throw;
	}
}

void anti_debugging::check_for_debugger_2()
{
	bool debugger_present{ };
	auto success = ::CheckRemoteDebuggerPresent(::GetCurrentProcess(), reinterpret_cast<PBOOL>(&debugger_present));

	if (success && debugger_present)
	{
		throw;
	}
}

void anti_debugging::check_for_debugger_3()
{
	CONTEXT context = { 0 };
	context.ContextFlags = CONTEXT_DEBUG_REGISTERS;

	auto success = ::GetThreadContext(::GetCurrentThread(), &context);

	if (success && (context.Dr0 || context.Dr1 || context.Dr2 || context.Dr3 || context.Dr6 || context.Dr7))
	{
		throw;
	}
}

void anti_debugging::check_for_debugger_4(void* function)
{
	if (*reinterpret_cast<byte*>(function) == 0xCC)
	{
		throw;
	}
}
