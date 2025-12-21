#ifndef CPPXX_TUPLE_H
#define CPPXX_TUPLE_H

#include <tuple>
#include <type_traits>

namespace cppxx {
    template <typename T>
    struct is_tuple : std::false_type {};

    template <typename... Ts>
    struct is_tuple<std::tuple<Ts...>> : std::true_type {};

    template <typename T>
    constexpr bool is_tuple_v = is_tuple<T>::value;


    template <typename F, typename Tuple>
    struct is_invocable_with_tuple;

    template <typename F, typename... Ts>
    struct is_invocable_with_tuple<F, std::tuple<Ts...>> : std::is_invocable<F, Ts...> {};

    template <typename F, typename Tuple>
    inline constexpr bool is_invocable_with_tuple_v = is_invocable_with_tuple<F, Tuple>::value;

    template <typename Tuple, typename F>
    constexpr void tuple_for_each(Tuple &&tpl, F &&fn) {
        std::apply(
            [&](auto &...item) {
                std::size_t i = 0;
                (fn(item, i++), ...);
            },
            std::forward<Tuple>(tpl)
        );
    }
} // namespace cppxx

#endif
