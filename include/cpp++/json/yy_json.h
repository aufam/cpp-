#ifndef CPPXX_JSON_YYJSON_H
#define CPPXX_JSON_YYJSON_H

#include <cpp++/json/json.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>
#include <cpp++/defer.h>
#include <array>
#include <variant>
#include <tuple>
#include <unordered_map>
#include <ctime>

#ifndef YYJSON_H
#    include <yyjson.h>
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

namespace cppxx::json::yy_json {
    template <typename From>
    using Serialize = ::cppxx::serde::Serialize<yyjson_mut_val, From>;

    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<yyjson_val, To>;

    using Dump = ::cppxx::serde::Dump<yyjson_mut_doc, std::string>;

    using Parse = ::cppxx::serde::Parse<yyjson_doc, std::string>;

    template <typename T>
    void parse(const std::string &str, T &val, yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    void parse_from_file(const std::string &path, T &val, yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &str, yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T>
    parse_from_file(const std::string &path, yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val, yyjson_write_flag = YYJSON_WRITE_NOFLAG);
} // namespace cppxx::json::yy_json


namespace cppxx::serde {
    template <>
    struct Parse<yyjson_doc, std::string> {
        const std::string &src;
        yyjson_read_flag   flag = YYJSON_READ_NOFLAG;

        template <typename T>
        void into(T &val, bool src_is_path = false) const {
            yyjson_read_err err;
            yyjson_doc     *doc = src_is_path ? yyjson_read_file(const_cast<char *>(src.c_str()), flag, nullptr, &err)
                                              : yyjson_read_opts(const_cast<char *>(src.c_str()), src.size(), flag, nullptr, &err);
            if (!doc)
                throw error(err.msg);

            auto _ = defer([&] { yyjson_doc_free(doc); });
            Deserialize<yyjson_val, T>{yyjson_doc_get_root(doc)}.into(val);
        }
    };

    template <>
    struct Dump<yyjson_mut_doc, std::string> {
        yyjson_write_flag flag = YYJSON_WRITE_NOFLAG;

        template <typename T>
        std::string from(const T &val) const {
            yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
            if (!doc)
                throw error("Fatal error: cannot create yyjson doc");

            auto _ = defer([&]() { yyjson_mut_doc_free(doc); });
            yyjson_mut_doc_set_root(doc, Serialize<yyjson_mut_val, T>{doc}.from(val));

            size_t           len;
            yyjson_write_err err;
            const char      *str = yyjson_mut_write_opts(doc, flag, nullptr, &len, &err);
            if (!str)
                throw error(err.msg);

            std::string res = {str, len};
            ::free((void *)str);
            return res;
        }
    };

    // bool
    template <>
    struct Serialize<yyjson_mut_val, bool> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(bool v) const {
            return yyjson_mut_bool(doc, v);
        }
    };

    template <>
    struct Deserialize<yyjson_val, bool> {
        yyjson_val *val;

        void into(bool &v) const {
            if (!yyjson_is_bool(val))
                throw error::type_mismatch_error("bool", yyjson_get_type_desc(val));
            v = yyjson_get_bool(val);
        }
    };

    // sint
    template <typename T>
    struct Serialize<
        yyjson_mut_val,
        T,
        std::enable_if_t<std::is_signed_v<T> && !std::is_same_v<T, bool> && !std::is_floating_point_v<T>>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(T v) const {
            return yyjson_mut_sint(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<
        yyjson_val,
        T,
        std::enable_if_t<std::is_signed_v<T> && !std::is_same_v<T, bool> && !std::is_floating_point_v<T>>> {
        yyjson_val *val;

        void into(T &v) const {
            if (!yyjson_is_sint(val) && !yyjson_is_uint(val))
                throw error::type_mismatch_error("sint", yyjson_get_type_desc(val));
            v = (T)yyjson_get_sint(val);
        }
    };

    // uint
    template <typename T>
    struct Serialize<yyjson_mut_val, T, std::enable_if_t<std::is_unsigned_v<T> && !std::is_same_v<T, bool>>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(T v) const {
            return yyjson_mut_uint(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<yyjson_val, T, std::enable_if_t<std::is_unsigned_v<T> && !std::is_same_v<T, bool>>> {
        yyjson_val *val;

        void into(T &v) const {
            if (!yyjson_is_uint(val))
                throw error::type_mismatch_error("uint", yyjson_get_type_desc(val));
            v = (T)yyjson_get_uint(val);
        }
    };

    // float
    template <typename T>
    struct Serialize<yyjson_mut_val, T, std::enable_if_t<std::is_floating_point_v<T>>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(T v) const {
            return yyjson_mut_real(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<yyjson_val, T, std::enable_if_t<std::is_floating_point_v<T>>> {
        yyjson_val *val;

        void into(T &v) const {
            if (!yyjson_is_real(val))
                throw error::type_mismatch_error("real", yyjson_get_type_desc(val));
            v = (T)yyjson_get_real(val);
        }
    };

    // string
    template <>
    struct Serialize<yyjson_mut_val, std::string_view> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(std::string_view v, bool owned = false) const {
            return owned ? yyjson_mut_strncpy(doc, v.data(), v.size()) : yyjson_mut_strn(doc, v.data(), v.size());
        }

        yyjson_mut_val *from_raw(std::string_view v) {
            yyjson_read_err err;
            yyjson_doc     *doc = yyjson_read_opts(const_cast<char *>(v.data()), v.size(), 0, nullptr, &err);

            if (!doc)
                throw error(err.msg);
            auto _ = defer([&]() { yyjson_doc_free(doc); });

            yyjson_val *root = yyjson_doc_get_root(doc);
            return yyjson_val_mut_copy(this->doc, root);
        }
    };

    template <>
    struct Serialize<yyjson_mut_val, std::string> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const std::string &v) const {
            return Serialize<yyjson_mut_val, std::string_view>{doc}.from(v, false);
        }
        yyjson_mut_val *from(std::string &&v) const {
            return Serialize<yyjson_mut_val, std::string_view>{doc}.from(v, true);
        }

        yyjson_mut_val *from_raw(const std::string &v) const {
            return Serialize<yyjson_mut_val, std::string_view>{doc}.from_raw(v);
        }
    };

    template <>
    struct Deserialize<yyjson_val, std::string> {
        yyjson_val *val;

        void into(std::string &v) const {
            if (!yyjson_is_str(val))
                throw error::type_mismatch_error("string", yyjson_get_type_desc(val));
            v = yyjson_get_str(val);
        }

        void into_raw(std::string &v) const {
            yyjson_mut_doc *doc    = yyjson_mut_doc_new(nullptr);
            yyjson_mut_val *copied = yyjson_val_mut_copy(doc, val);
            yyjson_mut_doc_set_root(doc, copied);

            size_t           len;
            yyjson_write_err err;
            const char      *str = yyjson_mut_write_opts(doc, 0, nullptr, &len, &err);
            if (!str)
                throw error(err.msg);

            v = std::string(str, len);
        }
    };

    // optional
    template <typename T>
    struct Serialize<yyjson_mut_val, std::optional<T>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const std::optional<T> &v) const {
            if (!v.has_value())
                return yyjson_mut_null(doc);
            return Serialize<yyjson_mut_val, T>{doc}.from(*v);
        }
    };

    template <typename T>
    struct Deserialize<yyjson_val, std::optional<T>, std::enable_if_t<std::is_default_constructible_v<T>>> {
        yyjson_val *val;

        void into(std::optional<T> &v) const {
            if (!val || yyjson_is_null(val))
                return void(v = std::nullopt);
            v = T{};
            Deserialize<yyjson_val, T>{val}.into(*v);
        }
    };

    // array
    template <typename T, size_t N>
    struct Serialize<yyjson_mut_val, std::array<T, N>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const std::array<T, N> &v) const {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            for (size_t i = 0; i < v.size(); ++i)
                try {
                    yyjson_mut_arr_append(arr, Serialize<yyjson_mut_val, T>{doc}.from(v[i]));
                } catch (error &e) {
                    throw e.add_context(i);
                }
            return arr;
        }
    };

    template <typename T, size_t N>
    struct Deserialize<yyjson_val, std::array<T, N>> {
        yyjson_val *val;

        void into(std::array<T, N> &v) const {
            auto arr = this->val;
            if (!yyjson_is_arr(arr))
                throw error::type_mismatch_error("array", yyjson_get_type_desc(arr));
            if (auto n = yyjson_arr_size(arr); N != n)
                throw error::size_mismatch_error(N, n);

            size_t      idx, max;
            yyjson_val *val;
            yyjson_arr_foreach(arr, idx, max, val) {
                try {
                    Deserialize<yyjson_val, T>{val}.into(v[idx]);
                } catch (error &e) {
                    throw e.add_context(idx);
                }
            }
        }
    };

    // vector
    template <typename T>
    struct Serialize<yyjson_mut_val, std::vector<T>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const std::vector<T> &v) const {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            for (size_t i = 0; i < v.size(); ++i)
                try {
                    yyjson_mut_arr_append(arr, Serialize<yyjson_mut_val, T>{doc}.from(v[i]));
                } catch (error &e) {
                    throw e.add_context(i);
                }
            return arr;
        }
    };

    template <typename T>
    struct Deserialize<yyjson_val, std::vector<T>, std::enable_if_t<std::is_default_constructible_v<T>>> {
        yyjson_val *val;

        void into(std::vector<T> &v) const {
            auto arr = this->val;
            if (!yyjson_is_arr(arr))
                throw error::type_mismatch_error("array", yyjson_get_type_desc(arr));

            v.resize(yyjson_arr_size(arr));
            size_t      idx, max;
            yyjson_val *val;
            yyjson_arr_foreach(arr, idx, max, val) {
                try {
                    Deserialize<yyjson_val, T>{val}.into(v[idx]);
                } catch (error &e) {
                    throw e.add_context(idx);
                }
            }
        }
    };

    // tuple
    template <typename... Ts>
    struct Serialize<yyjson_mut_val, std::tuple<Ts...>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const std::tuple<Ts...> &tpl) const {
            const TagInfoTuple<sizeof...(Ts)>         ti     = cppxx::json::get_tag_info_from_tuple(tpl);
            const std::array<TagInfo, sizeof...(Ts)> &ts     = ti.ts;
            const bool                                is_obj = ti.is_obj;

            yyjson_mut_val *obj_or_arr = is_obj ? yyjson_mut_obj(doc) : yyjson_mut_arr(doc);
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

                if (t.omitempty && cppxx::json::detail::is_empty_value(v))
                    return;

                yyjson_mut_val *val;
                try {
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            val = Serialize<yyjson_mut_val, std::string>{doc}.from_raw(v);
                        else
                            throw error("field with tag `noserde` can only be serialized from std::string");
                    else
                        val = Serialize<yyjson_mut_val, T>{doc}.from(v);
                } catch (error &e) {
                    if (is_obj)
                        throw e.add_context(t.key);
                    else
                        throw e.add_context(i);
                }
                if (is_obj)
                    yyjson_mut_obj_add(obj_or_arr, yyjson_mut_strn(doc, t.key.data(), t.key.size()), val);
                else
                    yyjson_mut_arr_append(obj_or_arr, val);
            });
            return obj_or_arr;
        }
    };

    template <typename... Ts>
    struct Deserialize<yyjson_val, std::tuple<Ts...>> {
        yyjson_val *val;

        void into(std::tuple<Ts...> &tpl) const {
            const TagInfoTuple<sizeof...(Ts)>         ti     = cppxx::json::get_tag_info_from_tuple(tpl);
            const std::array<TagInfo, sizeof...(Ts)> &ts     = ti.ts;
            const bool                                is_obj = ti.is_obj;

            yyjson_val *obj = this->val;
            yyjson_val *arr = this->val;

            if (is_obj && !yyjson_is_obj(obj))
                throw error::type_mismatch_error("object", yyjson_get_type_desc(obj));
            if (!is_obj) {
                if (!yyjson_is_arr(arr))
                    throw error::type_mismatch_error("array", yyjson_get_type_desc(arr));
                else if (auto n = yyjson_arr_size(arr); sizeof...(Ts) != n)
                    throw error::size_mismatch_error(sizeof...(Ts), n);
            }

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

                yyjson_val *val = is_obj ? yyjson_obj_getn(obj, t.key.data(), t.key.size()) : yyjson_arr_get(arr, i);
                if (!val && t.skipmissing)
                    return;

                try {
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            Deserialize<yyjson_val, std::string>{val}.into_raw(v);
                        else
                            throw error("field with tag `noserde` can only be deserialized into std::string");
                    else
                        Deserialize<yyjson_val, T>{val}.into(v);
                } catch (error &e) {
                    if (is_obj)
                        throw e.add_context(t.key);
                    else
                        throw e.add_context(i);
                }
            });
        }
    };

    // variant
    template <typename... T>
    struct Serialize<yyjson_mut_val, std::variant<T...>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const std::variant<T...> &v) const {
            return std::visit(
                [&](const auto &var) { return Serialize<yyjson_mut_val, std::decay_t<decltype(var)>>{doc}.from(var); }, v
            );
        }
    };

    template <typename... T>
    struct Deserialize<yyjson_val, std::variant<T...>, std::enable_if_t<(std::is_default_constructible_v<T> && ...)>> {
        yyjson_val *val;

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
                            Deserialize<yyjson_val, Elem>{val}.into(element);
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
                throw error::type_mismatch_error("variant", yyjson_get_type_desc(val));
        }
    };

    // map
    template <typename T>
    struct Serialize<yyjson_mut_val, std::unordered_map<std::string, T>> {
        yyjson_mut_doc *doc;
        yyjson_mut_val *from(const std::unordered_map<std::string, T> &v) const {
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            for (auto &[k, v] : v) {
                yyjson_mut_val *key = yyjson_mut_strn(doc, k.data(), k.size()), *item;
                try {
                    item = Serialize<yyjson_mut_val, T>{doc}.from(v);
                } catch (error &e) {
                    throw e.add_context(k);
                }
                yyjson_mut_obj_add(obj, key, item);
            }
            return obj;
        }
    };

    template <typename T>
    struct Deserialize<yyjson_val, std::unordered_map<std::string, T>, std::enable_if_t<std::is_default_constructible_v<T>>> {
        yyjson_val *val;

        void into(std::unordered_map<std::string, T> &v) {
            auto obj = this->val;
            if (!yyjson_is_obj(obj))
                throw error::type_mismatch_error("object", yyjson_get_type_desc(obj));
            size_t      idx, max;
            yyjson_val *key, *val;
            yyjson_obj_foreach(obj, idx, max, key, val) {
                std::string key_str;
                Deserialize<yyjson_val, std::string>{key}.into(key_str);
                auto item = T{};
                try {
                    Deserialize<yyjson_val, T>{val}.into(item);
                } catch (error &e) {
                    throw e.add_context(key_str);
                }
                v.emplace(std::move(key_str), std::move(item));
            }
        }
    };

    // std::tm
    template <>
    struct Serialize<yyjson_mut_val, std::tm> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const std::tm &tm) const {
            std::array<char, 64> buf;
            if (auto len = std::strftime(buf.data(), buf.size(), "%Y-%m-%dT%H:%M:%SZ", &tm))
                return Serialize<yyjson_mut_val, std::string>{doc}.from(std::string(buf.data(), len));
            throw error("Cannot serialize std::tm");
        }
    };

    template <>
    struct Deserialize<yyjson_val, std::tm> {
        yyjson_val *val;

        void into(std::tm &tm) {
            std::string buf;
            Deserialize<yyjson_val, std::string>{val}.into(buf);

            if (buf.size() != 20 || buf[4] != '-' || buf[7] != '-' || buf[10] != 'T' || buf[13] != ':' || buf[16] != ':' ||
                buf[19] != 'Z')
                throw error("Invalid datetime format: " + buf);

            try {
                tm.tm_year  = std::stoi(buf.substr(0, 4)) - 1900;
                tm.tm_mon   = std::stoi(buf.substr(5, 2)) - 1;
                tm.tm_mday  = std::stoi(buf.substr(8, 2));
                tm.tm_hour  = std::stoi(buf.substr(11, 2));
                tm.tm_min   = std::stoi(buf.substr(14, 2));
                tm.tm_sec   = std::stoi(buf.substr(17, 2));
                tm.tm_isdst = 0; // UTC
            } catch (...) {
                throw error("Invalid datetime format: " + buf);
            }
        }
    };

#ifdef BOOST_PFR_HPP
    // aggregate struct
    template <typename S>
    struct Serialize<yyjson_mut_val, S, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            return Serialize<yyjson_mut_val, decltype(tpl)>{doc}.from(tpl);
        }
    };

    template <typename S>
    struct Deserialize<yyjson_val, S, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
        yyjson_val *val;

        void into(S &v) {
            auto tpl = boost::pfr::structure_tie(v);
            Deserialize<yyjson_val, decltype(tpl)>{val}.into(tpl);
        }
    };
#endif

#ifdef NEARGYE_MAGIC_ENUM_HPP
    // enum
    template <typename S>
    struct Serialize<yyjson_mut_val, S, std::enable_if_t<std::is_enum_v<S>>> {
        yyjson_mut_doc *doc;

        yyjson_mut_val *from(const S &v) const {
            return Serialize<yyjson_mut_val, std::string_view>{doc}.from(magic_enum::enum_name(v));
        }
    };

    template <typename S>
    struct Deserialize<yyjson_val, S, std::enable_if_t<std::is_enum_v<S>>> {
        yyjson_val *val;

        void into(S &v) {
            std::string str;
            Deserialize<yyjson_val, std::string>{val}.into(str);
            auto e = magic_enum::enum_cast<S>(str);
            if (!e.has_value()) {
                std::string what = "invalid value `" + str + "`, expected one of {";
                for (auto &name : magic_enum::enum_names<S>()) {
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


namespace cppxx::json::yy_json {
    template <typename T>
    void parse(const std::string &str, T &val, yyjson_read_flag flag) {
        cppxx::serde::Parse<yyjson_doc, std::string>{str, flag}.into(val);
    }

    template <typename T>
    void parse_from_file(const std::string &path, T &val, yyjson_read_flag flag) {
        cppxx::serde::Parse<yyjson_doc, std::string>{path, flag}.into(val, true);
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &str, yyjson_read_flag flag) {
        T val = {};
        cppxx::serde::Parse<yyjson_doc, std::string>{str, flag}.into(val);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse_from_file(const std::string &path, yyjson_read_flag flag) {
        T val = {};
        cppxx::serde::Parse<yyjson_doc, std::string>{path, flag}.into(val, true);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val, yyjson_write_flag flag) {
        return cppxx::serde::Dump<yyjson_mut_doc, std::string>{flag}.from(val);
    }
} // namespace cppxx::json::yy_json
#endif
