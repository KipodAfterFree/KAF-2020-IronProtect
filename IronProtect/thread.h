#pragma once

#include <windows.h>
#include <stdexcept>
#include <system_error>

class Thread final
{
public:
	explicit Thread(HANDLE handle);
	~Thread();

public:
	void suspend();
	void resume();
	void terminate();
	void wait();

public:
	int get_tid() const;

public:
	CONTEXT get_context() const;
	void set_context(const CONTEXT& context);

public:
	explicit operator HANDLE() const;

private:
	HANDLE _handle;
};
