
#include <iostream>
#include <string>

#include "clau_csv.h"



int main(void)
{
	char buffer_text[] = "abc,\"\"\"d\"\"\"ab,e,f\r\nx,y,z,w\r\n,,,\r\n";

	wiz::InFileReserver reserver(buffer_text, strlen(buffer_text));

	char* buffer = nullptr;
	
	long long buffer_len;
	long long* token_arr = nullptr;
	long long token_arr_len;


	reserver(wiz::LoadDataOption(), 3, buffer, &buffer_len, token_arr, &token_arr_len);


	for (int i = 0; i < token_arr_len; ++i) {
		wiz::InFileReserver::PrintToken(buffer, token_arr[i]);
		std::cout << "\n";
	}


	if (token_arr) {
		delete[] token_arr;
	}
	return 0;
}
