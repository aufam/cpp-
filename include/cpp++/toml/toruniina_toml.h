#ifndef CPPXX_TOML_TORUNIINA_TOML_H
#define CPPXX_TOML_TORUNIINA_TOML_H

#include <cpp++/toml/toml.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>
#include <array>
#include <variant>
#include <tuple>
#include <unordered_map>
#include <ctime>

#ifndef TOML11_TOML_HPP
#    include <toml.hpp>
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

namespace __toml11 = ::toml;

namespace cppxx::toml::toruniina_toml {
    using spec = __toml11::spec;

    template <typename From>
    using Serialize = cppxx::serde::Serialize<__toml11::value, From>;

    template <typename To>
    using Deserialize = cppxx::serde::Deserialize<__toml11::value, To>;

    using Parse = cppxx::serde::Parse<__toml11::table, std::string>;
    using Dump  = cppxx::serde::Dump<__toml11::table, std::string>;

    template <typename T>
    void parse(const std::string &str, T &val, const spec &s = spec::default_version());

    template <typename T>
    void parse_from_file(const std::string &path, T &val, const spec &s = spec::default_version());

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T>
    parse(const std::string &str, const spec &s = spec::default_version());

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T>
    parse_from_file(const std::string &path, const spec &s = spec::default_version());

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val, const spec &s = spec::default_version());
} // namespace cppxx::toml::toruniina_toml

namespace cppxx::toml::toruniina_toml::detail {
    inline std::string type(const __toml11::value &val) {
        switch (val.type()) {
        case __toml11::value_t::empty:
            return "null";
        case __toml11::value_t::boolean:
            return "boolean";
        case __toml11::value_t::integer:
            return "integer";
        case __toml11::value_t::floating:
            return "floating";
        case __toml11::value_t::string:
            return "string";
        case __toml11::value_t::offset_datetime:
            return "offset_datetime";
        case __toml11::value_t::local_datetime:
            return "local_datetime";
        case __toml11::value_t::local_date:
            return "local_date";
        case __toml11::value_t::local_time:
            return "local_time";
        case __toml11::value_t::array:
            return "array";
        case __toml11::value_t::table:
            return "table";
        }
        return "unknown";
    }
} // namespace cppxx::toml::toruniina_toml::detail

namespace cppxx::serde {
    template <>
    struct Dump<__toml11::table, std::string> {
        ::cppxx::toml::toruniina_toml::spec spec = ::cppxx::toml::toruniina_toml::spec::default_version();

        template <typename T>
        std::string from(const T &v) const {
            __toml11::value val = Serialize<__toml11::value, T>{}.from(v);

            if (val.is_table()) {
                std::ostringstream oss;
                oss << __toml11::format(val, spec);
                return oss.str();
            } else
                throw type_mismatch_error("table", std::string(::cppxx::toml::toruniina_toml::detail::type(val)));
        }
    };

    template <>
    struct Parse<__toml11::table, std::string> {
        const std::string                  &src;
        ::cppxx::toml::toruniina_toml::spec spec = ::cppxx::toml::toruniina_toml::spec::default_version();

        template <typename T>
        void into(T &val, bool src_is_path = false) const {
            __toml11::value    tbl;
            std::istringstream iss(src);

            try {
                try {
                    tbl = src_is_path ? __toml11::parse(src, spec) : __toml11::parse(iss, "<unknown>", spec);
                } catch (std::exception &e) {
                    throw error(e.what());
                }
                Deserialize<__toml11::value, T>{tbl}.into(val);
            } catch (error &err) {
                if (src_is_path)
                    err.path = src;
                throw;
            }
        }
    };

    // bool
    template <>
    struct Serialize<__toml11::value, bool> {
        __toml11::value from(bool v) const {
            return {__toml11::value::boolean_type(v)};
        }
    };

    template <>
    struct Deserialize<__toml11::value, bool> {
        const __toml11::value &node;

        void into(bool &v) const {
            if (node.is_boolean())
                v = node.as_boolean();
            else
                throw type_mismatch_error("bool", ::cppxx::toml::toruniina_toml::detail::type(node));
        }
    };

    // int
    template <typename T>
    struct Serialize<__toml11::value, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
        __toml11::value from(T v) const {
            return __toml11::value(__toml11::value::integer_type(v));
        }
    };

    template <typename T>
    struct Deserialize<__toml11::value, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
        const __toml11::value &node;

        void into(T &v) const {
            if (node.is_integer())
                v = (T)node.as_integer();
            else
                throw type_mismatch_error("int", ::cppxx::toml::toruniina_toml::detail::type(node));
        }
    };

    // float
    template <typename T>
    struct Serialize<__toml11::value, T, std::enable_if_t<std::is_floating_point_v<T>>> {
        __toml11::value from(T v) const {
            return __toml11::value(__toml11::value::floating_type(v));
        }
    };

    template <typename T>
    struct Deserialize<__toml11::value, T, std::enable_if_t<std::is_floating_point_v<T>>> {
        const __toml11::value &node;

        void into(T &v) const {
            if (node.is_floating())
                v = (T)node.as_floating();
            else
                throw type_mismatch_error("float", ::cppxx::toml::toruniina_toml::detail::type(node));
        }
    };

    // string
    template <>
    struct Serialize<__toml11::value, std::string> {
        __toml11::value from(const std::string &v) const {
            return {v};
        }

        __toml11::value from_raw(const std::string &v) const {
            std::istringstream iss(v);
            try {
                return __toml11::parse(iss);
            } catch (std::exception &e) {
                throw error(e.what());
            }
        }
    };

    template <>
    struct Serialize<__toml11::value, std::string_view> {
        __toml11::value from(std::string_view v) const {
            return {__toml11::value::string_type(v)};
        }

        __toml11::value from_raw(std::string_view v) const {
            return Serialize<__toml11::value, std::string>{}.from_raw(std::string(v));
        }
    };

    template <>
    struct Deserialize<__toml11::value, std::string> {
        const __toml11::value &node;

        void into(std::string &v) const {
            if (node.is_string())
                v = node.as_string();
            else
                throw type_mismatch_error("string", ::cppxx::toml::toruniina_toml::detail::type(node));
        }

        void into_raw(std::string &v) const {
            if (node.is_table()) {
                std::ostringstream oss;
                oss << __toml11::format(node);
                v = oss.str();
            } else
                throw type_mismatch_error("table", ::cppxx::toml::toruniina_toml::detail::type(node));
        }
    };

    // optional
    template <typename T>
    struct Serialize<
        __toml11::value,
        std::optional<T>,
        std::enable_if_t<std::is_default_constructible_v<T> && is_serializable_v<__toml11::value, T>>> {
        __toml11::value from(const std::optional<T> &v) const {
            Serialize<__toml11::value, T> ser = {};
            if (v.has_value())
                return ser.from(*v);
            else
                return ser.from(T{});
        }
    };

    template <typename T>
    struct Deserialize<
        __toml11::value,
        std::optional<T>,
        std::enable_if_t<std::is_default_constructible_v<T> && is_deserializable_v<__toml11::value, T>>> {
        const __toml11::value &node;

        void into(std::optional<T> &v) const {
            if (node.is_empty()) {
                v = std::nullopt;
                return;
            }
            v = T{};
            Deserialize<__toml11::value, T>{node}.into(*v);
        }
    };

    // array
    template <typename T, size_t N>
    struct Serialize<__toml11::value, std::array<T, N>, std::enable_if_t<is_serializable_v<__toml11::value, T>>> {
        __toml11::value from(const std::array<T, N> &v) const {
            __toml11::value arr = __toml11::array();
            for (auto &item : v)
                arr.push_back(Serialize<__toml11::value, T>{}.from(item));
            return arr;
        }
    };

    template <typename T, size_t N>
    struct Deserialize<__toml11::value, std::array<T, N>, std::enable_if_t<is_deserializable_v<__toml11::value, T>>> {
        const __toml11::value &node;

        void into(std::array<T, N> &v) const {
            if (!node.is_array())
                throw type_mismatch_error("array", ::cppxx::toml::toruniina_toml::detail::type(node));

            const auto  &arr = node.as_array();
            const size_t n   = arr.size();
            if (n != N)
                throw size_mismatch_error(N, n);

            for (size_t i = 0; i < n; ++i)
                try {
                    Deserialize<__toml11::value, T>{arr[i]}.into(v[i]);
                } catch (error &e) {
                    e.add_context(i);
                    throw;
                }
        };
    };

    template <typename T>
    struct Serialize<__toml11::value, std::vector<T>, std::enable_if_t<is_serializable_v<__toml11::value, T>>> {
        __toml11::value from(const std::vector<T> &v) const {
            __toml11::value arr = __toml11::array();
            for (auto &item : v)
                arr.push_back(Serialize<__toml11::value, T>{}.from(item));
            return arr;
        }
    };

    template <typename T>
    struct Deserialize<
        __toml11::value,
        std::vector<T>,
        std::enable_if_t<std::is_default_constructible_v<T> && is_deserializable_v<__toml11::value, T>>> {
        const __toml11::value &node;

        void into(std::vector<T> &v) const {
            if (!node.is_array())
                throw type_mismatch_error("array", ::cppxx::toml::toruniina_toml::detail::type(node));

            const auto  &arr = node.as_array();
            const size_t n   = arr.size();
            v.resize(n);
            for (size_t i = 0; i < n; ++i)
                try {
                    Deserialize<__toml11::value, T>{arr[i]}.into(v[i]);
                } catch (error &e) {
                    e.add_context(i);
                    throw;
                }
        }
    };

    template <typename... Ts>
    struct Serialize<__toml11::value, std::tuple<Ts...>> {
        __toml11::value from(const std::tuple<Ts...> &tpl) {
            const TagInfoTuple<sizeof...(Ts)>         ti     = cppxx::toml::get_tag_info_from_tuple(tpl);
            const std::array<TagInfo, sizeof...(Ts)> &ts     = ti.ts;
            const bool                                is_obj = ti.is_obj;

            __toml11::value node = is_obj ? __toml11::value(__toml11::table()) : __toml11::value(__toml11::array());

            tuple_for_each(tpl, [&](const auto &item, const size_t i) {
                const TagInfo &t = ts[i];
                const auto    &v = detail::get_underlying_value(item);
                using T          = std::decay_t<decltype(v)>;

                if (!is_serializable_v<__toml11::value, T> || (is_obj && t.key == "") ||
                    (t.omitempty && detail::is_empty_value(v)))
                    return;

                __toml11::value val;
                try {
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            val = Serialize<__toml11::value, std::string>{}.from_raw(v);
                        else
                            throw error("field with tag `noserde` can only be serialized from std::string");
                    else {
                        if constexpr (is_serializable_v<__toml11::value, T>)
                            val = Serialize<__toml11::value, T>{}.from(v);
                    }
                } catch (error &e) {
                    if (is_obj)
                        e.add_context(t.key);
                    else
                        e.add_context(i);
                    throw;
                }
                if (is_obj)
                    node.as_table(std::nothrow)[std::string(t.key)] = std::move(val);
                else
                    node.as_array(std::nothrow).push_back(std::move(val));
            });

            return node;
        }
    };

    template <typename... Ts>
    struct Deserialize<__toml11::value, std::tuple<Ts...>> {
        const __toml11::value &node;

        void into(std::tuple<Ts...> &tpl) const {
            const TagInfoTuple<sizeof...(Ts)>         ti     = cppxx::toml::get_tag_info_from_tuple(tpl);
            const std::array<TagInfo, sizeof...(Ts)> &ts     = ti.ts;
            const bool                                is_obj = ti.is_obj;

            if (!is_obj && !node.is_array())
                throw type_mismatch_error("array", ::cppxx::toml::toruniina_toml::detail::type(node));
            if (is_obj && !node.is_table())
                throw type_mismatch_error("table", ::cppxx::toml::toruniina_toml::detail::type(node));
            const __toml11::array *arr = &node.as_array(std::nothrow);
            const __toml11::table *tbl = &node.as_table(std::nothrow);

            tuple_for_each(tpl, [&](auto &item, const size_t i) {
                const TagInfo &t = ts[i];
                auto          &v = detail::get_underlying_value(item);
                using T          = std::decay_t<decltype(v)>;

                if (is_obj && t.key == "")
                    return;

                const __toml11::value  empty;
                const __toml11::value *val = &empty;
                if (is_obj) {
                    if (auto it = tbl->find(std::string(t.key)); it != tbl->end())
                        val = &it->second;
                } else {
                    if (i < arr->size())
                        val = &(*arr)[i];
                }
                if (val->is_empty() && t.skipmissing)
                    return;

                try {
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            Deserialize<__toml11::value, std::string>{*val}.into_raw(v);
                        else
                            throw error("field with tag `noserde` can only be deserialized into std::string");
                    else {
                        if constexpr (is_deserializable_v<__toml11::value, T>)
                            Deserialize<__toml11::value, T>{*val}.into(v);
                    }
                } catch (error &e) {
                    if (is_obj)
                        e.add_context(t.key);
                    else
                        e.add_context(i);
                    throw;
                }
            });
        }
    };

    template <typename... T>
    struct Serialize<__toml11::value, std::variant<T...>> {
        __toml11::value from(const std::variant<T...> &v) const {
            return std::visit(
                [](const auto &var) { return Serialize<__toml11::value, std::decay_t<decltype(var)>>{}.from(var); }, v
            );
        }
    };

    template <typename... T>
    struct Deserialize<__toml11::value, std::variant<T...>, std::enable_if_t<(std::is_default_constructible_v<T> && ...)>> {
        const __toml11::value &node;

        void into(std::variant<T...> &v) const {
            bool        done = false;
            std::string type_names;
            (
                [&]() {
                    try {
                        if (!done) {
                            auto element = T{};
                            Deserialize<__toml11::value, T>{node}.into(element);
                            v    = std::move(element);
                            done = true;
                        }
                    } catch (type_mismatch_error &e) {
                        type_names += e.expected_type + '|';
                    }
                }(),
                ...
            );
            if (!done) {
                type_names.pop_back();
                throw type_mismatch_error(type_names, ::cppxx::toml::toruniina_toml::detail::type(node));
            }
        }
    };

    // table
    template <typename T>
    struct Serialize<
        __toml11::value,
        std::unordered_map<std::string, T>,
        std::enable_if_t<is_serializable_v<__toml11::value, T>>> {
        __toml11::value from(const std::unordered_map<std::string, T> &v) const {
            __toml11::value node = __toml11::table();
            for (auto &[key, item] : v)
                node.as_table()[key] = Serialize<__toml11::value, T>{}.from(item);
            return node;
        }
    };

    template <typename T>
    struct Deserialize<
        __toml11::value,
        std::unordered_map<std::string, T>,
        std::enable_if_t<std::is_default_constructible_v<T> && is_deserializable_v<__toml11::value, T>>> {
        const __toml11::value &node;

        void into(std::unordered_map<std::string, T> &v) const {
            if (!node.is_table())
                throw type_mismatch_error("table", ::cppxx::toml::toruniina_toml::detail::type(node));

            for (auto &[key, node] : node.as_table(std::nothrow)) {
                auto item = T{};
                try {
                    Deserialize<__toml11::value, T>{node}.into(item);
                } catch (error &e) {
                    e.add_context(std::string_view(key));
                    throw;
                }
                v.emplace(std::string(key), std::move(item));
            }
        }
    };

    // std::tm
    template <>
    struct Serialize<__toml11::value, std::tm> {
        __toml11::value from(const std::tm &tm) const {
            __toml11::local_datetime dt;

            dt.date.year  = (int16_t)(tm.tm_year + 1900);
            dt.date.month = tm.tm_mon + 1;
            dt.date.day   = tm.tm_mday;

            dt.time.hour   = tm.tm_hour;
            dt.time.minute = tm.tm_min;
            dt.time.second = tm.tm_sec;

            return dt;
        }
    };

    template <>
    struct Deserialize<__toml11::value, std::tm> {
        const __toml11::value &node;

        void into(std::tm &v) const {
            if (auto val = &node.as_local_time(std::nothrow); node.is_local_time())
                to_tm(*val, v);
            else if (auto val = &node.as_local_date(std::nothrow); node.is_local_date())
                to_tm(*val, v);
            else if (auto val = &node.as_local_datetime(std::nothrow); node.is_local_datetime()) {
                to_tm(val->time, v);
                to_tm(val->date, v);
            } else
                throw type_mismatch_error("time", ::cppxx::toml::toruniina_toml::detail::type(node));
        }

        static void to_tm(const __toml11::local_date &d, std::tm &tm) {
            tm.tm_year = d.year - 1900; // tm_year is years since 1900
            tm.tm_mon  = d.month - 1;   // tm_mon is 0–11
            tm.tm_mday = d.day;
        }

        static void to_tm(const __toml11::local_time &t, std::tm &tm) {
            tm.tm_hour = t.hour;
            tm.tm_min  = t.minute;
            tm.tm_sec  = t.second;
        }
    };

#ifdef BOOST_PFR_HPP
    template <typename S>
    struct Serialize<__toml11::value, S, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
        __toml11::value from(const S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            return Serialize<__toml11::value, decltype(tpl)>{}.from(tpl);
        }
    };

    template <typename S>
    struct Deserialize<__toml11::value, S, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
        const __toml11::value &node;

        void into(S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            Deserialize<__toml11::value, decltype(tpl)>{node}.into(tpl);
        }
    };
#endif

#ifdef NEARGYE_MAGIC_ENUM_HPP
    // enum
    template <typename S>
    struct Serialize<__toml11::value, S, std::enable_if_t<std::is_enum_v<S>>> {
        __toml11::value from(const S &v) const {
            return Serialize<__toml11::value, std::string_view>{}.from(magic_enum::enum_name(v));
        }
    };

    template <typename S>
    struct Deserialize<__toml11::value, S, std::enable_if_t<std::is_enum_v<S>>> {
        const __toml11::value &node;

        void into(S &v) const {
            auto str = std::string();
            Deserialize<__toml11::value, std::string>{node}.into(str);

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

namespace cppxx::toml::toruniina_toml {
    template <typename T>
    void parse(const std::string &str, T &val, const spec &spec) {
        cppxx::toml::toruniina_toml::Parse{str, spec}.into(val, false);
    }

    template <typename T>
    void parse_from_file(const std::string &path, T &val, const spec &spec) {
        cppxx::toml::toruniina_toml::Parse{path, spec}.into(val, true);
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &str, const spec &spec) {
        T val = {};
        cppxx::toml::toruniina_toml::Parse{str, spec}.into(val, false);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse_from_file(const std::string &path, const spec &spec) {
        T val = {};
        cppxx::toml::toruniina_toml::Parse{path, spec}.into(val, true);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val, const spec &spec) {
        return cppxx::toml::toruniina_toml::Dump{spec}.from(val);
    }
} // namespace cppxx::toml::toruniina_toml
#endif
