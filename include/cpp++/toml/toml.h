#ifndef CPPXX_TOML_TOML_H
#define CPPXX_TOML_TOML_H

#include <cpp++/serde/tag_info.h>

namespace cppxx::toml {
    template <typename T>
    constexpr serde::TagInfo get_tag_info(const T &field) {
        return serde::get_tag_info(field, "toml");
    }

    template <typename... T>
    constexpr serde::TagInfoTuple<sizeof...(T)> get_tag_info_from_tuple(const std::tuple<T...> &fields) {
        return serde::get_tag_info_from_tuple(fields, "toml");
    }
} // namespace cppxx::toml

#endif
