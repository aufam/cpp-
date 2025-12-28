#ifndef CPPXX_PROTO_PROTO_H
#define CPPXX_PROTO_PROTO_H

#include <cpp++/serde/tag_info.h>
#include <array>

namespace cppxx::proto {
    template <typename T>
    constexpr serde::TagInfo get_tag_info(const T &field) {
        return serde::get_tag_info(field, "proto");
    }

    constexpr int get_field_number(const serde::TagInfo &ti) {
        auto   sv       = ti.key;
        int    result   = 0;
        bool   negative = false;
        size_t i        = 0;

        if (i < sv.length() && sv[i] == '-') {
            negative = true;
            ++i;
        }

        for (; i < sv.length(); ++i)
            if (sv[i] >= '0' && sv[i] <= '9')
                result = result * 10 + (sv[i] - '0');
            else
                break;

        return negative ? -result : result;
    }

    template <typename... Ts>
    constexpr std::array<int, sizeof...(Ts)> get_field_nubers_from_tuple(const std::tuple<Ts...> &tpl) {
        std::array<int, sizeof...(Ts)> ret = {};
        tuple_for_each(tpl, [&](const auto &v, size_t i) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (is_tagged_v<T>)
                ret[i] = get_field_number(get_tag_info(v));
        });
        return ret;
    }

} // namespace cppxx::proto

#endif
