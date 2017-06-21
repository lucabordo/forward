#include "stdafx.h"
#include "CppUnitTest.h"

#include <vector>
#include <string>

#include "forward.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace forward
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(Composition1)
		{
			using namespace forward;
			std::vector<int> v{ 15, 21 };

			auto result = from(v)
				>> where([](int i) { return i < 20; })
				>> to_vector<int>();

			assert(result.size() == 1);
			assert(result[0] == 15);
		}

		TEST_METHOD(Composition2)
		{
			using namespace forward;
			std::vector<int> v{ 15, 21 };

			std::function<int(int)> f;
			decltype(f)::result_type;

			auto result = 
				from(v)
				>> where([](int i) { return i < 20; })
				>> select([](int i) { return i + 100; })
				>> to_vector<int>();

			assert(result.size() == 1);
			assert(result[0] == 115);
		}

		TEST_METHOD(StringSizes)
		{
			using namespace forward;
			std::vector<std::string> v{ "cat", "bunny", "doggy" };

			auto lambda = [](int x) {return x + 1; };

			auto result =
				from(v)
				>> where([](const std::string& s) { return s[0] < 'h'; })
				>> select([](const std::string& s) { return s.size(); })
				>> to_vector<int>();

			assert(result.size() == 3);
//			assert(result[0] == 3);
	//		assert(result[1] == 5);
		//	assert(result[2] == 5);
		}

		// Interating counter-examples
		// from(std::vector<int>{1,2,3})
		// >> select([](int i){return std::make_unique<int>(i); })
	};
}