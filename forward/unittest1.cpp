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


		TEST_METHOD(RangeBasic)
		{
			using namespace forward;
			auto r = range(10, 34) >> to_vector<int>();

			assert(r[0] == 10);
			assert(r.back() == 33);
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

		// Tests of the various implications of storing lambdas 
		// by ref, pointer (so that default-constructible), copied or moved.

		TEST_METHOD(LambdaReuse1)
		{
			using namespace forward;
			std::vector<std::string> v{ "cat", "bunny", "doggy", "horsey" };

			auto sizes = from(v)
				>> where([](const auto& s) { return s[0] != 'c'; })
				>> select([](const auto& s) { return s.size(); });

			auto copy1 = to_vector(sizes);
			auto copy2 = to_vector(sizes);

			assert(copy1[0] == copy2[0] && copy2[0] == 5);
			assert(copy1[1] == copy2[1] && copy2[1] == 5);
			assert(copy1[2] == copy2[2] && copy2[2] == 6);
		}

		TEST_METHOD(LambdaReuse2)
		{
			using namespace forward;
			std::vector<std::string> v{ "cat", "bunny", "doggy", "horsey" };

			auto filter = [](const std::string& s) { return s[0] != 'c'; };
			auto mapper = [](const std::string& s) { return s.size(); };

			// Note that the lambdas are moved and probably voided by the expression
			// construction therefore not usable afterwards. That's our assumption.
			auto sizes = from(v) >> where(filter) >> select(mapper) >> to_vector<size_t>();

			assert(sizes[0] == 5);
			assert(sizes[1] == 5);
			assert(sizes[2] == 6);
		}
	};
}