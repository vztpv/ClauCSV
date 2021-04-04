
#ifndef CLAU_CSV_H
#define CLAU_CSV_H

#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <set>


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
		const int DelimiterType = 1;
		const int LineType = 2;
		const int EodType = 3;
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

	class Utility
	{
	public:
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
			
			if (length < 0) {
				length = 0;
			}

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
				x += 6; // 0110
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
	};
	class Scanner
	{
	private:
		static void _Scanning(const char* text, long long num, const long long length,
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
							token_arr[num + token_arr_count] = Utility::Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}

						token_first = i;
						token_last = i;

						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Utility::Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case '\r':
						token_last = i - 1; // i - 1?
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Utility::Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;
						break;
					case '\n':
						token_last = i - 1; // i - 1?
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Utility::Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Utility::Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case '\0':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Utility::Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Utility::Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					case '#':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Utility::Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Utility::Get(i + num, 1, ch, option);
							token_arr_count++;
						}

						break;
					case ',':
						token_last = i - 1;
						if (token_last - token_first + 1 > 0) {
							token_arr[num + token_arr_count] = Utility::Get(token_first + num, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;
						}
						token_first = i + 1;
						token_last = i + 1;

						{//
							token_arr[num + token_arr_count] = 1;
							token_arr[num + token_arr_count] += Utility::Get(i + num, 1, ch, option);
							token_arr_count++;
						}
						break;
					}

				}

				if (length - 1 - token_first + 1 > 0) {
					token_arr[num + token_arr_count] = Utility::Get(token_first + num, length - 1 - token_first + 1, text[token_first], option);
					token_arr_count++;
				}
				token_arr_size = token_arr_count;
			}

			{
				_token_arr_size = token_arr_size;
			}
		}



		static void ScanningNew(const char* text, const long long length, const int thr_num,
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
						if ('\n' == text[x] || '\0' == text[x] ||
							option.Delimiter == text[x]) {
							start[i] = x;
							break;
						}
					}
				}
				for (int i = 0; i < thr_num - 1; ++i) {
					last[i] = start[i + 1];
					for (long long x = last[i]; x <= length; ++x) {
						if ('\n' == text[x] || '\0' == text[x] ||
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

					const long long len = Utility::GetLength(tokens[i]);
					const char ch = text[Utility::GetIdx(tokens[i])];
					const long long idx = Utility::GetIdx(tokens[i]);

					{
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
							if ('\"' == ch && Utility::GetIdx(tokens[inner_qouted_start]) + 1 == Utility::GetIdx(tokens[i])) {
								state = 1;
							}
							else {
								state = 0;

								{
									long long idx = Utility::GetIdx(tokens[qouted_start]);

									long long len = Utility::GetIdx(tokens[inner_qouted_start]) - idx + 1;

									tokens[real_token_arr_count] = Utility::Get(idx, len, text[idx], option);
									real_token_arr_count++;
								}

								--j;
								continue;
							}
						}
						else if (3 == state && ('\n' == ch || '\0' == ch)) { 
							state = 0;
						}
					}
					
					if (0 == state) { 
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


		static void Scanning(const char* text, const long long length,
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
								token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}

							state = 3;
						}
						else if ('\"' == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}

							token_first = i;
							token_last = i;

							state = 1;
						}
						else if ('\r' == ch) {
							token_last = i - 1; // i - 1? text\r\n
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i + 1;
							token_last = i + 1;

						}
						else if ('\n' == ch) {
							token_last = i - 1; // i - 1? text\r\n
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i + 1;
							token_last = i + 1;

							token_arr[token_arr_count] = Utility::Get(i, 1, ch, option);
							token_arr_count++;
						}
						else if ('\0' == ch) {
							token_last = i - 1; // i - 1? text\r\n
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}
							token_first = i + 1;
							token_last = i + 1;

							token_arr[token_arr_count] = Utility::Get(i, 1, ch, option);
							token_arr_count++;
						}
						else if (option.Delimiter == ch) {
							token_last = i - 1;
							if (token_last - token_first + 1 > 0) {
								token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
								token_arr_count++;
							}

							token_first = i;
							token_last = i;

							token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i + 1;
							token_last = i + 1;
						}
					}
					else if (1 == state) {
						if ('\"' == ch) {
							state = 2;
						}
					}
					else if (2 == state) {
						if ('\"' == ch) {
							state = 1;
						}
						else {
							token_last = i - 1;

							token_arr[token_arr_count] = Utility::Get(token_first, token_last - token_first + 1, text[token_first], option);
							token_arr_count++;

							token_first = i;
							token_last = i;

							state = 0;
							--i;

						}
					}
					else if (3 == state) {
						if ('\n' == ch || '\0' == ch) {
							state = 0;

							token_first = i + 1;
							token_last = i + 1;

							token_arr[token_arr_count] = Utility::Get(i, 1, ch, option);
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


		static std::pair<bool, int> Scan(FILE* inFile, const char* buf, long long buf_size, const int num, const wiz::LoadDataOption& option, int thr_num,
			const char*& _buffer, long long* _buffer_len, long long*& _token_arr, long long* _token_arr_len)
		{
			if (inFile == nullptr && buf == nullptr) {
				return { false, 0 };
			}

			long long* arr_count = nullptr; //
			long long arr_count_size = 0;

			std::string temp;
			const char* buffer = nullptr;
			long long file_length;

			{
				if (inFile) {
					fseek(inFile, 0, SEEK_END);
					unsigned long long length = ftell(inFile);
					fseek(inFile, 0, SEEK_SET);

					Utility::BomType x = Utility::ReadBom(inFile);

					//	wiz::Out << "length " << length << "\n";
					if (x == Utility::BomType::UTF_8) {
						length = length - 3;
					}

					file_length = length;
					buffer = new char[file_length + 1]; // 

					//int a = clock();
					// read data as a block:
					fread(const_cast<char*>(buffer), sizeof(char), file_length, inFile);
					//int b = clock();
					//std::cout << b - a << " " << file_length <<"\n";

					const_cast<char*>(buffer)[file_length] = '\0';
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
		const char* buf = nullptr;
		long long buf_size = 0;
	public:
		int Num = 0;
	public:
		explicit Scanner(FILE* inFile)
		{
			pInFile = inFile;
			Num = 1;
		}

		explicit Scanner(const char* buffer, long long size) {
			buf = buffer;
			buf_size = size;
		}
	public:
		bool operator() (const wiz::LoadDataOption& option, int thr_num, const char*& buffer, long long* buffer_len, long long*& token_arr, long long* token_arr_len)
		{
			bool x = Scan(pInFile, buf, buf_size, Num, option, thr_num, buffer, buffer_len, token_arr, token_arr_len).second > 0;

			return x;
		}
	};


	template <class T>
	class Node {
	public:
		std::vector<T> element;


		Node<T>() {
			//
		}

		Node<T>(const Node<T>& other) {
			element = other.element;
		}
		Node<T>(Node<T>&& other) {
			element = std::move(other.element);
		}

		void operator=(const Node<T>& other) {
			element = other.element;
		}
		void operator=(const Node<T>&& other) {
			element = std::move(other.element);
		}

		size_t Size() const {
			return element.size();
		}

		bool Empty() const {
			return element.empty();
		}
	};

	template <class T>
	class Tree {
	private:
		std::vector<Node<T>> data;
		std::vector<T> header;
	
	public:
		// access data.
			// useHeader?
			// useIdx?


		Node<T>& GetData(size_t idx) {
			return data[idx];
		}

		const Node<T>& GetData(size_t idx) const {
			return data[idx];
		}
		void Change() { // warnning.
			for (size_t i = 0; i < data[0].element.size(); ++i) {
				header.push_back(data[0].element[i]);
			}
			data.clear();
		}

		void PushBack(const T& str) {
			data.back().element.push_back(str);
		}
		void PushBack(const char* str, const long long start, const long long len) {
			data.back().element.emplace_back(str + start, len);
		}

		void NewLine() {
			data.push_back(Node<T>());
		}
		// initHeader.
		// etc...
		size_t Size() const {
			return data.size();
		}

		void Reserve(size_t sz) {
			data.back().element.reserve(sz);
		}

		void Reserve2(size_t sz) {
			data.reserve(sz);
		}

		Node<T>&& GetMovedData(size_t idx) {
			return std::move(data[idx]);
		}
		
		void AddData(Node<T>&& node) {
			if (!this->data.empty() && this->data.back().Empty()) {
				this->data.back().element = std::move(node.element);
			}
			else {
				this->data.push_back(std::move(node));
			}
		}
	};

	template <class T>
	class Parser {
	private:
		static void Merge(Tree<T>* tree, Tree<T>* element) {
			for (size_t i = 0; i < element->Size(); ++i) {
				tree->AddData(element->GetMovedData(i));
			}
		}

		static void _Parse(const char* text, long long num, long long* token_arr, long long token_arr_size, int start_state, int last_state, wiz::LoadDataOption option, Tree<T>* output) {
			int state = start_state;

			long long idx = Utility::GetIdx(token_arr[0]);

			std::vector<long long> vec;

			output->NewLine();

			bool is_first = true;
			long long count = 0;

			for (long long i = 0; i < token_arr_size; ++i) {
				if (is_first) {
					count++;
				}
				
				if (option.DelimiterType == Utility::GetType(token_arr[i])) {
				
					const long long diff = Utility::GetIdx(token_arr[i]) - idx;

					if (diff >= 0) {
						if (diff > 0) {
							vec.push_back(Utility::Get(idx, diff, ' ', option));
						}
						else {
							vec.push_back(0);
						}
					}

					idx = Utility::GetIdx(token_arr[i + 1]);
				}
				else if (option.LineType == Utility::GetType(token_arr[i])) {
					if (is_first) {
						if (count > 0) {
							output->Reserve2(token_arr_size / count);
						}
						is_first = false;
					}

					const long long diff = Utility::GetIdx(token_arr[i]) - 1 - idx;

					if (diff >= 0) {
						if (diff > 0) {
							vec.push_back(Utility::Get(idx, diff, ' ', option));
						}
						else {
							vec.push_back(0);
						}
					}

					output->Reserve(vec.size());

					for (size_t i = 0; i < vec.size(); ++i) {
						output->PushBack(text, Utility::GetIdx(vec[i]), Utility::GetLength(vec[i]));
					}
					vec.clear();

					idx = Utility::GetIdx(token_arr[i + 1]);
					output->NewLine();
				}
				else if (option.EodType == Utility::GetType(token_arr[i])) {
					const long long diff = Utility::GetIdx(token_arr[i]) - idx;

					if (diff >= 0) {
						if (diff > 0) {
							output->PushBack(std::string(text + idx, diff));
						}
						else {
							output->PushBack(std::string());
						}
					}
				}
			}

			// processing errors.

			if (state != last_state) {
				std::cout << "error in _Parser\n";
			}
		}

		static long long FindDivisionPlace(const char* buffer, const long long* token_arr, long long start, long long last, const wiz::LoadDataOption& option)
		{
			for (long long a = last; a >= start; --a) {
				long long len = Utility::GetLength(token_arr[a]);
				long long val = Utility::GetType(token_arr[a]);

				if (option.LineType == val) {
					return a;
				}
			}
			return -1;
		}

		static Tree<T> Parse(wiz::Scanner& scanner, const char* text, long long* token_arr, long long token_arr_len, int numOfParse, wiz::LoadDataOption option, bool useHeader = false) {
			Tree<T> global;
			const int pivot_num = numOfParse - 1;

			// Header chk..
			if (useHeader) {
				size_t idx = 0;
				for (; idx < token_arr_len; ++idx) {
					if (Utility::GetType(token_arr[idx]) == option.LineType || Utility::GetType(token_arr[idx]) == option.EodType) {
						break;
					}
				}
				_Parse(text, 0, token_arr, idx, 0, 0, option, &global);

				global.Change(); //

				token_arr = token_arr + idx;
				token_arr_len = token_arr_len - idx;
			}

			// Data Division.. with \n Token.
			std::set<long long> _pivots;
			std::vector<long long> pivots;
			const long long num = token_arr_len; //

			if (pivot_num > 0) {
				std::vector<long long> pivot;
				pivots.reserve(pivot_num);
				pivot.reserve(pivot_num);

				for (int i = 0; i < pivot_num; ++i) {
					pivot.push_back(FindDivisionPlace(text, token_arr, (num / (pivot_num + 1)) * (i), (num / (pivot_num + 1)) * (i + 1) - 1, option));
				}

				for (int i = 0; i < pivot.size(); ++i) {
					if (pivot[i] != -1) {
						_pivots.insert(pivot[i]);
					}
				}

				for (auto& x : _pivots) {
					pivots.push_back(x);
				}
			}


			// Do Parallel..
			{

				std::vector<Tree<T>> __global(pivots.size() + 1);

				std::vector<std::thread> thr(pivots.size() + 1);

				{
					long long idx = pivots.empty() ? num - 1 : pivots[0];
					long long _token_arr_len = idx - 0 + 1;

					thr[0] = std::thread(_Parse, text, 0, token_arr, _token_arr_len, 0, 0, option, &__global[0]);
				}

				for (int i = 1; i < pivots.size(); ++i) {
					long long _token_arr_len = pivots[i] - (pivots[i - 1] + 1) + 1;

					thr[i] = std::thread(_Parse, text, pivots[i - 1] + 1, token_arr + pivots[i - 1] + 1, _token_arr_len, 0, 0, option, &__global[i]);

				}

				if (pivots.size() >= 1) {
					long long _token_arr_len = num - 1 - (pivots.back() + 1) + 1;

					thr[pivots.size()] = std::thread(_Parse, text, pivots.back() + 1, token_arr + pivots.back() + 1, _token_arr_len, 0, 0, option, &__global[pivots.size()]);
				}

				// wait
				for (int i = 0; i < thr.size(); ++i) {
					thr[i].join();
				}

				long long sum_size = 0;
				for (int i = 0; i < thr.size(); ++i) {
					sum_size += __global[i].Size();
				}

				global.Reserve2(sum_size);


				for (int i = 0; i < thr.size(); ++i) {
					Merge(&global, &__global[i]);
				}
			}

			return global;
		}
	public:
		static Tree<T> ParseFromFile(const std::string& fileName, bool useHeader = false, int numOfScan = 0, int numOfParse = 0) {
			if (numOfScan <= 0) {
				numOfScan = std::thread::hardware_concurrency();
			}
			if (numOfScan <= 0) {
				numOfScan = 1;
			}

			if (numOfParse <= 0) {
				numOfParse = std::thread::hardware_concurrency();
			}
			if (numOfParse <= 0) {
				numOfParse = 1;
			}


			const char* buffer = nullptr;
			long long buffer_len = 0;
			long long* token_arr = nullptr;
			long long token_arr_len = 0;

			{
				FILE* inFile = fopen(fileName.c_str(), "rb");
				if (!inFile || ferror(inFile)) {
					std::cout << "file open error in parser\n";
					return Tree<T>();
				}
				Scanner scanner(inFile);
				if (!scanner(wiz::LoadDataOption(), numOfScan, buffer, &buffer_len, token_arr, &token_arr_len)) {
					fclose(inFile);
					std::cout << "scanner error in parser\n";
					return Tree<T>();
				}
				fclose(inFile);

				if (token_arr_len <= 0) {
					if (buffer) {
						delete[] buffer;
					}
					if (token_arr) {
						delete[] token_arr;
					}
					return Tree<T>();
				}

				auto ret = Parse(scanner, buffer, token_arr, token_arr_len, numOfParse, wiz::LoadDataOption(), useHeader);

				if (token_arr) {
					delete[] token_arr;
				}
				if (buffer) {
					delete[] buffer;
				}

				return ret;
			}
		}
		static Tree<T> ParseFromString(const std::string& str, bool useHeader = false, int numOfScan = 0, int numOfParse = 0) {
			if (numOfScan <= 0) {
				numOfScan = std::thread::hardware_concurrency();
			}
			if (numOfScan <= 0) {
				numOfScan = 1;
			}

			if (numOfParse <= 0) {
				numOfParse = std::thread::hardware_concurrency();
			}
			if (numOfParse <= 0) {
				numOfParse = 1;
			}

			const char* buffer = nullptr;
			long long buffer_len = 0;
			long long* token_arr = nullptr;
			long long token_arr_len = 0;

			{
				Scanner scanner(str.c_str(), str.size());
				if (!scanner(wiz::LoadDataOption(), numOfScan, buffer, &buffer_len, token_arr, &token_arr_len)) {
					std::cout << "fail to scanning in parser\n";
					return Tree<T>();
				}

				if (token_arr_len <= 0) {
					if (token_arr) {
						delete[] token_arr;
					}
					return Tree<T>();
				}

				Tree<T> ret = Parse(scanner, buffer, token_arr, token_arr_len, numOfParse, wiz::LoadDataOption(), useHeader);

				if (token_arr) {
					delete[] token_arr;
				}

				return ret;
			}
		}
	};
}

#endif
