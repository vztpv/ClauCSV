

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string>

#include "clau_csv.h"
#include "test.h"

#include <ctime>

int main(void)
{
	char buffer_text[] = "abc,\"\"\"d\"\"\" a b,e,f\r\nx,y,z,w\r\n,,,\r\nd";
	
	for (int i = 0; i < 1; ++i) {
		for (int j = 0; j < 5; ++j) {

			int a = clock();
			rapidcsv::Document doc("test.csv");

			int b = clock();
			std::cout << b - a << "ms\n";

			 a = clock();
			auto tree2 = wiz::Parser<std::string>::ParseFromFile("test.csv", false, 0, 0);
			 b = clock();
			std::cout << b - a << "ms\n";

			std::cout << "\n";
		}
	}

	/*std::cout << "chk\n";
	for (size_t i = 0; i < tree.Size(); ++i) {
		for (size_t j = 0; j < tree.GetData(i).Size(); ++j) {
			std::cout << tree.GetData(i).element[j] << " ";
		}
		std::cout << "\n";
	}*/

	return 0;
}
