#include "virtual_machine.h"
#include "anti_debugging.h"

#include <iostream>

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " flag." << std::endl;
		return 1;
	}

	try
	{
		anti_debugging::check_for_debugger_3();

		VirtualMachine vm;
		vm.run(argv[1]);

		std::cout << "gg my man :)" << std::endl;
	}
	catch (...)
	{
		std::cout << "wtf???" << std::endl;
	}

	return 0;
}