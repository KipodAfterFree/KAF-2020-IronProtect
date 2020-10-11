#include <iostream>

bool check_flag(const std::string& flag)
{
	return flag == "KAF{1r0n_s0n_f0r_th3_w1n}";
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout << "Usage: " << argv[0] << " flag." << std::endl;
		return 1;
	}

	if (!check_flag(argv[1]))
	{
		return 1;
	}

	return 0;
}