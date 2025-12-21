#ifndef CPPXX_FMT_H
#define CPPXX_FMT_H

#include <cpp++/serde/tag_info.h>
#include <cpp++/serde/serialize.h>
#include <optional>

#ifndef FMT_RANGES_H_
#    include <fmt/ranges.h>
#endif

#ifndef FMT_CHRONO_H_
#    include <fmt/chrono.h>
#endif

template <typename T>
struct fmt::formatter<cppxx::Tag<T>, char, std::enable_if_t<fmt::is_formattable<T, char>::value>> : fmt::formatter<T> {
    fmt::context::iterator format(const cppxx::Tag<T> &v, fmt::context &c) const {
        if (cppxx::serde::TagInfo ti = cppxx::serde::get_tag_info(v, "fmt"); ti.key != "") {
            fmt::context::iterator out = c.out();

            out = fmt::format_to(out, ti.key);
            out = fmt::format_to(out, "=");
        }
        return fmt::formatter<T>::format(v.get_value(), c);
    }
};

template <typename S>
struct fmt::formatter<S, char, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> : fmt::formatter<int> {
    fmt::context::iterator format(const S &v, fmt::context &c) const {
        fmt::context::iterator out = c.out();
        return fmt::format_to(out, "{}", boost::pfr::structure_tie(v));
    }
};

template <typename T>
struct fmt::formatter<std::optional<T>> : fmt::formatter<T> {
    fmt::context::iterator format(const std::optional<T> &v, fmt::context &c) const {
        if (v.has_value()) {
            return fmt::formatter<T>::format(*v, c);
        }
        fmt::context::iterator out = c.out();
        return fmt::format_to(out, "null");
    }
};

#endif
