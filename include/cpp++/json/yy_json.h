#ifndef CPPXX_JSON_YYJSON_H
#define CPPXX_JSON_YYJSON_H

#include <cpp++/json/json.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/defer.h>
#include <yyjson.h>
#include <magic_enum/magic_enum.hpp>
#include <boost/pfr.hpp>
#include <array>
#include <variant>
#include <tuple>
#include <unordered_map>
#include <ctime>


namespace cppxx::json::yy_json {
    class error;

    struct Serializer {
        using Ctx   = yyjson_mut_doc *;
        using To    = yyjson_mut_val *;
        using Spec  = ::cppxx::json::TagInfo;
        using error = error;

        template <typename T>
        static bool is_empty_value(const T &val) {
            return ::cppxx::json::detail::is_empty_value(val);
        }

        template <typename T>
        static constexpr bool is_aggregate_contains_tagged_v = cppxx::json::detail::is_aggregate_contains_tagged_v<T>;
    };

    struct Deserializer {
        using Ctx   = yyjson_doc *;
        using From  = yyjson_val *;
        using Spec  = ::cppxx::json::TagInfo;
        using error = error;

        template <typename T>
        static bool is_empty_value(const T &val) {
            return ::cppxx::json::detail::is_empty_value(val);
        }

        template <typename T>
        static constexpr bool is_aggregate_contains_tagged_v = cppxx::json::detail::is_aggregate_contains_tagged_v<T>;
    };

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
    template <typename T, typename Enable = void>
    struct ToJson;

    template <typename T, typename Enable = void>
    struct FromJson;

    class error : public std::exception {
        mutable std::string what_;

    public:
        std::string context;
        std::string msg;

        error(std::string msg)
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
    };

    inline void throw_type_mismatch_error(const char *expect, yyjson_val *val) {
        throw error("Type mismatch error: expect `" + std::string(expect) + "` got `" + yyjson_get_type_desc(val) + "`");
    }

    inline void throw_size_mismatch_error(size_t expect, size_t got) {
        throw error("Size mismatch error: expect `" + std::to_string(expect) + "` got `" + std::to_string(got) + "`");
    }

    template <typename T>
    void generic_parse(const std::string &str, T &val, uint32_t yyjson_read_flag, bool doc_is_path) {
        yyjson_read_err err;
        yyjson_doc     *doc = doc_is_path
                                  ? yyjson_read_file(const_cast<char *>(str.c_str()), yyjson_read_flag, nullptr, &err)
                                  : yyjson_read_opts(const_cast<char *>(str.c_str()), str.size(), yyjson_read_flag, nullptr, &err);
        if (!doc)
            throw detail::error(err.msg);

        auto _   = defer([&]() { yyjson_doc_free(doc); });
        auto ser = detail::FromJson<T>{};
        ser.from(yyjson_doc_get_root(doc), val);
    }
} // namespace cppxx::json::yy_json::detail

namespace cppxx::json::yy_json {}

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
            throw detail::error("Fatal error: cannot create yyjson doc");

        auto _   = defer([&]() { yyjson_mut_doc_free(doc); });
        auto ser = detail::ToJson<T>{};
        yyjson_mut_doc_set_root(doc, ser.to(doc, val));

        size_t           len;
        yyjson_write_err err;
        const char      *str = yyjson_mut_write_opts(doc, yyjson_write_flag, nullptr, &len, &err);
        if (!str)
            throw detail::error(err.msg);

        std::string res = {str, len};
        ::free((void *)str);
        return res;
    }
} // namespace cppxx::json::yy_json


namespace cppxx::serde {

    // bool
    template <>
    struct Serialize<::cppxx::json::yy_json::Serializer, bool> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, bool v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_bool(doc, v);
        }
    };

    template <>
    struct Deserialize<::cppxx::json::yy_json::Deserializer, bool> : ::cppxx::json::yy_json::Deserializer {
        void from(From val, bool &v, Spec t = {}) const {
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
        ::cppxx::json::yy_json::Serializer,
        T,
        std::enable_if_t<std::is_signed_v<T> && !std::is_same_v<T, bool> && !std::is_floating_point_v<T>>>
        : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, T v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_sint(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<
        ::cppxx::json::yy_json::Deserializer,
        T,
        std::enable_if_t<std::is_signed_v<T> && !std::is_same_v<T, bool> && !std::is_floating_point_v<T>>>
        : ::cppxx::json::yy_json::Deserializer {
        void from(From val, T &v, Spec t = {}) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_sint(val) && !yyjson_is_uint(val))
                throw error::type_mismatch_error("sint", val);
            v = (T)yyjson_get_sint(val);
        }
    };

    // uint
    template <typename T>
    struct Serialize<::cppxx::json::yy_json::Serializer, T, std::enable_if_t<std::is_unsigned_v<T> && !std::is_same_v<T, bool>>>
        : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, T v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_uint(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<
        ::cppxx::json::yy_json::Deserializer,
        T,
        std::enable_if_t<std::is_unsigned_v<T> && !std::is_same_v<T, bool>>> : ::cppxx::json::yy_json::Deserializer {
        void from(From val, T &v, Spec t = {}) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_uint(val))
                throw error::type_mismatch_error("uint", val);
            v = (T)yyjson_get_uint(val);
        }
    };

    // float
    template <typename T>
    struct Serialize<::cppxx::json::yy_json::Serializer, T, std::enable_if_t<std::is_floating_point_v<T>>>
        : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, T v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_real(doc, v);
        }
    };

    template <typename T>
    struct Deserialize<::cppxx::json::yy_json::Deserializer, T, std::enable_if_t<std::is_floating_point_v<T>>>
        : ::cppxx::json::yy_json::Deserializer {
        void from(From val, T &v, Spec t = {}) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_real(val))
                throw error::type_mismatch_error("real", val);
            v = (T)yyjson_get_real(val);
        }
    };

    // string
    template <>
    struct Serialize<::cppxx::json::yy_json::Serializer, std::string> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::string &v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_strn(doc, v.c_str(), v.size());
        }
        To to_owned(Ctx doc, const std::string &v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_strncpy(doc, v.c_str(), v.size());
        }
    };

    template <>
    struct Serialize<::cppxx::json::yy_json::Serializer, std::string_view> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::string_view &v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            return yyjson_mut_strn(doc, v.data(), v.size());
        }
    };

    template <>
    struct Deserialize<::cppxx::json::yy_json::Deserializer, std::string> : ::cppxx::json::yy_json::Deserializer {
        void from(From val, std::string &v, Spec t = {}) const {
            if (!val && t.skipmissing)
                return;
            if (!yyjson_is_str(val))
                throw error::type_mismatch_error("string", val);
            v = yyjson_get_str(val);
        }
    };

    // optional
    template <typename T>
    struct Serialize<::cppxx::json::yy_json::Serializer, std::optional<T>> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::optional<T> &v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            if (!v.has_value())
                return yyjson_mut_null(doc);
            return Serialize<::cppxx::json::yy_json::Serializer, T>{}.to(doc, *v);
        }
    };

    template <typename T>
    struct Deserialize<
        ::cppxx::json::yy_json::Deserializer,
        std::optional<T>,
        std::enable_if_t<std::is_default_constructible_v<T>>> : ::cppxx::json::yy_json::Deserializer {
        void from(From val, std::optional<T> &v, Spec t = {}) const {
            if (!val && t.skipmissing)
                return;
            if (!val || yyjson_is_null(val)) {
                v = std::nullopt;
                return;
            }
            v = T{};
            Deserialize<::cppxx::json::yy_json::Deserializer, T>{}.from(*v);
        }
    };

    // array
    template <typename T, size_t N>
    struct Serialize<::cppxx::json::yy_json::Serializer, std::array<T, N>> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::array<T, N> &v, Spec = {}) const {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            for (size_t i = 0; i < v.size(); ++i) {
                try {
                    yyjson_mut_arr_append(arr, Serialize<::cppxx::json::yy_json::Serializer, T>{}.to(doc, v[i]));
                } catch (error &e) {
                    throw e.add_context(i);
                }
            }
            return arr;
        }
    };

    template <typename T, size_t N>
    struct Deserialize<::cppxx::json::yy_json::Deserializer, std::array<T, N>> : ::cppxx::json::yy_json::Deserializer {
        void from(From arr, std::array<T, N> &v, Spec t = {}) const {
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
                    Deserialize<::cppxx::json::yy_json::Deserializer, T>{}.from(v[idx]);
                } catch (error &e) {
                    throw e.add_context(idx);
                }
            }
        }
    };

    // vector
    template <typename T>
    struct Serialize<::cppxx::json::yy_json::Serializer, std::vector<T>> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::vector<T> &v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            for (size_t i = 0; i < v.size(); ++i) {
                try {
                    yyjson_mut_arr_append(arr, Serialize<::cppxx::json::yy_json::Serializer, T>{}.to(doc, v[i]));
                } catch (error &e) {
                    throw e.add_context(i);
                }
            }
            return arr;
        }
    };

    template <typename T>
    struct Deserialize<::cppxx::json::yy_json::Deserializer, std::vector<T>, std::enable_if_t<std::is_default_constructible_v<T>>>
        : ::cppxx::json::yy_json::Deserializer {
        void from(From arr, std::vector<T> &v, Spec t = {}) const {
            if (!arr && t.skipmissing)
                return;
            if (!yyjson_is_arr(arr))
                throw error::type_mismatch_error("array", arr);
            v.resize(yyjson_arr_size(arr));
            size_t      idx, max;
            yyjson_val *val;
            yyjson_arr_foreach(arr, idx, max, val) {
                try {
                    Deserialize<::cppxx::json::yy_json::Deserializer, T>{}.from(v[idx]);
                } catch (error &e) {
                    throw e.add_context(idx);
                }
            }
        }
    };

    // tuple
    template <typename... T>
    struct Serialize<::cppxx::json::yy_json::Serializer, std::tuple<T...>> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::tuple<T...> &v, Spec = {}) const {
            yyjson_mut_val *arr = yyjson_mut_arr(doc);
            return std::apply(
                [&](const auto &...item) {
                    size_t i = 0;
                    (
                        [&]() {
                            try {
                                yyjson_mut_arr_append(
                                    arr,
                                    Serialize<::cppxx::json::yy_json::Serializer, std::decay_t<decltype(item)>>{}.to(doc, item)
                                );
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
    struct Deserialize<::cppxx::json::yy_json::Deserializer, std::tuple<T...>> : ::cppxx::json::yy_json::Deserializer {
        void from(From arr, std::tuple<T...> &v, Spec t = {}) const {
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
                                Deserialize<::cppxx::json::yy_json::Deserializer, std::decay_t<decltype(item)>>{}.from(
                                    yyjson_arr_get(arr, i), item
                                );
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
    struct Serialize<::cppxx::json::yy_json::Serializer, std::variant<T...>> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::variant<T...> &v, Spec t = {}) const {
            return std::visit(
                [&](const auto &var) {
                    return Serialize<::cppxx::json::yy_json::Serializer, std::decay_t<decltype(var)>>{}.to(doc, var, t);
                },
                v
            );
        }
    };

    template <typename... T>
    struct Deserialize<
        ::cppxx::json::yy_json::Deserializer,
        std::variant<T...>,
        std::enable_if_t<(std::is_default_constructible_v<T> && ...)>> : ::cppxx::json::yy_json::Deserializer {
        void from(From val, std::variant<T...> &v, Spec t = {}) const {
            if (!val && t.skipmissing)
                return;
            try_for_each(val, v, std::index_sequence_for<T...>{});
        }

    protected:
        template <size_t... I>
        static void try_for_each(yyjson_val *val, std::variant<T...> &v, std::index_sequence<I...>) {
            bool done = false;
            (
                [&]() {
                    using Elem = std::tuple_element_t<I, std::tuple<T...>>;
                    try {
                        if (!done) {
                            auto element = Elem{};
                            Deserialize<::cppxx::json::yy_json::Deserializer, Elem>{}.from(val, element);
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
    struct Serialize<::cppxx::json::yy_json::Serializer, std::unordered_map<std::string, T>>
        : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::unordered_map<std::string, T> &v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            for (auto &[k, v] : v) {
                try {
                    yyjson_mut_val *key  = yyjson_mut_strn(doc, k.data(), k.size());
                    yyjson_mut_val *item = Serialize<::cppxx::json::yy_json::Serializer, T>{}.to(doc, v);
                    yyjson_mut_obj_add(obj, key, item);
                } catch (error &e) {
                    throw e.add_context(k);
                }
            }
            return obj;
        }
    };

    template <typename T>
    struct Deserialize<
        ::cppxx::json::yy_json::Deserializer,
        std::unordered_map<std::string, T>,
        std::enable_if_t<std::is_default_constructible_v<T>>> : ::cppxx::json::yy_json::Deserializer {
        void from(From obj, std::unordered_map<std::string, T> &v, Spec t = {}) {
            if (!obj && t.skipmissing)
                return;
            if (!yyjson_is_obj(obj))
                throw error::type_mismatch_error("object", obj);
            size_t      idx, max;
            yyjson_val *key, *val;
            yyjson_obj_foreach(obj, idx, max, key, val) {
                std::string key_str;
                Deserialize<::cppxx::json::yy_json::Deserializer, std::string>{}.from(key, key_str);

                auto item = T{};
                try {
                    Deserialize<::cppxx::json::yy_json::Deserializer, T>{}.from(val, item);
                } catch (error &e) {
                    throw e.add_context(key_str);
                }
                v.emplace(std::move(key_str), std::move(item));
            }
        }
    };

    // aggregate struct (only tagged)
    template <typename S>
    struct Serialize<
        ::cppxx::json::yy_json::Serializer,
        S,
        std::enable_if_t<::cppxx::json::detail::is_aggregate_contains_tagged_v<S>>> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const S &v, Spec t = {}) const {
            if (t.omitempty && is_empty_value(v))
                return nullptr;
            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            boost::pfr::for_each_field(v, [&](auto &field) {
                using T = std::decay_t<decltype(field)>;
                if constexpr (cppxx::is_tagged_v<T>) {
                    if (auto [tag, ti] = json::TagInfo::from_tagged(field); tag != "") {
                        yyjson_mut_val *key = yyjson_mut_strn(doc, tag.data(), tag.size()), *item;
                        try {
                            item =
                                Serialize<::cppxx::json::yy_json::Serializer, remove_tag_t<T>>{}.to(doc, field.get_value(), ti);
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
    struct Deserialize<
        ::cppxx::json::yy_json::Deserializer,
        S,
        std::enable_if_t<::cppxx::json::detail::is_aggregate_contains_tagged_v<S>>> : ::cppxx::json::yy_json::Deserializer {
        void from(From obj, S &v, Spec t = {}) {
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
                            Deserialize<::cppxx::json::yy_json::Deserializer, remove_tag_t<T>>{}.from(
                                item, field.get_value(), ti
                            );
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
    struct Serialize<::cppxx::json::yy_json::Serializer, S, std::enable_if_t<std::is_enum_v<S>>>
        : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const S &v, Spec = {}) const {
            return Serialize<::cppxx::json::yy_json::Serializer, std::string_view>{}.to(doc, magic_enum::enum_name(v));
        }
    };

    template <typename S>
    struct Deserialize<::cppxx::json::yy_json::Deserializer, S, std::enable_if_t<std::is_enum_v<S>>>
        : ::cppxx::json::yy_json::Deserializer {
        void from(From val, S &v, Spec t = {}) {
            if (!val && t.skipmissing)
                return;

            std::string str;
            Deserialize<::cppxx::json::yy_json::Deserializer, std::string>{}.from(val, str);
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
    struct Serialize<::cppxx::json::yy_json::Serializer, std::tm> : ::cppxx::json::yy_json::Serializer {
        To to(Ctx doc, const std::tm &tm, Spec = {}) const {
            char buf[64];
            auto len = std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
            if (len == 0)
                throw error("Cannot serialize std::tm");

            return Serialize<::cppxx::json::yy_json::Serializer, std::string>{}.to_owned(doc, std::string(buf, len));
        }
    };

    template <>
    struct Deserialize<::cppxx::json::yy_json::Deserializer, std::tm> : ::cppxx::json::yy_json::Deserializer {
        void from(From val, std::tm &tm, Spec t = {}) {
            if (!val && t.skipmissing)
                return;

            std::string buf;
            Deserialize<::cppxx::json::yy_json::Deserializer, std::string>{}.from(val, buf, t);

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
#endif
