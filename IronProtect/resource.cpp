#include "resource.h"
#include "anti_debugging.h"

#include <system_error>

std::vector<byte> Resource::get_resource_data(int res_id, void (*unpack_cb)(std::vector<byte>&))
{
	Resource res{ res_id };
	unpack_cb(res._data);
	return res.get_data();
}

void Resource::dump_resource_to_file(int res_id, const std::string& path, void (*unpack_cb)(std::vector<byte>&))
{
	Resource res{ res_id };
	unpack_cb(res._data);
	res.dump_to_file(path);
}

Resource::Resource(int id)
	: _data{ }
{
	anti_debugging::check_for_debugger_4(::FindResource);
	auto resource = ::FindResource(
		nullptr,
		MAKEINTRESOURCE(id),
		RT_RCDATA
	);

	if (!resource)
	{
		throw;
	}

	anti_debugging::check_for_debugger_4(::LoadResource);
	auto global = ::LoadResource(
		nullptr,
		resource
	);

	if (!global)
	{
		throw;
	}

	anti_debugging::check_for_debugger_1();

	anti_debugging::check_for_debugger_4(::LockResource);
	auto data = ::LockResource(
		global
	);

	if (!data)
	{
		throw;
	}

	anti_debugging::check_for_debugger_4(::SizeofResource);
	auto size = ::SizeofResource(
		nullptr,
		resource
	);

	if (!size)
	{
		throw;
	}

	_data.resize(size);
	std::memcpy(_data.data(), data, size);
}

std::vector<byte> Resource::get_data() const
{
	return _data;
}

void Resource::dump_to_file(const std::string& path) const
{
	anti_debugging::check_for_debugger_4(::CreateFileA);
	auto file_handle = ::CreateFileA(
		path.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		0,
		nullptr
	);

	if (file_handle == INVALID_HANDLE_VALUE)
	{
		throw;
	}

	int bytes_written{ };

	anti_debugging::check_for_debugger_4(::WriteFile);
	auto success = ::WriteFile(
		file_handle,
		_data.data(),
		static_cast<DWORD>(_data.size()),
		reinterpret_cast<DWORD*>(&bytes_written),
		nullptr
	);

	if (!success)
	{
		throw;
	}

	anti_debugging::check_for_debugger_4(::CloseHandle);
	::CloseHandle(file_handle);
}
