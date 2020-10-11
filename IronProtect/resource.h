#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <cinttypes>

class Resource final
{
public:
	static std::vector<byte> get_resource_data(int res_id, void (*unpack_cb)(std::vector<byte>&));
	static void dump_resource_to_file(int res_id, const std::string& path, void (*unpack_cb)(std::vector<byte>&));

private:
	explicit Resource(int id);

private:
	std::vector<byte> get_data() const;
	void dump_to_file(const std::string& path) const;

private:
	std::vector<byte> _data;
};
