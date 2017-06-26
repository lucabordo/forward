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
		
		TEST_METHOD(BasicEnumerationFromVector)
		{
			using namespace forward;
			auto vec = std::vector<int>{ 1, 2, 3, 4 };
			auto list = from(vec);
			std::vector<int> converted = to_vector(list);

			assert(converted.size() == 4);
			assert(converted[0] == 1);
			assert(converted[3] == 4);
		}


		TEST_METHOD(SimpleWhere)
		{
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
			std::vector<std::string> v{ "cat", "bunny", "doggy", "horsey" };

			auto result =
				from(v)
				>> where([](const std::string& s) { return s[0] < 'h'; })
				>> select([](const std::string& s) { return static_cast<int>(s.size()); })
				>> to_vector<int>();

			assert(result.size() == 3);
			assert(result[0] == 3);
			assert(result[1] == 5);
			assert(result[2] == 5);
		}

		TEST_METHOD(UniquePointers)
		{
			using namespace forward;
			std::vector<int> v{ 1, 2, 3, 4, 5 };

			auto result =
				from(v)
				>> select([](int i) {return std::make_unique<int>(i); })
				>> to_vector<std::unique_ptr<int>>();

			assert(result.size() == 5);
			for (int i = 0; i < v.size(); ++i)
				assert(v[i] == *result[i]);
		}

		TEST_METHOD(Pair)
		{
			using namespace forward;
			std::vector<std::string> v{ "cat", "bunny", "doggy", "horsey" };

			auto result = to_vector(
				from(v)
				>> select([](std::string& s) { return std::make_tuple(std::move(s), s.size()); }));

			// We should have a single copy, but the initial collection always intact
			assert(v[0] == "cat");
		}
	};
}