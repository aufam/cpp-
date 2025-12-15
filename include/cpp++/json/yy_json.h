#ifndef CPPXX_JSON_YYJSON_H
#define CPPXX_JSON_YYJSON_H

#include <cpp++/json/json.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/defer.h>
#include <array>
#include <variant>
#include <tuple>
#include <unordered_map>
#include <ctime>

#ifndef YYJSON_H
#    include <yyjson.h>
#endif

#ifndef NEARGYE_MAGIC_ENUM_HPP
#    include <magic_enum/magic_enum.hpp>
#endif


namespace cppxx::json::yy_json {
    struct Serializer;

    struct Deserializer;

    template <typename From, typename Enable = void>
    using Serialize = ::cppxx::serde::Serialize<Serializer, From, Enable>;

    template <typename To, typename Enable = void>
    using Deserialize = ::cppxx::serde::Deserialize<Deserializer, To, Enable>;

    template <typename T>
    void parse(const std::string &str, T &val, uint32_t yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    void parse_from_file(const std::string &path, T &val, uint32_t yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T>
    parse(const std::string &str, uint32_t yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T>
    parse_from_file(const std::string &path, uint32_t yyjson_read_flag = YYJSON_READ_NOFLAG);

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val, uint32_t yyjson_write_flag = YYJSON_WRITE_NOFLAG);
} // namespace cppxx::json::yy_json


namespace cppxx::json::yy_json {
    class error;

    struct Serializer {
        using type  = yyjson_mut_val *;
        using error = ::cppxx::json::yy_json::error;

        yyjson_mut_doc        *doc;
        ::cppxx::json::TagInfo t = {};

        template <typename T>
        static bool is_empty_value(const T &val) {
            return ::cppxx::json::detail::is_empty_value(val);
        }

        template <typename T>
        static constexpr bool is_aggregate_contains_tagged_v = cppxx::json::detail::is_aggregate_contains_tagged_v<T>;
    };

    struct Deserializer {
        using error = ::cppxx::json::yy_json::error;

        yyjson_val            *val;
        ::cppxx::json::TagInfo t = {};

        template <typename T>
        static bool is_empty_value(const T &val) {
            return ::cppxx::json::detail::is_empty_value(val);
        }

        template <typename T>
        static constexpr bool is_aggregate_contains_tagged_v = cppxx::json::detail::is_aggregate_contains_tagged_v<T>;
    };

    class error : public std::exception {
        mutable std::string what_;

    public:
        std::string context;
        std::string msg;

        explicit error(std::string msg)
            : msg(std::move(msg)) {}

        error(std::string_view key, std::string msg)
            : context("." + std::string(key))
            , msg(std::move(msg)) {}

        error(size_t key, std::string msg)
            : context("[" + std::to_string(key) + "]")
            , msg(std::move(msg)) {}

        error add_context(std::string_view key) & {
            error e(std::move(msg));
            e.context = "." + std::string(key) + std::move(context);
            return e;
        }

        error add_context(size_t key) & {
            error e(std::move(msg));
            e.context = "[" + std::to_string(key) + "]" + std::move(context);
            return e;
        }

        const char *what() const noexcept override {
            what_ = "yyjson error at " + (context.empty() ? "<root>" : context) + ": " + msg;
            return what_.c_str();
        }

        static error type_mismatch_error(const char *expect, yyjson_val *val) {
            return error("Type mismatch error: expect `" + std::string(expect) + "` got `" + yyjson_get_type_desc(val) + "`");
        }

        static error size_mismatch_error(size_t expect, size_t got) {
            return error("Size mismatch error: expect `" + std::to_string(expect) + "` got `" + std::to_string(got) + "`");
        }
    };
} // namespace cppxx::json::yy_json

namespace cppxx::json::yy_json::detail {
    template <typename T>
    void generic_parse(const std::string &str, T &val, uint32_t yyjson_read_flag, bool doc_is_path) {
        yyjson_read_err err;
        yyjson_doc     *doc = doc_is_path
                                  ? yyjson_read_file(const_cast<char *>(str.c_str()), yyjson_read_flag, nullptr, &err)
                                  : yyjson_read_opts(const_cast<char *>(str.c_str()), str.size(), yyjson_read_flag, nullptr, &err);
        if (!doc)
            throw error(err.msg);

        auto _ = defer([&]() { yyjson_doc_free(doc); });
        Deserialize<T>{yyjson_doc_get_root(doc)}.into(val);
    }
} // namespace cppxx::json::yy_json::detail


namespace cppxx::json::yy_json {
    template <typename T>
    void parse(const std::string &str, T &val, uint32_t yyjson_read_flag) {
        return detail::generic_parse<T>(str, val, yyjson_read_flag, false);
    }

    template <typename T>
    void parse_from_file(const std::string &path, T &val, uint32_t yyjson_read_flag) {
        return detail::generic_parse<T>(path, val, yyjson_read_flag, true);
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &str, uint32_t yyjson_read_flag) {
        T val = {};
        detail::generic_parse<T>(str, val, yyjson_read_flag, false);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse_from_file(const std::string &path, uint32_t yyjson_read_flag) {
        T val = {};
        detail::generic_parse<T>(path, val, yyjson_read_flag, true);
        return val;
    }

    template <typename T>
    [[nodiscard]]
    std::string dump(const T &val, uint32_t yyjson_write_flag) {
        yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
        if (!doc)
            throw error("Fatal error: cannot create yyjson doc");

        auto _ = defer([&]() { yyjson_mut_doc_free(doc); });
        yyjson_mut_doc_set_root(doc, Serialize<T>{doc}.from(val));

        size_t           len;
        yyjson_write_err err;
        const char      *str = yyjson_mut_write_opts(doc, yyjson_write_flag, nullptr, &len, &err);
        if (!str)
            throw error(err.msg);

        std::string res = {str, len};
        ::free((void *)str);
        return res;
    }
} // namespace cppxx::json::yy_json


#define SERIALIZER   ::cppxx::json::yy_json::Serializer
#define DESERIALIZER ::cppxx::json::yy_json::Deserializer

namespace cppxx::serde {
    template <>
    struct Parse<DESERIALIZER, std::string> {
        const std::string &str;
        uint32_t           yyjson_read_flag = YYJSON_READ_NOFLAG;

        template <typename T>
        void into(T &val) const {
            ::cppxx::json::yy_json::parse(str, val, yyjson_read_flag);
        }
    };

    template <>
    struct ParseFromFile<DESERIALIZER> {
        const std::string &path;
        uint32_t           yyjson_read_flag = YYJSON_READ_NOFLAG;

        template <typename T>
        void into(T &val) const {
            ::cppxx::json::yy_json::parse_from_file(path, val, yyjson_read_flag);
        }
    };

    template <>
    struct Dump<SERIALIZER, std::string> {
        uint32_t yyjson_write_flag = YYJSON_WRITE_NOFLAG;

        template <typename T>
        std::string from(const T &val) const {
            return ::cppxx::json::yy_json::dump(val, yyjson_write_flag);
        }
    };

    // bool
    template <>
    struct Serialize<SERIALIZER, bool> : SERIALIZER {
        type from(bool v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_bool(doc, v);
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, bool> : DESERIALIZER {
        void into(bool &v) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_bool(val))
                throw error::type_mismatch_error("bool", val);
            v = yyjson_get_bool(val);
        }
    };

    // sint
    template <typename T>
    struct Serialize<
        SERIALIZER,
        T,
        std::enable_if_t<std::is_signed_v<T> && !std::is_same_v<T, bool> && !std::is_floating_point_v<T>>> : SERIALIZER {
        type from(T v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_sint(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<
        DESERIALIZER,
        T,
        std::enable_if_t<std::is_signed_v<T> && !std::is_same_v<T, bool> && !std::is_floating_point_v<T>>> : DESERIALIZER {
        void into(T &v) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_sint(val) && !yyjson_is_uint(val))
                throw error::type_mismatch_error("sint", val);
            v = (T)yyjson_get_sint(val);
        }
    };

    // uint
    template <typename T>
    struct Serialize<SERIALIZER, T, std::enable_if_t<std::is_unsigned_v<T> && !std::is_same_v<T, bool>>> : SERIALIZER {
        type from(T v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_uint(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, T, std::enable_if_t<std::is_unsigned_v<T> && !std::is_same_v<T, bool>>> : DESERIALIZER {
        void into(T &v) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_uint(val))
                throw error::type_mismatch_error("uint", val);
            v = (T)yyjson_get_uint(val);
        }
    };

    // float
    template <typename T>
    struct Serialize<SERIALIZER, T, std::enable_if_t<std::is_floating_point_v<T>>> : SERIALIZER {
        type from(T v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_real(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, T, std::enable_if_t<std::is_floating_point_v<T>>> : DESERIALIZER {
        void into(T &v) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_real(val))
                throw error::type_mismatch_error("real", val);
            v = (T)yyjson_get_real(val);
        }
    };

    // string
    template <>
    struct Serialize<SERIALIZER, std::string> : SERIALIZER {
        type from(const std::string &v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_strn(doc, v.c_str(), v.size());
        }
        type from(std::string &&v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_strncpy(doc, v.c_str(), v.size());
        }
    };

    template <>
    struct Serialize<SERIALIZER, std::string_view> : SERIALIZER {
        type from(std::string_view v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_strn(doc, v.data(), v.size());
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, std::string> : DESERIALIZER {
        void into(std::string &v) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_str(val))
                throw error::type_mismatch_error("string", val);
            v = yyjson_get_str(val);
        }
    };

    // optional
    template <typename T>
    struct Serialize<SERIALIZER, std::optional<T>> : SERIALIZER {
        type from(const std::optional<T> &v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            if (!v.has_value())
                return yyjson_mut_null(doc);
            return Serialize<SERIALIZER, T>{doc}.from(*v);
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, std::optional<T>, std::enable_if_t<std::is_default_constructible_v<T>>> : DESERIALIZER {
        void into(std::optional<T> &v) const {
            if (!val && t.skipmissing)
                return;
            if (!val || yyjson_is_null(val)) {
                v = std::nullopt;
                return;
            }
            v = T{};
            Deserialize<DESERIALIZER, T>{val}.into(*v);
        }
    };

    // array
    template <typename T, size_t N>
    struct Serialize<SERIALIZER, std::array<T, N>> : SERIALIZER {
        type from(const std::array<T, N> &v) const {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            for (size_t i = 0; i < v.size(); ++i) {
                try {
                    yyjson_mut_arr_append(arr, Serialize<SERIALIZER, T>{doc}.from(v[i]));
                } catch (error &e) {
                    throw e.add_context(i);
                }
            }
            return arr;
        }
    };

    template <typename T, size_t N>
    struct Deserialize<DESERIALIZER, std::array<T, N>> : DESERIALIZER {
        void into(std::array<T, N> &v) const {
            auto arr = this->val;
            if (!arr && t.skipmissing)
                return;
            if (yyjson_is_arr(arr))
                throw error::type_mismatch_error("array", arr);
            if (auto n = yyjson_arr_size(arr); N != n)
                throw error::size_mismatch_error(N, n);

            size_t      idx, max;
            yyjson_val *val;
            yyjson_arr_foreach(arr, idx, max, val) {
                try {
                    Deserialize<DESERIALIZER, T>{val}.into(v[idx]);
                } catch (error &e) {
                    throw e.add_context(idx);
                }
            }
        }
    };

    // vector
    template <typename T>
    struct Serialize<SERIALIZER, std::vector<T>> : SERIALIZER {
        type from(const std::vector<T> &v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            for (size_t i = 0; i < v.size(); ++i) {
                try {
                    yyjson_mut_arr_append(arr, Serialize<SERIALIZER, T>{doc}.from(v[i]));
                } catch (error &e) {
                    throw e.add_context(i);
                }
            }
            return arr;
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, std::vector<T>, std::enable_if_t<std::is_default_constructible_v<T>>> : DESERIALIZER {
        void into(std::vector<T> &v) const {
            auto arr = this->val;
            if (!arr && t.skipmissing)
                return;
            if (!yyjson_is_arr(arr))
                throw error::type_mismatch_error("array", arr);
            v.resize(yyjson_arr_size(arr));
            size_t      idx, max;
            yyjson_val *val;
            yyjson_arr_foreach(arr, idx, max, val) {
                try {
                    Deserialize<DESERIALIZER, T>{val}.into(v[idx]);
                } catch (error &e) {
                    throw e.add_context(idx);
                }
            }
        }
    };

    // tuple
    template <typename... T>
    struct Serialize<SERIALIZER, std::tuple<T...>> : SERIALIZER {
        type from(const std::tuple<T...> &v) const {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            return std::apply(
                [&](const auto &...item) {
                    size_t i = 0;
                    (
                        [&]() {
                            try {
                                yyjson_mut_arr_append(arr, Serialize<SERIALIZER, std::decay_t<decltype(item)>>{doc}.from(item));
                            } catch (error &e) {
                                throw e.add_context(i);
                            }
                            ++i;
                        }(),
                        ...
                    );
                },
                v
            );
        }
    };

    template <typename... T>
    struct Deserialize<DESERIALIZER, std::tuple<T...>> : DESERIALIZER {
        void into(std::tuple<T...> &v) const {
            auto arr = this->val;
            if (!arr && t.skipmissing)
                return;
            if (yyjson_is_arr(arr))
                throw error::type_mismatch_error("array", arr);
            if (auto n = yyjson_arr_size(arr); sizeof...(T) != n)
                throw error::size_mismatch_error(sizeof...(T), n);

            int i = 0;
            std::apply(
                [&](auto &...item) {
                    (
                        [&]() {
                            try {
                                Deserialize<DESERIALIZER, std::decay_t<decltype(item)>>{yyjson_arr_get(arr, i)}.into(item);
                            } catch (error &e) {
                                throw e.add_context(i);
                            }
                            i++;
                        }(),
                        ...
                    );
                },
                v
            );
        }
    };

    // variant
    template <typename... T>
    struct Serialize<SERIALIZER, std::variant<T...>> : SERIALIZER {
        type from(const std::variant<T...> &v) const {
            return std::visit(
                [&](const auto &var) { return Serialize<SERIALIZER, std::decay_t<decltype(var)>>{doc, t}.from(var); }, v
            );
        }
    };

    template <typename... T>
    struct Deserialize<DESERIALIZER, std::variant<T...>, std::enable_if_t<(std::is_default_constructible_v<T> && ...)>>
        : DESERIALIZER {
        void into(std::variant<T...> &v) const {
            if (!val && t.skipmissing)
                return;
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
                            Deserialize<DESERIALIZER, Elem>{val}.into(element);
                            v    = std::move(element);
                            done = true;
                        }
                    } catch (...) {
                    }
                }(),
                ...
            );
            if (!done)
                throw error::type_mismatch_error("variant", val);
        }
    };

    // map
    template <typename T>
    struct Serialize<SERIALIZER, std::unordered_map<std::string, T>> : SERIALIZER {
        type from(const std::unordered_map<std::string, T> &v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            for (auto &[k, v] : v) {
                yyjson_mut_val *key = yyjson_mut_strn(doc, k.data(), k.size()), *item;
                try {
                    item = Serialize<SERIALIZER, T>{doc}.from(v);
                } catch (error &e) {
                    throw e.add_context(k);
                }
                yyjson_mut_obj_add(obj, key, item);
            }
            return obj;
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, std::unordered_map<std::string, T>, std::enable_if_t<std::is_default_constructible_v<T>>>
        : DESERIALIZER {
        void into(std::unordered_map<std::string, T> &v) {
            auto obj = this->val;
            if (!obj && t.skipmissing)
                return;
            if (!yyjson_is_obj(obj))
                throw error::type_mismatch_error("object", obj);
            size_t      idx, max;
            yyjson_val *key, *val;
            yyjson_obj_foreach(obj, idx, max, key, val) {
                std::string key_str;
                Deserialize<DESERIALIZER, std::string>{key}.into(key_str);
                auto item = T{};
                try {
                    Deserialize<DESERIALIZER, T>{val}.into(item);
                } catch (error &e) {
                    throw e.add_context(key_str);
                }
                v.emplace(std::move(key_str), std::move(item));
            }
        }
    };

    // aggregate struct (only tagged)
    template <typename S>
    struct Serialize<SERIALIZER, S, std::enable_if_t<SERIALIZER::is_aggregate_contains_tagged_v<S>>> : SERIALIZER {
        type from(const S &v) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            boost::pfr::for_each_field(v, [&](auto &field) {
                using T = std::decay_t<decltype(field)>;
                if constexpr (cppxx::is_tagged_v<T>) {
                    if (auto [tag, ti] = json::TagInfo::from_tagged(field); tag != "") {
                        yyjson_mut_val *key = yyjson_mut_strn(doc, tag.data(), tag.size()), *item;
                        try {
                            item = Serialize<SERIALIZER, remove_tag_t<T>>{doc, ti}.from(field.get_value());
                        } catch (error &e) {
                            throw e.add_context(tag);
                        }
                        yyjson_mut_obj_add(obj, key, item);
                    }
                }
            });
            return obj;
        }
    };

    template <typename S>
    struct Deserialize<DESERIALIZER, S, std::enable_if_t<DESERIALIZER::is_aggregate_contains_tagged_v<S>>> : DESERIALIZER {
        void into(S &v) {
            auto obj = this->val;
            if (!obj && t.skipmissing)
                return;
            if (!yyjson_is_obj(obj))
                throw error::type_mismatch_error("object", obj);
            boost::pfr::for_each_field(v, [&](auto &field) {
                using T = std::decay_t<decltype(field)>;
                if constexpr (cppxx::is_tagged_v<T>) {
                    if (auto [tag, ti] = json::TagInfo::from_tagged(field); tag != "") {
                        yyjson_val *item = yyjson_obj_getn(obj, tag.data(), tag.size());
                        try {
                            Deserialize<DESERIALIZER, remove_tag_t<T>>{item, ti}.into(field.get_value());
                        } catch (error &e) {
                            throw e.add_context(tag);
                        }
                    }
                }
            });
        }
    };

    // enum
    template <typename S>
    struct Serialize<SERIALIZER, S, std::enable_if_t<std::is_enum_v<S>>> : SERIALIZER {
        type from(const S &v) const {
            return Serialize<SERIALIZER, std::string_view>{doc}.from(magic_enum::enum_name(v));
        }
    };

    template <typename S>
    struct Deserialize<DESERIALIZER, S, std::enable_if_t<std::is_enum_v<S>>> : DESERIALIZER {
        void into(S &v) {
            if (!val && t.skipmissing)
                return;

            std::string str;
            Deserialize<DESERIALIZER, std::string>{val}.into(str);
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

    // std::tm
    template <>
    struct Serialize<SERIALIZER, std::tm> : SERIALIZER {
        type from(const std::tm &tm) const {
            char buf[64];
            auto len = std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
            if (len == 0)
                throw error("Cannot serialize std::tm");

            return Serialize<SERIALIZER, std::string>{doc}.from(std::string(buf, len));
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, std::tm> : DESERIALIZER {
        void into(std::tm &tm) {
            if (!val && t.skipmissing)
                return;

            std::string buf;
            Deserialize<DESERIALIZER, std::string>{val}.into(buf);

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
} // namespace cppxx::serde

#undef SERIALIZER
#undef DESERIALIZER
#endif
