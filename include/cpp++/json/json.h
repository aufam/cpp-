#ifndef CPPXX_JSON_JSON_H
#define CPPXX_JSON_JSON_H

#include <cpp++/serde/tag_info.h>

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

#endif
