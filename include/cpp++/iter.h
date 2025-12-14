#ifndef CPPXX_ITER_H
#define CPPXX_ITER_H

#include <iterator>
#include <tuple>
#include <limits>

namespace cppxx::iter_detail {
    template <typename Item>
    decltype(auto) deref(Item item) {
        if constexpr (std::is_integral_v<Item>) {
            return Item(item);
        } else {
            return *item;
        }
    }

    template <typename... T>
    struct DefaultMapper {
        constexpr std::tuple<decltype(deref(std::declval<T>()))...>
        operator()(decltype(deref(std::declval<T>()))... elems) const {
            return {elems...};
        }
    };
} // namespace cppxx::iter_detail

namespace cppxx {
    template <template <typename...> typename To, typename From>
    struct template_rebind;

    template <
        template <typename...> typename To,
        template <typename...> typename From,
        typename... Ts>
    struct template_rebind<To, From<Ts...>> {
        using type = To<Ts...>;
    };

    template <template <typename...> typename To, typename From>
    using template_rebind_t = typename template_rebind<To, From>::type;


    template <typename F, typename Tuple>
    struct is_invocable_with_tuple;

    template <typename F, template <typename...> typename Tuple, typename... Ts>
    struct is_invocable_with_tuple<F, Tuple<Ts...>> : std::is_invocable<F, Ts...> {};

    template <typename F, typename Tuple>
    inline constexpr bool is_invocable_with_tuple_v = is_invocable_with_tuple<F, Tuple>::value;

    template <typename... Iterables>
    class TupleIterable {
        std::tuple<Iterables...> tpl;

    public:
        constexpr TupleIterable(Iterables... i)
            : tpl{i...} {}
    };

    template <
        typename T1,
        typename T2 = T1,
        typename F  = template_rebind_t<iter_detail::DefaultMapper, T1>>
    class Iter {
    public:
        constexpr Iter(T1 start, T2 stop, F mapper)
            : start(start)
            , stop(stop)
            , mapper(std::move(mapper)) {}

        constexpr decltype(auto) get() const {
            return std::apply(
                [this](const auto &...elems) { return mapper(iter_detail::deref(elems)...); }, start
            );
        }

        constexpr operator bool() const {
            return !any_equal(start, stop, std::make_index_sequence<std::tuple_size_v<T1>>{});
        }

        constexpr bool is_done() const {
            return any_equal(start, stop, std::make_index_sequence<std::tuple_size_v<T1>>{});
        }

        constexpr void next() {
            std::apply([](auto &...elems) { (++elems, ...); }, start);
        }

        template <typename I = int>
        constexpr auto enumerate(I start = 0, I stop = std::numeric_limits<I>::max()) const {
            static_assert(
                std::is_same_v<F, template_rebind_t<iter_detail::DefaultMapper, T1>>,
                "Only if mapper is default"
            );
            auto cat_start = std::tuple_cat(std::make_tuple(start), this->start);
            auto cat_stop  = std::tuple_cat(std::make_tuple(stop), this->stop);
            return Iter<decltype(cat_start), decltype(cat_stop)>(cat_start, cat_stop, {});
        }

        template <typename... U>
        constexpr auto zip(const Iter<std::tuple<U...>> &other) const {
            static_assert(
                std::is_same_v<F, template_rebind_t<iter_detail::DefaultMapper, T1>>,
                "Only if mapper is default"
            );
            auto cat_start = std::tuple_cat(start, other.start);
            auto cat_stop  = std::tuple_cat(stop, other.stop);
            return Iter<decltype(cat_start), decltype(cat_stop)>(cat_start, cat_stop, {});
        }

        constexpr Iter &drop(size_t n) {
            for (size_t i = 0; *this && i < n; ++i)
                next();
            return *this;
        }

        constexpr Iter &take(size_t n) {
            static_assert(std::is_same_v<T1, T2>, "Only if start and stop are the same type");

            stop = Iter<T1, T2>(start, stop, {}).drop(n).start;
            return *this;
        }

        template <typename FF>
        constexpr auto map(FF &&fn) {
            static_assert(
                is_invocable_with_tuple_v<FF, decltype(get())>, "Must be invocable with get()"
            );
            return Iter<T1, T2, FF>(start, stop, std::forward<FF>(fn));
        }

        template <
            template <typename...> typename Container,
            typename... ContainerSpecs,
            typename... Args>
        constexpr auto collect() {
            struct Iterator {
                Iter *ptr;

                using value_type = std::decay_t<decltype(ptr->get())>;
                decltype(auto) operator*() const {
                    return ptr->get();
                }
                bool operator==(Iterator) const {
                    return !(*ptr);
                }
                bool operator!=(Iterator) const {
                    return (*ptr);
                }
                Iter operator++() {
                    return {&ptr->next()};
                }
            };
            return T(Iterator{this}, Iterator{this});
        }

        T1 start;
        T2 stop;
        F  mapper;

    protected:
        template <size_t... I>
        static bool any_equal(const T1 &start, const T2 &stop, std::index_sequence<I...>) {
            return ((std::get<I>(start) == std::get<I>(stop)) || ...);
        }
    };

    template <typename T1, typename T2>
    auto iterate(T1 start, T2 stop) {
        return Iter<std::tuple<T1>, std::tuple<T2>>(
            std::make_tuple(start), std::make_tuple(stop), {}
        );
    }

    template <typename Iterable>
    auto iterate(const Iterable &iterable) {
        return iterate(std::begin(iterable), std::end(iterable));
    }

    template <typename Iterable>
    auto iterate(Iterable &iterable) {
        return iterate(std::begin(iterable), std::end(iterable));
    }

    template <typename Iterable>
    void iterate(Iterable &&iterable) = delete;

    template <typename Iterable>
    auto reversed(const Iterable &iterable) {
        return iterate(std::rbegin(iterable), std::rend(iterable));
    }

    template <typename Iterable>
    auto reversed(Iterable &iterable) {
        return iterate(std::rbegin(iterable), std::rend(iterable));
    }

    template <typename Iterable>
    void reversed(Iterable &&iterable) = delete;

    template <typename... T, typename... U, typename... Others>
    auto zip(const Iter<T...> &iter_a, const Iter<U...> &iter_b, const Others &...others) {
        if constexpr (sizeof...(Others) == 0) {
            return iter_a.zip(iter_b);
        } else {
            return zip(iter_a.zip(iter_b), others...);
        }
    }
} // namespace cppxx

#endif
