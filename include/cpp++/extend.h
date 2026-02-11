#ifndef CPPXX_SERDE_EXTEND_H
#define CPPXX_SERDE_EXTEND_H

#include <cpp++/tuple.h>

#ifndef BOOST_PFR_HPP
#    if __has_include(<boost/pfr.hpp>)
#        include <boost/pfr.hpp>
#    endif
#endif

namespace cppxx {
    template <
        typename T,
        typename Enable = std::enable_if_t<
            is_tuple_v<T>
#ifdef BOOST_PFR_HPP
            || std::is_aggregate_v<T>
#endif
            >>
    class Extend {

    protected:
        T value;

    public:
        using type = T;

        template <typename U = T, typename = std::enable_if_t<std::is_default_constructible_v<U>>>
        constexpr Extend()
            : value{} {}

        constexpr Extend(const T &value)
            : value(value) {}

        constexpr Extend(T &&value)
            : value(std::move(value)) {}

        constexpr const T &operator()() const & {
            return value;
        }
        constexpr T &operator()() & {
            return value;
        }
        constexpr T &&operator()() && {
            return std::move(value);
        }

        void operator=(const T &) = delete;

        constexpr const T &get_value() const & {
            return value;
        }
        constexpr T &get_value() & {
            return value;
        }
        constexpr T &&get_value() && {
            return std::move(value);
        }

        constexpr auto as_tuple() const & {
#ifdef BOOST_PFR_HPP
            if constexpr (!is_tuple_v<T>)
                return boost::pfr::structure_tie(value);
            else
#endif
                return std::apply([](auto &...items) { return std::tuple_cat(std::tie(items)...); }, value);
        }

        constexpr auto as_tuple() & {
#ifdef BOOST_PFR_HPP
            if constexpr (!is_tuple_v<T>)
                return boost::pfr::structure_tie(value);
            else
#endif
                return std::apply([](auto &...items) { return std::tuple_cat(std::tie(items)...); }, value);
        }
    };

    template <typename T>
    struct is_extended : std::false_type {};

    template <typename T>
    struct is_extended<Extend<T>> : std::true_type {};

    template <typename T>
    inline static constexpr bool is_extended_v = is_extended<T>::value;


    template <typename T>
    struct remove_extend;

    template <typename T>
    struct remove_extend<Extend<T>> {
        using type = typename Extend<T>::type;
    };

    template <typename T>
    using remove_extend_t = typename remove_extend<T>::type;

    template <typename T>
    auto flatten_one(T &v) {
        if constexpr (is_extended_v<std::decay_t<T>>)
            return std::apply([](auto &...items) { return std::tuple_cat(flatten_one(items)...); }, v.as_tuple());
        else
            return std::tie(v);
    }

    template <typename... Ts>
    auto flatten(std::tuple<Ts &...> tpl) {
        return std::apply([](auto &...items) { return std::tuple_cat(flatten_one(items)...); }, tpl);
    }

    template <typename... Ts>
    auto flatten(const std::tuple<Ts...> &tpl) {
        return std::apply([](auto &...items) { return std::tuple_cat(flatten_one(items)...); }, tpl);
    }

    template <typename... Ts>
    auto flatten(std::tuple<Ts...> &tpl) {
        return std::apply([](auto &...items) { return std::tuple_cat(flatten_one(items)...); }, tpl);
    }
} // namespace cppxx

#endif
