#ifndef CPPXX_JSON_RAPIDJSON_H
#define CPPXX_JSON_RAPIDJSON_H

#include <cpp++/json/json.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>

#ifndef RAPIDJSON_DOCUMENT_H_
#    include <rapidjson/document.h>
#endif

#ifndef RAPIDJSON_PRETTYWRITER_H_
#    include <rapidjson/prettywriter.h>
#endif

#ifndef NEARGYE_MAGIC_ENUM_HPP
#    include <magic_enum/magic_enum.hpp>
#endif


namespace cppxx::json::rapid_json {
    template <typename From, typename Enable = void>
    using Serialize = ::cppxx::serde::Serialize<rapidjson::Document, From, Enable>;

    template <typename To, typename Enable = void>
    using Deserialize = ::cppxx::serde::Deserialize<rapidjson::Document, To, Enable>;

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
} // namespace cppxx::json::rapid_json


#define SERIALIZER   rapidjson::Document &
#define DESERIALIZER const rapidjson::Document &

namespace cppxx::serde {
    template <typename T>
    struct Serialize<
        SERIALIZER,
        T,
        std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, bool>>> {
        SERIALIZER doc;

        rapidjson::Value from(T v) const {
            return rapidjson::Value(v);
        }
    };

    template <>
    struct Serialize<SERIALIZER, std::string> {
        SERIALIZER doc;

        rapidjson::Value from(const std::string &v) const {
            return rapidjson::Value(v);
        }
    };
} // namespace cppxx::serde
#endif
