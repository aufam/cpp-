#ifndef CPPXX_JSON_NLOHMANN_JSON_H
#define CPPXX_JSON_NLOHMANN_JSON_H

#include <cpp++/json/json.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <variant>
#include <ctime>

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

    template <typename... Ts>
    struct adl_serializer<std::tuple<Ts...>> {
        static void to_json(json &j, const std::tuple<Ts...> &tpl) {
            cppxx::serde::TagInfoTuple<sizeof...(Ts)>         ts     = cppxx::json::get_tag_info_from_tuple(tpl);
            std::array<cppxx::serde::TagInfo, sizeof...(Ts)> &ti     = ts.ts;
            bool                                              is_obj = ts.is_obj;

            if (is_obj)
                j = nlohmann::json::object();
            else
                j = nlohmann::json::array();

            cppxx::tuple_for_each(tpl, [&](const auto &item, const size_t i) {
                const cppxx::serde::TagInfo &t = ti[i];
                const auto                  &v = [&]() -> decltype(auto) {
                    if constexpr (cppxx::is_tagged_v<std::decay_t<decltype(item)>>) {
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

                auto &val = is_obj ? j[t.key] : j[i];
                if (t.noserde)
                    if constexpr (std::is_same_v<T, std::string>)
                        val = nlohmann::json::parse(v);
                    else
                        throw nlohmann::json::type_error::create(
                            0, "field with tag `noserde` can only be serialized from std::string", nullptr
                        );
                else
                    val = nlohmann::json(v);
            });
        }

        static void from_json(const json &j, std::tuple<Ts...> &tpl) {
            cppxx::serde::TagInfoTuple<sizeof...(Ts)>         ts     = cppxx::json::get_tag_info_from_tuple(tpl);
            std::array<cppxx::serde::TagInfo, sizeof...(Ts)> &ti     = ts.ts;
            bool                                              is_obj = ts.is_obj;

            if (is_obj && !j.is_object())
                throw std::runtime_error("type mismatch: expect object");
            if (!is_obj) {
                if (!j.is_array())
                    throw std::runtime_error("type mismatch: expect array");
                else if (auto n = j.size(); sizeof...(Ts) != n)
                    throw std::runtime_error("size mismatch");
            }

            size_t idx = 0;
            cppxx::tuple_for_each(tpl, [&](auto &item, const size_t i) {
                cppxx::serde::TagInfo &t = ti[i];
                auto                  &v = [&]() -> decltype(auto) {
                    if constexpr (cppxx::is_tagged_v<std::decay_t<decltype(item)>>) {
                        return item.get_value();
                    } else {
                        return item;
                    }
                }();
                using T = std::decay_t<decltype(v)>;

                if (is_obj && t.key == "")
                    return;

                const nlohmann::json *ptr;
                try {
                    ptr = is_obj ? &j.at(t.key) : &j.at(i);
                } catch (...) {
                    if (t.skipmissing)
                        return;
                    else
                        throw;
                }

                if (t.noserde)
                    if constexpr (std::is_same_v<T, std::string>)
                        v = ptr->dump();
                    else
                        throw nlohmann::json::type_error::create(
                            0, "field with tag `noserde` can only be deserialized into std::string", nullptr
                        );
                else
                    ptr->get_to(v);
            });
            std::apply(
                [&](auto &...item) {
                    (
                        [&]() {
                            const auto             i = idx++;
                            cppxx::serde::TagInfo &t = ti[i];
                            auto                  &v = [&]() -> decltype(auto) {
                                if constexpr (cppxx::is_tagged_v<std::decay_t<decltype(item)>>) {
                                    return item.get_value();
                                } else {
                                    return item;
                                }
                            }();
                            using T = std::decay_t<decltype(v)>;

                            if (is_obj && t.key == "")
                                return;

                            const nlohmann::json *ptr;
                            try {
                                ptr = is_obj ? &j.at(t.key) : &j.at(i);
                            } catch (...) {
                                if (t.skipmissing)
                                    return;
                                else
                                    throw;
                            }

                            if (t.noserde)
                                if constexpr (std::is_same_v<T, std::string>)
                                    v = ptr->dump();
                                else
                                    throw nlohmann::json::type_error::create(
                                        0, "field with tag `noserde` can only be deserialized into std::string", nullptr
                                    );
                            else
                                ptr->get_to(v);
                        }(),
                        ...
                    );
                },
                tpl
            );
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
            bool done = false;
            (
                [&]() {
                    using Elem = std::tuple_element_t<I, std::tuple<T...>>;
                    try {
                        if (!done) {
                            auto element = Elem{};
                            j.get_to(element);
                            v    = std::move(element);
                            done = true;
                        }
                    } catch (nlohmann::json::exception &e) {
                        std::ignore = e;
                    }
                }(),
                ...
            );
            if (!done)
                throw nlohmann::json::type_error::create(0, "variant", nullptr);
        }
    };

    template <>
    struct adl_serializer<std::tm> {
        static void to_json(json &j, const std::tm &tm) {
            std::array<char, 64> buf;
            auto                 len = std::strftime(buf.data(), buf.size(), "%Y-%m-%dT%H:%M:%SZ", &tm);
            if (len == 0)
                throw nlohmann::json::type_error::create(303, "Failed to serialize std::tm", nullptr);

            j = std::string(buf.data(), buf.size());
        }

        static void from_json(const json &j, std::tm &tm) {
            std::string buf = j;

            if (buf.size() != 20 || buf[4] != '-' || buf[7] != '-' || buf[10] != 'T' || buf[13] != ':' || buf[16] != ':' ||
                buf[19] != 'Z')
                throw nlohmann::json::type_error::create(303, "Invalid datetime format: " + buf, nullptr);

            try {
                tm.tm_year  = std::stoi(buf.substr(0, 4)) - 1900;
                tm.tm_mon   = std::stoi(buf.substr(5, 2)) - 1;
                tm.tm_mday  = std::stoi(buf.substr(8, 2));
                tm.tm_hour  = std::stoi(buf.substr(11, 2));
                tm.tm_min   = std::stoi(buf.substr(14, 2));
                tm.tm_sec   = std::stoi(buf.substr(17, 2));
                tm.tm_isdst = 0; // UTC
            } catch (...) {
                throw nlohmann::json::type_error::create(303, "Invalid datetime format: " + buf, nullptr);
            }
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
                throw nlohmann::json::type_error::create(303, what, nullptr);
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
    struct Serialize<nlohmann::json, T> {
        nlohmann::json from(const T &v) const {
            return v;
        }
    };

    template <typename T>
    struct Deserialize<nlohmann::json, T> {
        const nlohmann::json &j;

        void into(T &v) const {
            j.get_to(v);
        }
    };

    template <>
    struct Parse<nlohmann::json, std::string> {
        const std::string &str;
        bool               ignore_comments = false;

        template <typename T>
        void into(T &val) const {
            nlohmann::json::parse(str, nullptr, true, ignore_comments).get_to(val);
        }
    };

    template <>
    struct Parse<nlohmann::json, std::istream> {
        std::istream stream;
        bool         ignore_comments = false;

        template <typename T>
        void into(T &val) const {
            nlohmann::json::parse(std::move(stream), nullptr, true, ignore_comments).get_to(val);
        }
    };

    template <>
    struct Parse<nlohmann::json, std::FILE *> {
        std::FILE *file;
        bool       ignore_comments = false;

        template <typename T>
        void into(T &val) const {
            nlohmann::json::parse(file, nullptr, true, ignore_comments).get_to(val);
        }
    };

    template <>
    struct Dump<nlohmann::json, std::string> {
        int  indent       = -1;
        char indent_char  = ' ';
        bool ensure_ascii = false;

        template <typename T>
        std::string from(const T &val) const {
            return nlohmann::json(val).dump(indent, indent_char, ensure_ascii);
        }
    };
} // namespace cppxx::serde
#endif
