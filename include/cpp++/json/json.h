#ifndef CPPXX_JSON_JSON_H
#define CPPXX_JSON_JSON_H

#include <cpp++/serde/serde.h>
#include <cpp++/optional.h>

#ifndef BOOST_PFR_HPP
#    include <boost/pfr.hpp>
#endif

namespace cppxx::json {
    using serde::TagInfo;

    template <typename T>
    constexpr TagInfo get_tag_info(const Tag<T> &field) {
        return serde::get_tag_info(field, "json");
    }
} // namespace cppxx::json

namespace cppxx::json::detail {
    template <typename T>
    struct is_aggregate_contains_tagged {
        static constexpr bool value_fn() {
            if constexpr (!std::is_aggregate_v<T> || !std::is_class_v<T>)
                return false;
            else if constexpr (boost::pfr::tuple_size_v<T> == 0)
                return true; // empty struct is empty obj
            else
                return value_fn_index_sequence(std::make_index_sequence<boost::pfr::tuple_size_v<T>>{});
        }

        template <size_t... I>
        static constexpr bool value_fn_index_sequence(std::index_sequence<I...>) {
            return (::cppxx::is_tagged_v<boost::pfr::tuple_element_t<I, T>> || ...);
        }

        static constexpr bool value = value_fn();
    };

    template <typename T>
    inline static constexpr bool is_aggregate_contains_tagged_v = is_aggregate_contains_tagged<T>::value;

    template <typename, typename = void>
    struct has_empty : std::false_type {};

    template <typename T>
    struct has_empty<T, std::void_t<decltype(std::declval<const T &>().empty())>> : std::true_type {};

    template <typename T>
    std::enable_if_t<!has_empty<T>::value, bool> is_empty_value(const T &value) {
        if constexpr (is_optional<T>::value)
            return !value.has_value();
        else if constexpr (std::is_arithmetic_v<T>)
            return !bool(value);
        else
            return false;
    }

    template <typename T>
    std::enable_if_t<has_empty<T>::value, bool> is_empty_value(const T &value) {
        return value.empty();
    }
} // namespace cppxx::json::detail

#endif
