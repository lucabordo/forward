#include "stdafx.h"
#include "CppUnitTest.h"

#include <vector>

#include "forward.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace forward
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestEnumeratorFromIterator)
		{
			using namespace forward;
			std::vector<int> v{15, 21};

			auto en = from(v);
			std::vector<int> copy = enumerable_to_vector(en);

			assert(copy.size() == 2);
			assert(copy[0] == v[0]);
			assert(copy[1] == v[1]);
		}


		TEST_METHOD(TestWhere)
		{
			using namespace forward;
			std::vector<int> v{ 15, 21 };
			auto selector = [](int i) { return i < 20; };

			auto en = from(v);
			auto selection = where_from_enumerable(en, selector);

			auto result = enumerable_to_vector(selection);

			assert(result.size() == 1);
			assert(result[0] == 15);
		}


		TEST_METHOD(TestSelect)
		{
			using namespace forward;
			std::vector<int> v{ 15, 21 };
			auto selector = [](int i) { return i + 3; };

			auto en = from(v);
			auto selection = select_from_enumerable(en, selector);

			auto result = enumerable_to_vector(selection);

			assert(result.size() == 2);
			assert(result[0] == 18);
			assert(result[1] == 24);
		}

		TEST_METHOD(Composition1)
		{
			using namespace forward;
			std::vector<int> v{ 15, 21 };

			auto en = from(v)
				>> where([](int i) { return i < 20; });

			auto result = enumerable_to_vector(en);

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

//			auto result = enumerable_to_vector(en);

			assert(result.size() == 1);
			assert(result[0] == 115);
		}

		// Interating counter-examples
		// from(std::vector<int>{1,2,3})
		// >> select([](int i){return std::make_unique<int>(i); })
	};
}