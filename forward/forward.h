#pragma once

#include <memory>
#include <functional>
#include <cassert>
#include <vector>

namespace forward
{
	#pragma region Enumerators

	// Enumerators are conceptually as follows. 
	// Enumerators are moved within more complex enumerators. They should be lightweight objects.
	/*
  	class AbstractEnumerator
	{
	public:

		static const bool is_enumerator = true;
		using value_type = int;
		
		// True if there is a current value.
		bool has_value() const;

		// Compute the current value.
		// Calling this twice might cause the same calculation to be redone or, worse, access to a moved object.
		value_type get_value() const;

		// Move to the next value, if any. 
		void forward();
	};
	*/


	// An enumerator based on a forward-iteratable container, STL-styler
	// Implements:
	//
	// for (auto it = begin(); it != end(); ++it)
	// {
	//     yield return *it;
	// }
	//
	template <typename Iterator>
	class EnumeratorFromIterator
	{
	public:

		static const bool is_enumerator = true;
		using value_type = typename Iterator::value_type;

		EnumeratorFromIterator(Iterator begin, Iterator end) :
			_current(std::move(begin)),
			_end(std::move(end))
		{
		}

		bool has_value() const
		{
			return _current != _end;
		}

		value_type get_value() const
		{
			assert(has_value());
			return *_current;
		}

		void forward()
		{
			assert(has_value());
			++_current;
		}

	private:

		Iterator _current;
		Iterator _end; // const, but the Iterator might be moved
	};


	// An enumerator that applies a function to all elements of an underlying enumerator.
	// The function is assumed to be stateless, deterministic, and is only const-referred to.
	// Implements:
	//
	//    for (; en.has_value(); en.forward())
	//    {
	//        yield return map(en.get_value());
	//    }
	//
	template <typename Enumerator, typename Map>
	class SelectEnumerator
	{
	public:

		static const bool is_enumerator = true;
		using value_type = typename Enumerator::value_type;

		SelectEnumerator(Enumerator enumerator, const Map& map) :
			_enumerator(std::move(enumerator)),
			_map(map)
		{
		}

		bool has_value() const
		{
			return _enumerator.has_value();
		}

		value_type get_value() const
		{
			assert(has_value());
			return _map(_enumerator.get_value());
		}

		void forward()
		{
			assert(has_value());
			_enumerator.forward();
		}

	private:

		Enumerator _enumerator;
		const Map& _map;
	};


	// An enumerator that only returns the objects from an underlying enumerator that pass a certain test, or filter.
	// The function is assumed to be stateless, deterministic, and is only const-referred to.
	// Implements:
	//
	//    for (; en.has_value(); en.forward())
    //    {
    //        auto current = en.get_value();
	//        bool valid = filter(current);
	//        if (valid)
    //           yield return std::move(current);
	//    }
	//
	template <typename Enumerator, typename Filter>
	class WhereEnumerator
	{
	public:

		static const bool is_enumerator = true;
		using value_type = typename Enumerator::value_type;

		WhereEnumerator(Enumerator enumerator, const Filter& filter) :
			_enumerator(std::move(enumerator)),
			_filter(filter),
			_valid(true)
		{
			seek();
		}

		bool has_value() const
		{
			return _valid;
		}

		value_type get_value() const
		{
			assert(has_value());
			return std::move(_current);
		}

		void forward()
		{
			assert(has_value());
			_enumerator.forward();
			seek();
		}

	private:

		// Move until either end or position that passes the filter
		void seek()
		{
			assert(_valid);

			for (; _enumerator.has_value(); _enumerator.forward())
			{
				_current = _enumerator.get_value();
				if (_filter(_current))
					return;
			}

			_valid = false;
		}


	private:

		value_type _current;
		bool _valid;

		Enumerator _enumerator;
		const Filter& _filter;
	};

	#pragma endregion

	#pragma region Enumerable

	// Conceptually, enumerables are:
	/*
	class Enumerable 
	{
		static const bool is_enumerable = true;
		using value_type = int;
		using enumerator = AbstractEnumerator;

		// Produce an enumerator over this content
		enumerator get_enumerator() const;
	};
	*/


	template <typename Iteratable>
	class EnumerableFromIteratableRef
	{
	public:

		static const bool is_enumerable = true;
		using iterator = typename Iteratable::const_iterator;
		using value_type = typename iterator::value_type;
		using enumerator = EnumeratorFromIterator<iterator>;

		EnumerableFromIteratableRef(const Iteratable& iteratable) :
			_iteratable(iteratable)
		{
		}

		enumerator get_enumerator() const
		{
			return enumerator(_iteratable.begin(), _iteratable.end());
		}

	private:

		const Iteratable& _iteratable;
	};



	template <typename Enumerable, typename Filter>
	class WhereEnumerable
	{
	public:

		static const bool is_enumerable = true;
		using value_type = typename Enumerable::value_type;
		using enumerator = WhereEnumerator<typename Enumerable::enumerator, Filter>;

		WhereEnumerable(const Enumerable& enumerable, const Filter& filter) :
			_enumerable(enumerable),
			_filter(std::move(filter))
		{
		}

		enumerator get_enumerator() const
		{
			return enumerator(_enumerable.get_enumerator(), _filter);
		}

	private:

		const Enumerable& _enumerable;
		const Filter& _filter;
	};


	template <typename Enumerable, typename Map>
	class SelectEnumerable
	{
	public:

		static const bool is_enumerable = true;
		using value_type = typename Map::result_type;
//		using value_type = decltype();
//			typename Enumerable::value_type;
		using enumerator = SelectEnumerator<typename Enumerable::enumerator, Map>;

		SelectEnumerable(const Enumerable& enumerable, const Map& filter) :
			_enumerable(enumerable),
			_map(std::move(filter))
		{
		}

		enumerator get_enumerator() const
		{
			return enumerator(_enumerable.get_enumerator(), _map);
		}

	private:

		const Enumerable& _enumerable;
		const Map& _map;
	};

	#pragma endregion


	template <typename Iteratable>
	EnumerableFromIteratableRef<Iteratable> from(const Iteratable& iteratable)
	{
		return EnumerableFromIteratableRef<Iteratable>(iteratable);
	}


	// Allow right hand side composition for where
	template <typename Filter>
	class WhereRightHandSide
	{
	private:
		const Filter& _filter;
	public:
		WhereRightHandSide(const Filter& filter) : _filter(filter) {}

		template <typename Enumerable>
		WhereEnumerable<Enumerable, Filter> apply(const Enumerable& enumerable) const
		{
			return WhereEnumerable<Enumerable, Filter>(enumerable, _filter);
		}
	};

	template <typename Filter>
	WhereRightHandSide<Filter> where(const Filter& filter) 
	{
		return WhereRightHandSide<Filter>(filter);
	}

	template <typename Enumerable, typename Filter>
	WhereEnumerable<Enumerable, Filter> operator >> (const Enumerable& enumerable, const WhereRightHandSide<Filter>& whereRightHandSide)
	{
		return whereRightHandSide.apply(enumerable);
	}


	// Allow right hand side composition for select
	template <typename Map>
	class SelectRightHandSide
	{
	private:
		const Map& _map;
	public:
		SelectRightHandSide(const Map& map) : _map(map) {}

		template <typename Enumerable>
		SelectEnumerable<Enumerable, Map> apply(const Enumerable& enumerable) const
		{
			return SelectEnumerable<Enumerable, Map>(enumerable, _map);
		}
	};

	template <typename Map>
	SelectRightHandSide<Map> select(const Map& map)
	{
		return SelectRightHandSide<Map>(map);
	}

	template <typename Enumerable, typename Map>
	SelectEnumerable<Enumerable, Map> operator >> (const Enumerable& enumerable, const SelectRightHandSide<Map>& selectRightHandSide)
	{
		return selectRightHandSide.apply(enumerable);
	}


	// Some end point functions that do not produce an Enumerable

	template <typename T>
	class ToVector
	{
	public:

		template <typename Enumerable>
		std::vector<typename Enumerable::value_type> apply(const Enumerable& enumerable) const
		{
			static_assert(Enumerable::is_enumerable, "Oops.");
			std::vector<typename Enumerable::value_type> result;
			for (auto enumerator = enumerable.get_enumerator(); enumerator.has_value(); enumerator.forward())
				result.push_back(enumerator.get_value());
			return result;
		}
	};


	template <typename Enumerable, typename T>
	std::vector<typename Enumerable::value_type> operator >> (const Enumerable& enumerable, const ToVector<T>& fold)
	{
		return fold.apply(enumerable);
	}

	template <typename T>
	ToVector<T> to_vector()
	{
		return ToVector<T>();
	}
}

