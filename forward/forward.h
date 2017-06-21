#pragma once

#include <memory>
#include <functional>
#include <cassert>
#include <vector>

namespace forward
{
/*	template <typename Iteratable>
	class EnumerableFromIteratableRef;

	template <typename Enumerable, typename Filter>
	class WhereEnumerable;

	template <typename Enumerable, typename Map>
	class SelectEnumerable;
	*/

	
	// No auto...

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

		// Convention: always returns a reference to something pre-computed.
		// This is to avoid any re-computation
		const value_type& get_value() const
		{
			return *_current;
		}

		void forward()
		{
			assert(has_value());
			++_current;
		}

	private:

		Iterator _current;
		const Iterator _end;
	};


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


	template <typename Enumerator, typename Map>
	class SelectEnumerator
	{
	public:

		static const bool is_enumerator = true;
		using value_type = typename Enumerator::value_type;

		SelectEnumerator(Enumerator&& enumerator, const Map& map) :
			_enumerator(enumerator),
			_map(map)
		{
			calculate_current_value();
		}

		bool has_value() const
		{
			return _enumerator.has_value();
		}

		const value_type& get_value() const
		{
			return _current;
		}

		void forward()
		{
			assert(has_value());
			_enumerator.forward();
			calculate_current_value();
		}

	private:

		void calculate_current_value()
		{
			_valid = _enumerator.has_value();
			if (_valid)
				_current = _map(_enumerator.get_value());
		}

	private:

		Enumerator _enumerator;
		const Map& _map;

		value_type _current; // default-constructible, therefore
		bool _valid;
	};


	template <typename Enumerator, typename Filter>
	class WhereEnumerator
	{
	public:

		static const bool is_enumerator = true;
		using value_type = typename Enumerator::value_type;

		WhereEnumerator(Enumerator&& enumerator, const Filter& filter) :
			_enumerator(enumerator),
			_filter(filter)
		{
			seek();
		}

		bool has_value() const
		{
			return _enumerator.has_value();
		}

		const value_type& get_value() const
		{
			return _enumerator.get_value();
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
			while (_enumerator.has_value())
			{
				if (_filter(_enumerator.get_value()))
					break;
				else
					_enumerator.forward();
			}
		}


	private:

		Enumerator _enumerator;
		const Filter& _filter;
	};


	template <typename Enumerable, typename Filter>
	class WhereEnumerable
	{
	public:

		static const bool is_enumerable = true;
		using value_type = typename Enumerable::value_type;
		using enumerator = WhereEnumerator<typename Enumerable::enumerator, Filter>;

		WhereEnumerable(const Enumerable& enumerable, Filter filter) :
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
		Filter _filter;
	};


	template <typename Enumerable, typename Map>
	class SelectEnumerable
	{
	public:

		static const bool is_enumerable = true;
		using value_type = typename Enumerable::value_type;
		using enumerator = SelectEnumerator<typename Enumerable::enumerator, Map>;

		SelectEnumerable(const Enumerable& enumerable, Map filter) :
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
		Map _map;
	};



	template <typename Iteratable>
	EnumerableFromIteratableRef<Iteratable> from(const Iteratable& iteratable)
	{
		return EnumerableFromIteratableRef<Iteratable>(iteratable);
	}


	// Ugly function for test
	template <typename Enumerable>
	std::vector<typename Enumerable::value_type> enumerable_to_vector(const Enumerable& enumerable)
	{
		static_assert(Enumerable::is_enumerable, "Oops.");
		std::vector<typename Enumerable::value_type> result;
		for (auto enumerator = enumerable.get_enumerator(); enumerator.has_value(); enumerator.forward())
			result.push_back(enumerator.get_value());
		return result;
	}


	// Ugly function for test
	template <typename Enumerable, typename Filter>
	auto where_from_enumerable(const Enumerable& enumerable, Filter filter)
	{
		return WhereEnumerable<Enumerable, Filter>(enumerable, std::move(filter));
	}


	// Ugly function for test
	template <typename Enumerable, typename Map>
	auto select_from_enumerable(const Enumerable& enumerable, Map map)
	{
		return SelectEnumerable<Enumerable, Map>(enumerable, std::move(map));
	}


	// Allow right hand side composition for where
	template <typename Filter>
	class WhereRightHandSide
	{
	private:
		Filter _filter;
	public:
		WhereRightHandSide(Filter filter) : _filter(std::move(filter)) {}

		template <typename Enumerable>
		WhereEnumerable<Enumerable, Filter> apply(const Enumerable& enumerable) const
		{
			// Avoid copying this filter?
			return WhereEnumerable<Enumerable, Filter>(enumerable, _filter);
		}
	};

	template <typename Filter>
	WhereRightHandSide<Filter> where(Filter filter) 
	{
		return WhereRightHandSide<Filter>(std::move(filter));
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
		Map _map;
	public:
		SelectRightHandSide(Map map) : _map(std::move(map)) {}

		template <typename Enumerable>
		SelectEnumerable<Enumerable, Map> apply(const Enumerable& enumerable) const
		{
			// Avoid copying this filter?
			return SelectEnumerable<Enumerable, Map>(enumerable, _map);
		}
	};

	template <typename Map>
	SelectRightHandSide<Map> select(Map map)
	{
		return SelectRightHandSide<Map>(std::move(map));
	}

	template <typename Enumerable, typename Map>
	SelectEnumerable<Enumerable, Map> operator >> (const Enumerable& enumerable, const SelectRightHandSide<Map>& selectRightHandSide)
	{
		return selectRightHandSide.apply(enumerable);
	}


	// Some end point functions that do not produce an Enumerable

	//template <typename Accumulator>
	//class FoldRightHandSide
	//{
	//private:
	//	
	//	Accumulator _accumulator;

	//public:

	//	FoldRightHandSide(Accumulator accumulator) :
	//		_accumulator(std::move(accumulator))
	//	{
	//	}

	//	template <typename Enumerable>
	//	auto apply(const Enumerable& enumerable) const
	//	{
	//		static_assert(Enumerable::is_enumerable, "Oops.");
	//		auto result = _accumulator; // copy for sanity?
	//		for (auto enumerator = enumerable.get_enumerator(); enumerator.has_value(); enumerator.forward())
	//			_accumulator.accumulate(enumerator.get_value());
	//		return _accumulator.result();
	//	}
	//};

	//template <typename Enumerable, typename Accumulator>
	//auto operator >> (const Enumerable& enumerable, const FoldRightHandSide<Accumulator>& fold)
	//{
	//	return fold.apply(enumerable);
	//}


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



	/*
	template <typename Enumerable>
	ToVector<Enumerable::value_type> to_vector(const Enumer)
	{
		return ToVector<Enumerable::value_type>();
	}


	template <typename Initializer, typename Action>
	class FoldRightHandSide
	{
	private:

		Initializer _initializer; // RENAME
		Action _action;

	public:

		FoldRightHandSide(Initializer initializer, Action action) :
			_initializer(std::move(initializer)),
			_action(std::move(action))
		{
		}

		template <typename Enumerable>
		auto apply(const Enumerable& enumerable) const
		{
			static_assert(Enumerable::is_enumerable, "Oops.");
			auto result = _initializer;
			for (auto enumerator = enumerable.get_enumerator(); enumerator.has_value(); enumerator.forward())
				_action(enumerator.get_value());
			return result;
		}
	};

	template <typename Enumerable, typename Initializer, typename Action>
	auto operator >> (const Enumerable& enumerable, const FoldRightHandSide<Initializer, Action>& fold)
	{
		return fold.apply(enumerable);
	}
*/

}

