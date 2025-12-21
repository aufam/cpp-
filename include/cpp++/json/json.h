#ifndef CPPXX_JSON_JSON_H
#define CPPXX_JSON_JSON_H

#include <cpp++/serde/tag_info.h>
#include <cpp++/optional.h>

namespace cppxx::json {
    template <typename T>
    constexpr serde::TagInfo get_tag_info(const T &field) {
        return serde::get_tag_info(field, "json");
    }

    template <typename... T>
    constexpr serde::TagInfoTuple<sizeof...(T)> get_tag_info_from_tuple(const std::tuple<T...> &fields) {
        return serde::get_tag_info_from_tuple(fields, "json");
    }
} // namespace cppxx::json

namespace cppxx::json::detail {
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
