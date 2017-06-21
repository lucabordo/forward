#include "stdafx.h"
#include "CppUnitTest.h"

#include <vector>
#include <string>

#include "forward.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace forward
{		
	template <typename Enumerable>
	auto convert_to_vector(const Enumerable& enumerable)
	{
		using namespace forward;
		auto enumerator = enumerable.get_enumerator();
		using T = std::remove_reference<decltype(std::get<1>(enumerator.next()))>::type;
		std::vector<T> result;

		for (;;)
		{
			auto next = enumerator.next();
			if (!has_more(next))
				break;
			result.push_back(get_value(next));
		}

		return result;
	}

	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(BasicEnumerationFromVector)
		{
			using namespace forward;
			auto vec = std::vector<int>{ 1, 2, 3, 4 };
			auto list = from(vec);
			std::vector<int> converted = convert_to_vector(list);

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

			auto lambda = [](int x) {return x + 1; };

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

		// Interating counter-examples
		// from(std::vector<int>{1,2,3})
		// >> select([](int i){return std::make_unique<int>(i); })
	};
}