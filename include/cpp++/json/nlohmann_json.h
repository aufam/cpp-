#ifndef CPPXX_JSON_NLOHMANN_JSON_H
#define CPPXX_JSON_NLOHMANN_JSON_H

#include <cpp++/json/json.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>
#include <cpp++/time.h>
#include <variant>

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#    include <nlohmann/json.hpp>
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

namespace cppxx::json::nlohmann_json {
    template <typename From>
    using Serialize = ::cppxx::serde::Serialize<nlohmann::json, From>;

    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<nlohmann::json, To>;

    using Dump = ::cppxx::serde::Dump<nlohmann::json, std::string>;

    template <typename From = std::string>
    using Parse = ::cppxx::serde::Parse<nlohmann::json, From>;
} // namespace cppxx::json::nlohmann_json

namespace nlohmann {
    // optional
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json &j, const std::optional<T> &opt) {
            if (opt.has_value())
                j = *opt;
            else
                j = nullptr;
        }

        static void from_json(const json &j, std::optional<T> &opt) {
            if (j.is_null())
                opt.reset();
            else
                opt = j.get<T>();
        }
    };

    // tuple
    template <typename... Ts>
    struct adl_serializer<std::tuple<Ts...>> {
        static void to_json(json &j, const std::tuple<Ts...> &tpl) {
            const cppxx::serde::TagInfoTuple<sizeof...(Ts)>         ts     = cppxx::json::get_tag_info_from_tuple(tpl);
            const std::array<cppxx::serde::TagInfo, sizeof...(Ts)> &ti     = ts.ts;
            const bool                                              is_obj = ts.is_obj;

            if (is_obj)
                j = nlohmann::json::object();
            else
                j = nlohmann::json::array();

            cppxx::tuple_for_each(tpl, [&](const auto &item, const size_t i) {
                const cppxx::serde::TagInfo &t = ti[i];
                const auto                  &v = cppxx::serde::detail::get_underlying_value(item);
                using T                        = std::decay_t<decltype(v)>;
                constexpr bool serializable    = cppxx::serde::is_serializable_v<nlohmann::json, T>;

                if (!serializable || (is_obj && t.key == "") || (t.omitempty && cppxx::serde::detail::is_empty_value(v)))
                    return;

                auto &val = is_obj ? j[t.key] : j[i];
                try {
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            val = nlohmann::json::parse(v);
                        else
                            throw cppxx::serde::error("field with tag `noserde` can only be serialized from std::string");
                    else {
                        if constexpr (serializable)
                            try {
                                val = v;
                            } catch (nlohmann::json::exception &e) {
                                throw cppxx::serde::error(e.what());
                            }
                    }
                } catch (cppxx::serde::error &e) {
                    if (is_obj)
                        e.add_context(t.key);
                    else
                        e.add_context(i);
                    throw;
                }
            });
        }

        static void from_json(const json &j, std::tuple<Ts...> &tpl) {
            const cppxx::serde::TagInfoTuple<sizeof...(Ts)>         ts     = cppxx::json::get_tag_info_from_tuple(tpl);
            const std::array<cppxx::serde::TagInfo, sizeof...(Ts)> &ti     = ts.ts;
            const bool                                              is_obj = ts.is_obj;

            if (is_obj && !j.is_object())
                throw cppxx::serde::type_mismatch_error("object", j.type_name());
            if (!is_obj && !j.is_array())
                throw cppxx::serde::type_mismatch_error("array", j.type_name());

            cppxx::tuple_for_each(tpl, [&](auto &item, const size_t i) {
                const cppxx::serde::TagInfo &t = ti[i];
                auto                        &v = cppxx::serde::detail::get_underlying_value(item);
                using T                        = std::decay_t<decltype(v)>;
                constexpr bool deserializable  = cppxx::serde::is_deserializable_v<nlohmann::json, T>;

                if (!deserializable || (is_obj && t.key == ""))
                    return;

                const nlohmann::json *ptr = nullptr;
                try {
                    try {
                        ptr = is_obj ? &j.at(t.key) : &j.at(i);
                    } catch (nlohmann::json::exception &e) {
                        if (t.skipmissing)
                            return;
                        else
                            throw cppxx::serde::error(e.what());
                    }
                    if (t.noserde)
                        if constexpr (std::is_same_v<T, std::string>)
                            v = ptr->dump();
                        else
                            throw cppxx::serde::error("field with tag `noserde` can only be deserialized into std::string");
                    else {
                        if constexpr (deserializable)
                            try {
                                ptr->get_to(v);
                            } catch (nlohmann::json::exception &e) {
                                throw cppxx::serde::error(e.what());
                            }
                    }
                } catch (cppxx::serde::error &e) {
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
    struct adl_serializer<std::variant<T...>> {
        static void from_json(const json &j, std::variant<T...> &v) {
            try_for_each(j, v, std::index_sequence_for<T...>{});
        }

        static void to_json(json &j, const std::variant<T...> &v) {
            std::visit([&](const auto &var) { j = var; }, v);
        }

        template <size_t... I>
        static void try_for_each(const json &j, std::variant<T...> &v, std::index_sequence<I...>) {
            bool        done = false;
            std::string type_names;
            (
                [&]() {
                    using Elem = std::tuple_element_t<I, std::tuple<T...>>;
                    try {
                        if (!done) {
                            auto element = Elem{};
                            cppxx::serde::Deserialize<nlohmann::json, Elem>{j}.into(element);
                            v    = std::move(element);
                            done = true;
                        }
                    } catch (cppxx::serde::type_mismatch_error &e) {
                        type_names += e.expected_type + '|';
                    }
                }(),
                ...
            );
            if (!done) {
                type_names.pop_back();
                throw cppxx::serde::type_mismatch_error(type_names, j.type_name());
            }
        }
    };

    template <>
    struct adl_serializer<std::tm> {
        static void to_json(json &j, const std::tm &tm) {
            j = cppxx::tm_to_string(tm);
        }

        static void from_json(const json &j, std::tm &tm) {
            std::string str = j;
            tm              = cppxx::tm_from_string(str);
        }
    };

#ifdef BOOST_PFR_HPP
    template <typename S>
    struct adl_serializer<S, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> {
        static void from_json(const json &j, S &v) {
            auto tpl = boost::pfr::structure_tie(v);
            j.get_to(tpl);
        }

        static void to_json(json &j, const S &v) {
            j = boost::pfr::structure_tie(v);
        }
    };
#endif

#ifdef NEARGYE_MAGIC_ENUM_HPP
    template <typename S>
    struct adl_serializer<S, std::enable_if_t<std::is_enum_v<S>>> {
        static void from_json(const json &j, S &v) {
            auto str = j.get<std::string>();
            auto e   = magic_enum::enum_cast<S>(str);
            if (!e.has_value()) {
                std::string what = "invalid value `" + str + "`, expected one of {";
                for (std::string_view name : magic_enum::enum_names<S>()) {
                    what += std::string(name) + ",";
                }
                what += "}";
                throw cppxx::serde::error(what);
            }
            v = *e;
        }

        static void to_json(json &j, const S &v) {
            j = magic_enum::enum_name(v);
        }
    };
#endif
} // namespace nlohmann

namespace cppxx::serde {
    template <typename T>
    struct Serialize<nlohmann::json, T, std::enable_if_t<std::is_convertible_v<T, nlohmann::json>>> {
        nlohmann::json from(const T &v) const {
            try {
                return v;
            } catch (nlohmann::json::exception &e) {
                throw error(e.what());
            }
        }
    };

    template <typename T>
    struct Deserialize<
        nlohmann::json,
        T,
        std::void_t<decltype(std::declval<const nlohmann::json &>().get_to(std::declval<T &>()))>> {
        const nlohmann::json &j;

        void into(T &v) const {
            try {
                j.get_to(v);
            } catch (nlohmann::json::type_error &e) {
                auto [expected, got] = extract_types(e.what());
                throw type_mismatch_error(expected, got);
            } catch (nlohmann::json::exception &e) {
                throw error(e.what());
            }
        }

        static std::pair<std::string, std::string> extract_types(std::string_view message) {
            std::string expected_type;
            std::string got_type;

            std::string_view prefix1 = "type must be ";
            std::string_view prefix2 = ", but is ";

            auto pos1 = message.find(prefix1);
            auto pos2 = message.find(prefix2);
            if (pos1 == std::string_view::npos || pos2 == std::string_view::npos)
                return {};

            expected_type = message.substr(pos1 + prefix1.size(), pos2 - (pos1 + prefix1.size()));
            got_type      = message.substr(pos2 + prefix2.size());
            return {expected_type, got_type};
        }
    };

    template <>
    struct Parse<nlohmann::json, std::string> {
        const std::string &str;
        bool               ignore_comments = false;

        template <typename T>
        void into(T &val) const {
            try {
                auto j = nlohmann::json::parse(str, nullptr, true, ignore_comments);
                Deserialize<nlohmann::json, T>{j}.into(val);
            } catch (nlohmann::json::exception &e) {
                throw error(e.what());
            }
        }
    };

    template <>
    struct Parse<nlohmann::json, std::istream> {
        std::istream stream;
        bool         ignore_comments = false;

        template <typename T>
        void into(T &val) const {
            try {
                auto j = nlohmann::json::parse(std::move(stream), nullptr, true, ignore_comments);
                Deserialize<nlohmann::json, T>{j}.into(val);
            } catch (nlohmann::json::exception &e) {
                throw error(e.what());
            }
        }
    };

    template <>
    struct Parse<nlohmann::json, std::FILE *> {
        std::FILE *file;
        bool       ignore_comments = false;

        template <typename T>
        void into(T &val) const {
            try {
                auto j = nlohmann::json::parse(file, nullptr, true, ignore_comments);
                Deserialize<nlohmann::json, T>{j}.into(val);
            } catch (nlohmann::json::exception &e) {
                throw error(e.what());
            }
        }
    };

    template <>
    struct Dump<nlohmann::json, std::string> {
        int  indent       = -1;
        char indent_char  = ' ';
        bool ensure_ascii = false;

        template <typename T>
        std::string from(const T &val) const {
            try {
                return Serialize<nlohmann::json, T>{}.from(val).dump(indent, indent_char, ensure_ascii);
            } catch (nlohmann::json::exception &e) {
                throw error(e.what());
            }
        }
    };
} // namespace cppxx::serde
#endif
