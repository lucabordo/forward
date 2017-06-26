#pragma once

#include <memory>
#include <functional>
#include <cassert>
#include <vector>

namespace forward
{
	#pragma region Nullable objects, represented by a tuple<bool, T>

	template <typename T>
	bool has_more(const T& current)
	{
		return std::get<0>(current);
	}

	template <typename T>
	auto get_value(const T& current)
	{
		return std::get<1>(current);
	}

	template <typename ReturnType>
	auto yield_break()
	{
		return std::make_tuple(false, ReturnType{});
	}

	template <typename ReturnType>
	auto yield_return(ReturnType&& current)
	{
		return std::make_tuple(true, std::forward<ReturnType>(current));
	}

	#pragma endregion

	#pragma region Enumerators

	// Enumerators are conceptually as follows. 
	// Enumerators are moved within more complex enumerators. They should be lightweight objects.
	// 
	/*

	template <typename Return_type> // Not the actual paramaterization. ReturnType needs to be default constructible
  	struct AbstractEnumerator
	{
		// Returns (true, next calculated value),
		// Or, if no next value exists: (false, default_coonstructor())
		std::tuple<bool, Return_type> next();
	};
	
	*/


	// An enumerator based on a pair of STL-style iterators.
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

		auto next()
		{
			using actual_type = decltype(*_current);
			using stored_type = std::remove_reference<actual_type>::type;

			if (_current == _end)
			{
				return yield_break<actual_type>();
			}
			else
			{
				auto&& result = *_current;
				++_current;
				return yield_return(std::forward<actual_type>(result));
			}
		}

	private:

		Iterator _current;
		Iterator _end; // const, but the Iterator might be moved
	};


	// An enumerator that applies a tranform to all elements of an underlying enumerator.
	// The function is assumed to be stateless, deterministic, and is only const-referred to.
	// Implements:
	//
	//    for (; en.has_value(); en.forward())
	//    {
	//        yield return map(en.get_value());
	//    }
	//
	template <typename Enumerator, typename Transform>
	class SelectEnumerator
	{
	public:

		SelectEnumerator(Enumerator enumerator, const Transform& transform) :
			_enumerator(std::move(enumerator)),
			_transform(transform)
		{
		}

		auto next()
		{
			auto&& underlying = _enumerator.next();
			using result_type = decltype(_transform(std::get<1>(underlying)));

			return has_more(underlying)
				? yield_return(_transform(std::get<1>(underlying)))
				: yield_break<result_type>();
		}

	private:

		Enumerator _enumerator;
		const Transform& _transform;
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
			using actual_type = decltype(std::get<1>(_enumerator.next()));

			for (;;)
			{
				auto&& current = _enumerator.next();

				if (!has_more(current))
					return yield_break<actual_type>();

				if (_filter(get_value(current)))
					return yield_return<>(std::forward<actual_type>(get_value(current)));
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

	struct Enumerable 
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


	template <typename Enumerable, typename Transform>
	class SelectEnumerable
	{
	public:

		static const bool is_enumerable = true;
		using enumerator = SelectEnumerator<typename Enumerable::enumerator, Transform>;

		SelectEnumerable(const Enumerable& enumerable, const Transform& filter) :
			_enumerable(enumerable),
			_transform(std::move(filter))
		{
		}

		enumerator get_enumerator() const
		{
			return enumerator(_enumerable.get_enumerator(), _transform);
		}

	private:

		const Enumerable& _enumerable;
		const Transform& _transform;
	};

	#pragma endregion

	#pragma region Syntax for from... where... select

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
	template <typename Transform>
	class SelectRightHandSide
	{
	private:
		const Transform& _map;
	public:
		SelectRightHandSide(const Transform& map) : _map(map) {}

		template <typename Enumerable>
		SelectEnumerable<Enumerable, Transform> apply(const Enumerable& enumerable) const
		{
			return SelectEnumerable<Enumerable, Transform>(enumerable, _map);
		}
	};

	template <typename Transform>
	SelectRightHandSide<Transform> select(const Transform& map)
	{
		return SelectRightHandSide<Transform>(map);
	}

	template <typename Enumerable, typename Transform>
	SelectEnumerable<Enumerable, Transform> operator >> (const Enumerable& enumerable, const SelectRightHandSide<Transform>& selectRightHandSide)
	{
		return selectRightHandSide.apply(enumerable);
	}

	#pragma endregion

	#pragma region Accumulator functions

	template <typename Enumerable>
	auto to_vector(const Enumerable& enumerable)
	{
		static_assert(Enumerable::is_enumerable, "Oops.");
		auto enumerator = enumerable.get_enumerator();

		using actual_type = decltype(std::get<1>(enumerator.next()));
		using stored_type = std::remove_reference<actual_type>::type;
		
		std::vector<stored_type> result;

		for (;;)
		{
			auto&& next = enumerator.next();
			if (!has_more(next))
				break;
			result.push_back(std::forward<actual_type>(std::get<1>(next)));
		}

		return result;
	}

	template <typename T>
	class ToVector
	{
	public:

		template <typename Enumerable>
		std::vector<T> apply(const Enumerable& enumerable) const
		{
			return to_vector(enumerable);
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

	#pragma endregion

    // TODO
	// range 
	// take, skip, 
	// single,
	// is_empty, 
	// sum_from, count
	// zip, unzip? 

	// revert

	// with additional includes
	// orderby
	// distinct
	// shuffle
}
