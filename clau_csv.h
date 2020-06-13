
#ifndef CLAU_CSV_H
#define CLAU_CSV_H

#include <thread>
#include <iostream>
#include <string>
#include <vector>


namespace wiz {

	template <typename T>
	inline T pos_1(const T x, const int base = 10)
	{
		if (x >= 0) { return x % base; }// x - ( x / 10 ) * 10; }
		else { return (x / base) * base - x; }
		// -( x - ( (x/10) * 10 ) )
	}


	template <typename T> /// T <- char, int, long, long long...
	std::string toStr(const T x) /// chk!!
	{
		const int base = 10;
		if (base < 2 || base > 16) { return "base is not valid"; }
		T i = x;

		const int INT_SIZE = sizeof(T) << 3; ///*8
		char* temp = new char[INT_SIZE + 1 + 1]; /// 1 NULL, 1 minus
		std::string tempString;
		int k;
		bool isMinus = (i < 0);
		temp[INT_SIZE + 1] = '\0'; //

		for (k = INT_SIZE; k >= 1; k--) {
			T val = pos_1<T>(i, base); /// 0 ~ base-1
									   /// number to ['0'~'9'] or ['A'~'F']
			if (val < 10) { temp[k] = val + '0'; }
			else { temp[k] = val - 10 + 'A'; }

			i /= base;

			if (0 == i) { // 
				k--;
				break;
			}
		}

		if (isMinus) {
			temp[k] = '-';
			tempString = std::string(temp + k);//
		}
		else {
			tempString = std::string(temp + k + 1); //
		}
		delete[] temp;

		return tempString;
	}


	class LoadDataOption // no change!
	{
	public:
		const char LineComment = '#';	// # 
		const char Delimiter = ',';
		const char Line = '\n';
		const char Eod = '\0'; // End of data.
	};

	inline bool isWhitespace(const char ch)
	{
		switch (ch)
		{
		case ' ':
		case '\t':
		case '\r':
		case '\n':
		case '\v':
		case '\f':
			return true;
			break;
		}
		return false;
	}


	inline int Equal(const long long x, const long long y)
	{
		if (x == y) {
			return 0;
		}
		return -1;
	}

	class InFileReserver
	{
	private:
		class BomInfo
		{
		public:
			size_t bom_size;
			char seq[5];
		};

		const static size_t BOM_COUNT = 1;

		enum class BomType { UTF_8, ANSI };

		inline static const BomInfo bomInfo[1] = {
			{ 3, { (char)0xEF, (char)0xBB, (char)0xBF } }
		};

		static BomType ReadBom(FILE* file) {
			char btBom[5] = { 0, };
			size_t readSize = fread(btBom, sizeof(char), 5, file);


			if (0 == readSize) {
				clearerr(file);
				fseek(file, 0, SEEK_SET);

				return BomType::ANSI;
			}

			BomInfo stBom = { 0, };
			BomType type = ReadBom(btBom, readSize, stBom);

			if (type == BomType::ANSI) { // ansi
				clearerr(file);
				fseek(file, 0, SEEK_SET);
				return BomType::ANSI;
			}

			clearerr(file);
			fseek(file, stBom.bom_size * sizeof(char), SEEK_SET);
			return type;
		}

		static BomType ReadBom(const char* contents, size_t length, BomInfo& outInfo) {
			char btBom[5] = { 0, };
			size_t testLength = length < 5 ? length : 5;
			memcpy(btBom, contents, testLength);

			size_t i, j;
			for (i = 0; i < BOM_COUNT; ++i) {
				const BomInfo& bom = bomInfo[i];

				if (bom.bom_size > testLength) {
					continue;
				}

				bool matched = true;

				for (j = 0; j < bom.bom_size; ++j) {
					if (bom.seq[j] == btBom[j]) {
						continue;
					}

					matched = false;
					break;
				}

				if (!matched) {
					continue;
				}

				outInfo = bom;

				return (BomType)i;
			}

			return BomType::ANSI;
		}


		// todo - rename.
		static long long Get(long long position, long long length, char ch, const wiz::LoadDataOption& option) {
			long long x = (position << 32) + (length << 3) + 0;

			if (length != 1) {
				return x;
			}

			if (option.Delimiter == ch) {
				x += 2; // 0010
			}
			else if (option.Line == ch) {
				x += 4; // 0100
			}
			else if (option.Eod == ch) {
				x += 8; // 1000
			}
			return x;
		}

		// todo - public, and Utility class
		static long long GetIdx(long long x) {
			return (x >> 32) & 0x00000000FFFFFFFF;
		}
		static long long GetLength(long long x) {
			return (x & 0x00000000FFFFFFF8) >> 3;
		}
		static long long GetType(long long x) { //to enum or enum class?
			return (x & 6) >> 1;
		}
		static bool IsToken2(long long x) {
			return (x & 1);
		}

	public:
		static void PrintToken(const char* buffer, long long token) {
			std::cout << std::string(buffer + GetIdx(token), GetLength(token));
		}
	private:
		static void _Scanning(char* text, long long num, const long long length,
			long long*& token_arr, long long& _token_arr_size, const LoadDataOption& option) {

			long long token_arr_size = 0;

			{
				int state = 0;

				long long token_first = 0;
				long long token_last = -1;

				long long token_arr_count = 0;

				for (long long i = 0; i < length; i = i + 1) {

					const char ch = text[i];

					switch (ch) {
					case '\"':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}

						token_first = i;
						token_last = i;

						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case '\n':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case '\0':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case '#':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Get(i + num, 1, ch, option);
							token_arr_count++;
						}

						break;
					case ',':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case ' ':
					case '\t':
					case '\r':
					case '\v':
					case '\f':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						break;
					}

				}

				if (length - 1 - token_first + 1 > 0) {
					token_arr[num + token_arr_count] = Get(token_first + num, length - 1 - token_first + 1, text[token_first], option);
					token_arr_count++;
				}
				token_arr_size = token_arr_count;
			}

			{
				_token_arr_size = token_arr_size;
			}
		}



		static void ScanningNew(char* text, const long long length, const int thr_num,
			long long*& _token_arr, long long& _token_arr_size, const LoadDataOption& option)
		{
			std::vector<std::thread> thr(thr_num);
			std::vector<long long> start(thr_num);
			std::vector<long long> last(thr_num);

			{
				start[0] = 0;

				for (int i = 1; i < thr_num; ++i) {
					start[i] = length / thr_num * i;

					for (long long x = start[i]; x <= length; ++x) {
						if (isWhitespace(text[x]) || '\0' == text[x] ||
							option.Delimiter == text[x]) {
							start[i] = x;
							break;
						}
					}
				}
				for (int i = 0; i < thr_num - 1; ++i) {
					last[i] = start[i + 1];
					for (long long x = last[i]; x <= length; ++x) {
						if (isWhitespace(text[x]) || '\0' == text[x] ||
							option.Delimiter == text[x]) {
							last[i] = x;
							break;
						}
					}
				}
				last[thr_num - 1] = length + 1;
			}
			long long real_token_arr_count = 0;

			long long* tokens = new long long[length + 1];
			long long token_count = 0;

			std::vector<long long> token_arr_size(thr_num);

			for (int i = 0; i < thr_num; ++i) {
				thr[i] = std::thread(_Scanning, text + start[i], start[i], last[i] - start[i], std::ref(tokens), std::ref(token_arr_size[i]), option);
			}

			for (int i = 0; i < thr_num; ++i) {
				thr[i].join();
			}

			int state = 0;
			long long qouted_start;
			long long inner_qouted_start;

			for (long long t = 0; t < thr_num; ++t) {
				for (long long j = 0; j < token_arr_size[t]; ++j) {
					const long long i = start[t] + j;

					const long long len = GetLength(tokens[i]);
					const char ch = text[GetIdx(tokens[i])];
					const long long idx = GetIdx(tokens[i]);
					const bool isToken2 = IsToken2(tokens[i]);

					if (isToken2) {
						if (0 == state && '\"' == ch) {
							state = 1;
							qouted_start = i;
						}
						else if (0 == state && option.LineComment == ch) {
							state = 3;
						}
						else if (1 == state && '\"' == ch) {
							state = 2;
							inner_qouted_start = i;
						}
						else if (2 == state) {
							if ('\"' == ch && GetIdx(tokens[inner_qouted_start]) + 1 == GetIdx(tokens[i])) {
								state = 1;
							}
							else {
								state = 0;

								{
									long long idx = GetIdx(tokens[qouted_start]);
									long long len = GetLength(tokens[qouted_start]);

									len = GetIdx(tokens[inner_qouted_start]) - idx + 1;

									tokens[real_token_arr_count] = Get(idx, len, text[idx], option);
									real_token_arr_count++;

									--j;
								}
							}
						}
						else if (3== state && ('\n' == ch || '\0' == ch)) { 
							state = 0;
						}
					}
					
					if (0 == state) { // , \n, \0
						tokens[real_token_arr_count] = tokens[i];
						real_token_arr_count++;
					}
				}
			}

			{
				if (0 != state) {
					std::cout << "[ERRROR] state [" << state << "] is not zero \n";
				}
			}


			{
				_token_arr = tokens;
				_token_arr_size = real_token_arr_count;
			}
		}


		static void Scanning(char* text, const long long length,
			long long*& _token_arr, long long& _token_arr_size, const LoadDataOption& option) {

			long long* token_arr = new long long[length + 1];
			long long token_arr_size = 0;

			{
				int state = 0;

				long long token_first = 0;
				long long token_last = -1;

				long long token_arr_count = 0;


				for (long long i = 0; i <= length; ++i) {
					const char ch = text[i];

					if (0 == state) {
						if (option.LineComment == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}

							state = 3;
						}
						else if ('\"' == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}

							token_first = i;
							token_last = i;

							state = 1;
						}
						else if ('\n' == ch || '\0' == ch) {
							token_last = i - 2; // i - 1? text\r\n
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i + 1;
							token_last = i + 1;

							token_arr[token_arr_count] = Get(i, 1, ch, option);
							token_arr_count++;
						}
						else if (option.Delimiter == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}

							token_first = i;
							token_last = i;

							token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i + 1;
							token_last = i + 1;
						}
					}
					else if (1 == state) {
						if (i < length && '\"' == ch) {
							state = 2;
						}
					}
					else if (2 == state) {
						if ('\"' == ch) {
							state = 1;
						}
						else {
							token_last = i - 1;

							token_arr[token_arr_count] = Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i;
							token_last = i;

							state = 0;
						}
					}
					else if (3 == state) {
						if ('\n' == ch || '\0' == ch) {
							state = 0;

							token_first = i + 1;
							token_last = i + 1;

							token_arr[token_arr_count] = Get(i, 1, ch, option);
							token_arr_count++;
						}
					}
				}

				token_arr_size = token_arr_count;

				if (0 != state) {
					std::cout << "[" << state << "] state is not zero.\n";
				}
			}

			{
				_token_arr = token_arr;
				_token_arr_size = token_arr_size;
			}
		}


		static std::pair<bool, int> Scan(FILE* inFile, char* buf, long long buf_size, const int num, const wiz::LoadDataOption& option, int thr_num,
			char*& _buffer, long long* _buffer_len, long long*& _token_arr, long long* _token_arr_len)
		{
			if (inFile == nullptr && buf == nullptr) {
				return { false, 0 };
			}

			long long* arr_count = nullptr; //
			long long arr_count_size = 0;

			std::string temp;
			char* buffer = nullptr;
			long long file_length;

			{
				if (inFile) {
					fseek(inFile, 0, SEEK_END);
					unsigned long long length = ftell(inFile);
					fseek(inFile, 0, SEEK_SET);

					BomType x = ReadBom(inFile);

					//	wiz::Out << "length " << length << "\n";
					if (x == BomType::UTF_8) {
						length = length - 3;
					}

					file_length = length;
					buffer = new char[file_length + 1]; // 

					//int a = clock();
					// read data as a block:
					fread(buffer, sizeof(char), file_length, inFile);
					//int b = clock();
					//std::cout << b - a << " " << file_length <<"\n";

					buffer[file_length] = '\0';
				}
				else {
					buffer = buf;
					file_length = buf_size;
				}

				{
					//int a = clock();
					long long* token_arr;
					long long token_arr_size;

					if (thr_num == 1) {
						Scanning(buffer, file_length, token_arr, token_arr_size, option);
					}
					else {
						ScanningNew(buffer, file_length, thr_num, token_arr, token_arr_size, option);
					}

					//int b = clock();
				//	std::cout << b - a << "ms\n";
					_buffer = buffer;
					_token_arr = token_arr;
					*_token_arr_len = token_arr_size;
					*_buffer_len = file_length;
				}
			}

			return{ true, 1 };
		}

	private:
		FILE* pInFile = nullptr;
		char* buf = nullptr;
		long long buf_size = 0;
	public:
		int Num = 0;
	public:
		explicit InFileReserver(FILE* inFile)
		{
			pInFile = inFile;
			Num = 1;
		}

		explicit InFileReserver(char* buffer, long long size) {
			buf = buffer;
			buf_size = size;
		}
	public:
		bool operator() (const wiz::LoadDataOption& option, int thr_num, char*& buffer, long long* buffer_len, long long*& token_arr, long long* token_arr_len)
		{
			bool x = Scan(pInFile, buf, buf_size, Num, option, thr_num, buffer, buffer_len, token_arr, token_arr_len).second > 0;

			return x;
		}
	};


}

#endif
