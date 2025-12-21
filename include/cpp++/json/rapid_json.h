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
    template <typename From>
    using Serialize = ::cppxx::serde::Serialize<rapidjson::Value, From>;

    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<rapidjson::Value, To>;

    using Dump = ::cppxx::serde::Dump<rapidjson::Document, std::string>;

    using Parse = ::cppxx::serde::Deserialize<rapidjson::Document, std::string>;
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
