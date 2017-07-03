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
    // to_ordered_vector, orderby
    // 
    // TODO:
    // revert
    //
    // permutate_randomly
    // lines of a text file, files of a directory

#pragma region ToSet, Distinct

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
            return from_moved(to_set(enumerable));
        }
    };

    template <typename Enumerable, typename T>
    auto operator >> (const Enumerable& enumerable, Distinct<T> fold)
    {
        return fold.apply(enumerable);
    }

    template <typename T>
    auto distinct()
    {
        return Distinct<T>();
    }

#pragma endregion

#pragma region Order

    template <typename Enumerable, typename Evaluation>
    auto to_vector_ordered_by(const Enumerable& enumerable, const Evaluation& evaluation)
    {
        static_assert(Enumerable::is_enumerable, "Oops.");
        auto result = to_vector(enumerable);

        std::sort(result.begin(), result.end(), [&](const auto& a, const auto& b)
        {
            return evaluation(a) < evaluation(b);
        });

        return result;
    }

    template <typename T, typename Evaluation>
    class ToVectorOrderedBy
    {
    public:

        ToVectorOrderedBy(Evaluation evaluation):
            _evaluation(evaluation) // copy
        {}

        template <typename Enumerable>
        auto apply(const Enumerable& enumerable) const
        {
            return to_vector_ordered_by(enumerable, _evaluation);
        }

    private:

        Evaluation _evaluation;
    };

    template <typename Enumerable, typename T, typename Evaluation>
    auto operator >> (const Enumerable& enumerable, const ToVectorOrderedBy<T, Evaluation>& fold)
    {
        return fold.apply(enumerable);
    }

    template <typename T, typename Evaluation>
    ToVectorOrderedBy<T, Evaluation> to_vector_ordered_by(Evaluation eval)
    {
        return ToVectorOrderedBy<T, Evaluation>(std::move(eval));
    }


    template <typename T, typename Evaluation>
    class OrderedBy
    {
    public:      
        
        OrderedBy(Evaluation evaluation) :
            _evaluation(std::move(evaluation))
        {}

        template <typename Enumerable>
        auto apply(const Enumerable& enumerable) const
        {
            return from_moved(to_vector_ordered_by(enumerable, _evaluation));
        }

    private:

        Evaluation _evaluation;
    };

    template <typename Enumerable, typename T, typename Evaluation>
    auto operator >> (const Enumerable& enumerable, OrderedBy<T, Evaluation> fold)
    {
        return fold.apply(enumerable);
    }

    template <typename T, typename Evaluation>
    auto order_by(Evaluation evaluation)
    {
        return OrderedBy<T, Evaluation>(std::move(evaluation));
    }

#pragma endregion
}
