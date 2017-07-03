#pragma once

#include "forward-basics.h"

#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <random>

namespace forward
{
    // CONTAINS:
    // to_unordered_set, distinct
    // 
    // TODO:
    // to_sorted_vector, orderby
    // revert
    // permutate_randomly
    // lines of a text file, files of a directory

#pragma region Set

    template <typename Enumerable>
    auto to_set(const Enumerable& enumerable)
    {
        static_assert(Enumerable::is_enumerable, "Oops.");
        auto enumerator = enumerable.get_enumerator();

        using actual_type = decltype(std::get<1>(enumerator.next()));
        using stored_type = std::remove_reference<actual_type>::type;

        std::unordered_set<stored_type> result;

        for (;;)
        {
            auto&& next = enumerator.next();
            if (!has_more(next))
                break;
            result.insert(std::forward<actual_type>(std::get<1>(next)));
        }

        return result;
    }

    template <typename T>
    class ToSet
    {
    public:

        template <typename Enumerable>
        std::unordered_set<T> apply(const Enumerable& enumerable) const
        {
            return to_set(enumerable);
        }
    };

    template <typename Enumerable, typename T>
    std::unordered_set<T> operator >> (const Enumerable& enumerable, const ToSet<T>& fold)
    {
        return fold.apply(enumerable);
    }

    template <typename T>
    ToSet<T> to_set()
    {
        return ToSet<T>();
    }


    template <typename T>
    class Distinct
    {
    public:

        template <typename Enumerable>
        auto apply(const Enumerable& enumerable) const
        {
            return from(to_set(enumerable));
        }
    };

    template <typename Enumerable, typename T>
    auto operator >> (const Enumerable& enumerable, const Distinct<T>& fold)
    {
        return fold.apply(enumerable);
    }

    template <typename T>
    auto distinct()
    {
        return Distinct<T>();
    }

#pragma endregion
}
