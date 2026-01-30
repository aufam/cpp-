#ifndef CPPXX_FMT_H
#define CPPXX_FMT_H

#include <cpp++/serde/tag_info.h>
#include <optional>
#include <variant>

#ifndef FMT_RANGES_H_
#    include <fmt/ranges.h>
#endif

#ifndef FMT_CHRONO_H_
#    include <fmt/chrono.h>
#endif

#ifndef BOOST_PFR_HPP
#    if __has_include(<boost/pfr.hpp>)
#        include <boost/pfr.hpp>
#    endif
#endif

#ifndef NEARGYE_MAGIC_ENUM_HPP
#    if __has_include(<magic_enum/magic_enum.hpp>)
#        include <magic_enum/magic_enum.hpp>
#    endif
#endif

template <typename T>
struct fmt::formatter<cppxx::Tag<T>, char, std::enable_if_t<fmt::is_formattable<T, char>::value>> : fmt::formatter<T> {
    fmt::context::iterator format(const cppxx::Tag<T> &v, fmt::context &c) const {
        if (cppxx::serde::TagInfo ti = cppxx::serde::get_tag_info(v, "fmt"); ti.key != "") {
            fmt::context::iterator out = c.out();

            out = fmt::format_to(out, "{}=", ti.key);
        }
        return fmt::formatter<T>::format(v.get_value(), c);
    }
};

template <typename T>
struct fmt::formatter<std::optional<T>, char, std::enable_if_t<fmt::is_formattable<T, char>::value>> : fmt::formatter<T> {
    fmt::context::iterator format(const std::optional<T> &v, fmt::context &c) const {
        if (v.has_value()) {
            return fmt::formatter<T>::format(*v, c);
        }
        fmt::context::iterator out = c.out();
        return fmt::format_to(out, "null");
    }
};

template <typename... T>
struct fmt::formatter<std::variant<T...>, char, std::enable_if_t<(fmt::is_formattable<T, char>::value && ...)>> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    fmt::context::iterator format(const std::variant<T...> &v, fmt::context &c) const {
        fmt::context::iterator out = c.out();
        return std::visit(
            [&](const auto &var) {
                if constexpr (std::is_same_v<std::decay_t<decltype(var)>, std::string> ||
                              std::is_same_v<std::decay_t<decltype(var)>, std::string_view>)
                    return fmt::format_to(out, "{:?}", var);
                else
                    return fmt::format_to(out, "{}", var);
            },
            v
        );
    }
};

#ifdef BOOST_PFR_HPP
template <typename S>
struct fmt::formatter<S, char, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
    constexpr auto parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    fmt::context::iterator format(const S &v, fmt::context &c) const {
        fmt::context::iterator out = c.out();

        auto tpl =
            std::apply([&](auto &...item) { return cppxx::tie_if<fmt::is_formattable>(item...); }, boost::pfr::structure_tie(v));

        return fmt::format_to(out, "{}", tpl);
    }
};
#endif

#ifdef NEARGYE_MAGIC_ENUM_HPP
template <typename S>
struct fmt::formatter<S, char, std::enable_if_t<std::is_enum_v<S>>> : fmt::formatter<std::string_view> {
    fmt::context::iterator format(S v, fmt::context &c) const {
        fmt::context::iterator out = c.out();
        return fmt::format_to(out, "{}", magic_enum::enum_name(v));
    }
};
#endif

#endif
