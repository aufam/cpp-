#ifndef CPPXX_JSON_NLOHMANN_JSON_H
#define CPPXX_JSON_NLOHMANN_JSON_H

#include <cpp++/json/json.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <ctime>

#ifndef INCLUDE_NLOHMANN_JSON_HPP_
#    include <nlohmann/json.hpp>
#endif

#ifndef NEARGYE_MAGIC_ENUM_HPP
#    include <magic_enum/magic_enum.hpp>
#endif

namespace cppxx::json {
    using nlohmann_json = nlohmann::json;
}

namespace nlohmann {
    template <typename S>
    struct adl_serializer<S, std::enable_if_t<cppxx::json::detail::is_aggregate_contains_tagged_v<S>>> {
        static void from_json(const json &j, S &v) {
            boost::pfr::for_each_field(v, [&](auto &field) {
                using T = std::decay_t<decltype(field)>;
                if constexpr (cppxx::is_tagged_v<T>)
                    if (auto [key, tag] = cppxx::json::TagInfo::from_tagged(field); key != "") {
                        if (!tag.skipmissing)
                            j.at(key).get_to(field.get_value());
                        else if (auto it = j.find(key); it != j.end())
                            it->get_to(field.get_value());
                    }
            });
        }

        static void to_json(json &j, const S &v) {
            boost::pfr::for_each_field(v, [&](auto &field) {
                using T = std::decay_t<decltype(field)>;
                if constexpr (cppxx::is_tagged_v<T>)
                    if (auto [key, tag] = cppxx::json::TagInfo::from_tagged(field); key != "") {
                        auto &val = field.get_value();
                        if (tag.omitempty && cppxx::json::detail::is_empty_value(val))
                            return;
                        j[key] = val;
                    }
            });
        }
    };

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

    template <>
    struct adl_serializer<std::tm> {
        static void to_json(json &j, const std::tm &tm) {
            char buf[64];
            auto len = std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
            if (len == 0)
                throw nlohmann::json::type_error::create(303, "Cannot serialize std::tm", nullptr);

            j = std::string(buf, len);
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
} // namespace nlohmann

namespace cppxx::serde {
    template <>
    struct Parse<nlohmann::json, std::string> {
        const std::string &str;
        bool               ignore_comments = false;

        template <typename T>
        void into(T &val) const {
            val = nlohmann::json::parse(str, nullptr, true, ignore_comments);
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
