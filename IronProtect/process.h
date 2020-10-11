#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <cinttypes>
#include <memory>

#include "thread.h"

class Process final
{
public:
	enum class Protection { N = 0x01, R = 0x02, RW = 0x04, X = 0x10, RX = 0x20, RWX = 0x40 };

public:
	explicit Process(const std::string& command_line, unsigned int creation_flags);
	~Process();

public:
	void wait_for_initialization();

public:
	void suspend();
	void resume();
	void terminate();
	void wait();

public:
	void* alloc(size_t size, Protection protection);
	void free(void* address);

public:
	void protect(void* address, size_t size, Protection protection);

public:
	std::vector<byte> read(const void* address, size_t size) const;
	void write(void* address, const std::vector<byte>& buffer);

public:
	std::shared_ptr<Thread> get_main_thread() { return _main_thread; }
	const std::shared_ptr<Thread> get_main_thread() const { return _main_thread; }

public:
	explicit operator HANDLE() const;

private:
	HANDLE _handle;
	std::shared_ptr<Thread> _main_thread;
};
