#pragma once

#include <memory>
#include <functional>
#include <cassert>
#include <vector>

namespace forward
{
	template <typename T>
	bool has_more(const T& current)
	{
		return std::get<0>(current);
	}

	template <typename T>
	auto get_value(T& current)
	{
		return std::get<1>(current);
	}

	template <typename ReturnType>
	auto yield_break()
	{
		using U = std::remove_reference<ReturnType>::type;
		return std::make_tuple(false, U{});
	}

	template <typename ReturnType>
	auto yield_return(ReturnType&& current)
	{
		return std::make_tuple(true, std::move(current));
	}


	#pragma region Enumerators

	// Enumerators are conceptually as follows. 
	// Enumerators are moved within more complex enumerators. They should be lightweight objects.
	/*
  	class AbstractEnumerator
	{
	public:

		// Calculate the next value, paired with a Boolean saying whether it is well-defined
		nullable<int> next();
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

		EnumeratorFromIterator(Iterator begin, Iterator end) :
			_current(std::move(begin)),
			_end(std::move(end))
		{
		}

		// If you need the type of this function:
		// std::remove_reference<decltype(std::get<1>(next()))>::type
		auto next()
		{
			if (_current == _end)
			{
				return yield_break<decltype(*_current)>();
			}
			else
			{
				auto result = yield_return(*_current);
				++_current;
				return std::move(result);
			}
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

		SelectEnumerator(Enumerator enumerator, const Map& map) :
			_enumerator(std::move(enumerator)),
			_map(map)
		{
		}

		auto next()
		{
			auto underlying = _enumerator.next();

			return has_more(underlying)
				? yield_return(_map(get_value(underlying)))
				: yield_break<decltype(_map(get_value(underlying)))>();
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

		WhereEnumerator(Enumerator enumerator, const Filter& filter) :
			_enumerator(std::move(enumerator)),
			_filter(filter)
		{
		}

		auto next()
		{
			for (;;)
			{
				auto current = _enumerator.next();
				using T = decltype(get_value(current));

				if (!has_more(current))
				{
					return yield_break<T>();
				}

				if (_filter(get_value(current)))
				{
					return yield_return(get_value(current));
				}
			}
		}

	private:

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
		std::vector<T> apply(const Enumerable& enumerable) const
		{
			static_assert(Enumerable::is_enumerable, "Oops.");
			std::vector<T> result;
			for (auto enumerator = enumerable.get_enumerator();;)
			{
				auto next = enumerator.next();
				if (!has_more(next))
					break;
				result.push_back(get_value(next));
			}
			return result;
		}
	};


	template <typename Enumerable, typename T>
	std::vector<T> operator >> (const Enumerable& enumerable, const ToVector<T>& fold)
	{
		return fold.apply(enumerable);
	}

	template <typename T>
	ToVector<T> to_vector()
	{
		return ToVector<T>();
	}
}

