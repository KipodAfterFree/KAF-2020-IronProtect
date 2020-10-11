#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <vector>

class VirtualMachine final
{
public:
	explicit VirtualMachine();

public:
	void run(const std::string& flag);

private:
	std::vector<byte> _vm_code;

	std::map<int, int> _opcode_to_vmcall;
	std::map<int, int> _vmcall_to_opcode;
};
