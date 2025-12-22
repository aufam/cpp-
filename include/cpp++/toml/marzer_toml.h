#ifndef CPPXX_TOML_MARZER_TOML_H
#define CPPXX_TOML_MARZER_TOML_H

#include <cpp++/toml/toml.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>
#include <array>
#include <variant>
#include <tuple>
#include <unordered_map>
#include <ctime>

#ifndef TOMLPLUSPLUS_HPP
#    include <toml++/toml.h>
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

namespace cppxx::toml::marzer_toml {
    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<::toml::node, To>;

    template <typename T>
    void parse(const std::string &str, T &val);

    template <typename T>
    void parse_from_file(const std::string &path, T &val);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &str);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse_from_file(const std::string &path);
} // namespace cppxx::toml::marzer_toml


namespace cppxx::serde {
    template <>
    struct Dump<::toml::table, std::string> {
        template <typename T>
        std::string from(const T &v) const {
            auto val = Serialize<::toml::node, T>{}.from(v);

            if (::toml::table *tbl = val.as_table()) {
                std::ostringstream oss;
                oss << *tbl;
                return oss.str();
            } else
                throw error::type_mismatch_error("tbl", std::string(::toml::impl::node_type_friendly_names[(int)val.type()]));
        }
    };

    template <>
    struct Parse<::toml::table, std::string> {
        const std::string &src;

        template <typename T>
        void into(T &val, bool src_is_path = false) const {
            ::toml::table tbl;
            try {
                tbl = src_is_path ? ::toml::parse_file(src) : ::toml::parse(src);
            } catch (std::exception &e) {
                throw error(e.what());
            }

            ::toml::node *node = &tbl;
            Deserialize<::toml::node, T>{node}.into(val);
        }
    };

    // bool
    template <>
    struct Serialize<::toml::node, bool> {
        std::unique_ptr<::toml::node> from(bool v) const {
            return std::make_unique<::toml::value<bool>>(v);
        }
    };

    template <>
    struct Deserialize<::toml::node, bool> {
        const ::toml::node *node;

        void into(bool &v) const {
            if (auto val = node->as_boolean())
                v = val->get();
            else
                throw error::type_mismatch_error("bool", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));
        }
    };

    // int
    template <typename T>
    struct Serialize<::toml::node, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
        std::unique_ptr<::toml::node> from(T v) const {
            return std::make_unique<::toml::value<int64_t>>(v);
        }
    };

    template <typename T>
    struct Deserialize<::toml::node, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
        const ::toml::node *node;

        void into(T &v) const {
            if (auto val = node->as_integer())
                v = (T)val->get();
            else
                throw error::type_mismatch_error("int", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));
        }
    };

    // float
    template <typename T>
    struct Serialize<::toml::node, T, std::enable_if_t<std::is_floating_point_v<T>>> {
        std::unique_ptr<::toml::node> from(T v) const {
            return std::make_unique<::toml::value<double>>(v);
        }
    };

    template <typename T>
    struct Deserialize<::toml::node, T, std::enable_if_t<std::is_floating_point_v<T>>> {
        const ::toml::node *node;

        void into(T &v) const {
            if (auto val = node->as_floating_point())
                v = (T)val->get();
            else
                throw error::type_mismatch_error("float", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));
        }
    };

    // string
    template <>
    struct Serialize<::toml::node, std::string_view> {
        std::unique_ptr<::toml::node> from(std::string_view v) const {
            return std::make_unique<::toml::value<std::string>>(std::string(v));
        }

        std::unique_ptr<::toml::node> from_raw(std::string_view v) const {
            ::toml::table tbl;
            try {
                tbl = ::toml::parse(v);
            } catch (std::exception &e) {
                throw error(e.what());
            }
            return std::make_unique<::toml::table>(std::move(tbl));
        }
    };

    template <>
    struct Serialize<::toml::node, std::string> {
        std::unique_ptr<::toml::node> from(const std::string &v) const {
            return std::make_unique<::toml::value<std::string>>(v);
        }

        std::unique_ptr<::toml::node> from_raw(const std::string &v) const {
            return Serialize<::toml::node, std::string_view>{}.from_raw(v);
        }
    };

    template <>
    struct Deserialize<::toml::node, std::string> {
        const ::toml::node *node;

        void into(std::string &v) const {
            if (auto val = node->as_string())
                v = val->get();
            else
                throw error::type_mismatch_error(
                    "string", std::string(::toml::impl::node_type_friendly_names[(int)node->type()])
                );
        }

        void into_raw(std::string &v) const {
            if (auto tbl = node->as_table()) {
                std::ostringstream oss;
                oss << *tbl;
                v = oss.str();
            } else
                throw error::type_mismatch_error("table", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));
        }
    };

    // optional
    template <typename T>
    struct Serialize<::toml::node, std::optional<T>, std::enable_if_t<std::is_default_constructible_v<T>>> {
        std::unique_ptr<::toml::node> from(const std::optional<T> &v) const {
            Serialize<::toml::node, T> ser = {};
            if (v.has_value())
                return ser.from(*v);
            else
                return ser.from(T{});
        }
    };

    template <typename T>
    struct Deserialize<::toml::node, std::optional<T>, std::enable_if_t<std::is_default_constructible_v<T>>> {
        const ::toml::node *node;

        void into(std::optional<T> &v) const {
            if (!node) {
                v = std::nullopt;
                return;
            }
            v = T{};
            Deserialize<::toml::node, T>{node}.into(*v);
        }
    };

    // array
    template <typename T, size_t N>
    struct Serialize<::toml::node, std::array<T, N>> {
        std::unique_ptr<::toml::node> from(const std::array<T, N> &v) const {
            auto arr = std::make_unique<::toml::array>();
            for (auto &item : v)
                arr->push_back(std::move(*Serialize<::toml::node, T>{}.from(item)));
            return arr;
        }
    };

    template <typename T, size_t N>
    struct Deserialize<::toml::node, std::array<T, N>> {
        const ::toml::node *node;

        void into(std::array<T, N> &v) const {
            auto arr = node->as_array();
            if (!arr)
                throw error::type_mismatch_error("array", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));

            const size_t n = arr->size();
            if (n != N)
                throw error::size_mismatch_error(N, n);

            for (size_t i = 0; i < n; ++i)
                try {
                    Deserialize<::toml::node, T>{arr->get(i)}.into(v[i]);
                } catch (error &e) {
                    throw e.add_context(i);
                }
        };
    };

    template <typename T>
    struct Serialize<::toml::node, std::vector<T>> {
        std::unique_ptr<::toml::node> from(const std::vector<T> &v) const {
            auto arr = std::make_unique<::toml::array>();
            for (auto &item : v)
                arr->push_back(std::move(*Serialize<::toml::node, T>{}.from(item)));
            return arr;
        }
    };

    template <typename T>
    struct Deserialize<::toml::node, std::vector<T>, std::enable_if_t<std::is_default_constructible_v<T>>> {
        const ::toml::node *node;

        void into(std::vector<T> &v) const {
            auto arr = node->as_array();
            if (!arr)
                throw error::type_mismatch_error("array", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));

            const size_t n = arr->size();
            v.resize(n);
            for (size_t i = 0; i < n; ++i)
                try {
                    Deserialize<::toml::node, T>{arr->get(i)}.into(v[i]);
                } catch (error &e) {
                    throw e.add_context(i);
                }
        }
    };

    template <typename... Ts>
    struct Serialize<::toml::node, std::tuple<Ts...>> {
        std::unique_ptr<::toml::node> from(const std::tuple<Ts...> &tpl) {
            const TagInfoTuple<sizeof...(Ts)>         ti     = cppxx::toml::get_tag_info_from_tuple(tpl);
            const std::array<TagInfo, sizeof...(Ts)> &ts     = ti.ts;
            const bool                                is_obj = ti.is_obj;

            std::unique_ptr<::toml::node> node = is_obj ? std::make_unique<::toml::table>() : std::make_unique<::toml::node>();
            tuple_for_each(tpl, [&](const auto &item, const size_t i) {
                const TagInfo &t = ts[i];
                const auto    &v = [&]() -> decltype(auto) {
                    if constexpr (is_tagged_v<std::decay_t<decltype(item)>>) {
                        return item.get_value();
                    } else {
                        return item;
                    }
                }();
                using T = std::decay_t<decltype(v)>;

                if (is_obj && t.key == "")
                    return;

                // if (t.omitempty && cppxx::json::detail::is_empty_value(v))
                //     return;

                std::unique_ptr<::toml::node> val;
                try {
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            val = Serialize<::toml::node, std::string>{}.from_raw(v);
                        else
                            throw error("field with tag `noserde` can only be serialized from std::string");
                    else
                        val = Serialize<::toml::node, T>{}.from(v);
                } catch (error &e) {
                    if (is_obj)
                        throw e.add_context(t.key);
                    else
                        throw e.add_context(i);
                }
                if (is_obj)
                    node->as_table()->insert_or_assign(t.key, std::move(*val));
                else
                    node->as_array()->push_back(std::move(*val));
            });

            return node;
        }
    };

    template <typename... Ts>
    struct Deserialize<::toml::node, std::tuple<Ts...>> {
        const ::toml::node *node;

        void into(std::tuple<Ts...> &tpl) const {
            const TagInfoTuple<sizeof...(Ts)>         ti     = cppxx::toml::get_tag_info_from_tuple(tpl);
            const std::array<TagInfo, sizeof...(Ts)> &ts     = ti.ts;
            const bool                                is_obj = ti.is_obj;

            auto arr = node->as_array();
            auto tbl = node->as_table();
            if (!is_obj && !arr)
                throw error::type_mismatch_error("array", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));
            if (is_obj && !tbl)
                throw error::type_mismatch_error("table", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));

            tuple_for_each(tpl, [&](auto &item, const size_t i) {
                const TagInfo &t = ts[i];
                auto          &v = [&]() -> decltype(auto) {
                    if constexpr (is_tagged_v<std::decay_t<decltype(item)>>) {
                        return item.get_value();
                    } else {
                        return item;
                    }
                }();
                using T = std::decay_t<decltype(v)>;

                if (is_obj && t.key == "")
                    return;

                const ::toml::node *val = is_obj ? tbl->get(t.key) : arr->get(i);
                if (!val && t.skipmissing)
                    return;

                try {
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            Deserialize<::toml::node, std::string>{val}.into_raw(v);
                        else
                            throw error("field with tag `noserde` can only be deserialized into std::string");
                    else
                        Deserialize<::toml::node, T>{val}.into(v);
                } catch (error &e) {
                    if (is_obj)
                        throw e.add_context(t.key);
                    else
                        throw e.add_context(i);
                }
            });
        }
    };

    template <typename... T>
    struct Serialize<::toml::node, std::variant<T...>> {
        std::unique_ptr<::toml::node> from(const std::variant<T...> &v) const {
            return std::visit(
                [](const auto &var) { return Serialize<::toml::node, std::decay_t<decltype(var)>>{}.from(var); }, v
            );
        }
    };

    template <typename... T>
    struct Deserialize<::toml::node, std::variant<T...>, std::enable_if_t<(std::is_default_constructible_v<T> && ...)>> {
        const ::toml::node *node;

        void into(std::variant<T...> &v) const {
            try_for_each(v, std::index_sequence_for<T...>{});
        }

    protected:
        template <size_t... I>
        void try_for_each(std::variant<T...> &v, std::index_sequence<I...>) const {
            bool done = false;
            (
                [&]() {
                    using Elem = std::tuple_element_t<I, std::tuple<T...>>;
                    try {
                        if (!done) {
                            auto element = Elem{};
                            Deserialize<::toml::node, Elem>{node}.into(element);
                            v    = std::move(element);
                            done = true;
                        }
                    } catch (error &e) {
                        std::ignore = e;
                    }
                }(),
                ...
            );
            if (!done)
                throw error::type_mismatch_error(
                    "variant", std::string(::toml::impl::node_type_friendly_names[(int)node->type()])
                );
        }
    };

    // table
    template <typename T>
    struct Serialize<::toml::node, std::unordered_map<std::string, T>> {
        std::unique_ptr<::toml::node> from(const std::unordered_map<std::string, T> &v) const {
            std::unique_ptr<::toml::node> node = std::make_unique<::toml::table>();
            for (auto &[key, item] : v)
                node->as_table()->insert_or_assign(key, std::move(*Serialize<::toml::node, T>{}.from(item)));
            return node;
        }
    };

    template <typename T>
    struct Deserialize<::toml::node, std::unordered_map<std::string, T>, std::enable_if_t<std::is_default_constructible_v<T>>> {
        const ::toml::node *node;

        void into(std::unordered_map<std::string, T> &v) const {
            auto table = node->as_table();
            if (!table)
                throw error::type_mismatch_error("table", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));

            for (auto [key, node] : *table) {
                auto item = T{};
                try {
                    Deserialize<::toml::node, T>{&node}.into(item);
                } catch (error &e) {
                    throw e.add_context(std::string_view(key));
                }
                v.emplace(std::string(key), std::move(item));
            }
        }
    };

    // std::tm
    template <>
    struct Serialize<::toml::node, std::tm> {
        std::unique_ptr<::toml::node> from(const std::tm &tm) const {
            ::toml::date_time dt;

            dt.date.year  = tm.tm_year + 1900;
            dt.date.month = tm.tm_mon + 1;
            dt.date.day   = tm.tm_mday;

            dt.time.hour   = tm.tm_hour;
            dt.time.minute = tm.tm_min;
            dt.time.second = tm.tm_sec;

            return std::make_unique<::toml::value<::toml::date_time>>(dt);
        }
    };

    template <>
    struct Deserialize<::toml::node, std::tm> {
        const ::toml::node *node;

        void into(std::tm &v) const {
            if (auto val = node->as_time())
                to_tm(val->get(), v);
            else if (auto val = node->as_date())
                to_tm(val->get(), v);
            else if (auto val = node->as_date_time()) {
                to_tm(val->get().time, v);
                to_tm(val->get().date, v);
            } else
                throw error::type_mismatch_error("time", std::string(::toml::impl::node_type_friendly_names[(int)node->type()]));
        }

        static void to_tm(const ::toml::date &d, std::tm &tm) {
            tm.tm_year = d.year - 1900; // tm_year is years since 1900
            tm.tm_mon  = d.month - 1;   // tm_mon is 0–11
            tm.tm_mday = d.day;
        }

        static void to_tm(const ::toml::time &t, std::tm &tm) {
            tm.tm_hour = t.hour;
            tm.tm_min  = t.minute;
            tm.tm_sec  = t.second;
        }
    };

#ifdef BOOST_PFR_HPP
    template <typename S>
    struct Serialize<::toml::node, S, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
        std::unique_ptr<::toml::node> from(const S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            return Serialize<::toml::node, decltype(tpl)>{}.from(tpl);
        }
    };

    template <typename S>
    struct Deserialize<::toml::node, S, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
        const ::toml::node *node;

        void into(S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            Deserialize<::toml::node, decltype(tpl)>{node}.into(tpl);
        }
    };
#endif

#ifdef NEARGYE_MAGIC_ENUM_HPP
    // enum
    template <typename S>
    struct Serialize<::toml::node, S, std::enable_if_t<std::is_enum_v<S>>> {
        std::unique_ptr<::toml::node> from(const S &v) const {
            return Serialize<::toml::node, std::string_view>{}.from(magic_enum::enum_name(v));
        }
    };

    template <typename S>
    struct Deserialize<::toml::node, S, std::enable_if_t<std::is_enum_v<S>>> {
        const ::toml::node *node;

        void into(S &v) const {
            auto str = std::string();
            Deserialize<::toml::node, std::string>{node}.into(str);

            auto e = magic_enum::enum_cast<S>(str);
            if (!e.has_value()) {
                std::string what = "invalid value `" + str + "`, expected one of {";
                for (std::string_view name : magic_enum::enum_names<S>()) {
                    what += std::string(name) + ",";
                }
                what += "}";
                throw error(std::move(what));
            }
            v = *e;
        }
    };
#endif
} // namespace cppxx::serde

namespace cppxx::toml::marzer_toml {
    template <typename T>
    void parse(const std::string &str, T &val) {
        cppxx::serde::Parse<::toml::table, std::string>{str}.into(val);
    }

    template <typename T>
    void parse_from_file(const std::string &path, T &val) {
        cppxx::serde::Parse<::toml::table, std::string>{path}.into(val, true);
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &str) {
        T val = {};
        cppxx::serde::Parse<::toml::table, std::string>{str}.into(val);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse_from_file(const std::string &path) {
        T val = {};
        cppxx::serde::Parse<::toml::table, std::string>{path}.into(val, true);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val) {
        return cppxx::serde::Dump<::toml::table, std::string>{}.from(val);
    }
} // namespace cppxx::toml::marzer_toml
#endif
