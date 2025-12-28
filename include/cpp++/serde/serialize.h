#ifndef CPPXX_SERDE_SERIALIZE_H
#define CPPXX_SERDE_SERIALIZE_H

#include <type_traits>

namespace cppxx::serde {
    template <typename Serializer, typename From, typename Enable = void>
    struct Serialize;

    template <typename Serializer, typename To, typename Enable = void>
    struct Dump;

    template <typename Serializer, typename From, typename = void>
    struct is_serializable : std::false_type {};

    template <typename Serializer, typename From>
    struct is_serializable<
        Serializer,
        From,
        std::void_t<decltype(std::declval<Serialize<Serializer, From>>().from(std::declval<const From &>()))>> : std::true_type {
    };

    template <typename Serializer, typename From>
    static constexpr bool is_serializable_v = is_serializable<Serializer, From>::value;
} // namespace cppxx::serde

#endif
