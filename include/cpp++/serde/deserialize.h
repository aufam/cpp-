#ifndef CPPXX_SERDE_DESERIALIZE_H
#define CPPXX_SERDE_DESERIALIZE_H

#include <type_traits>

namespace cppxx::serde {
    template <typename Deserializer, typename To, typename Enable = void>
    struct Deserialize;

    template <typename Deserializer, typename From, typename Enable = void>
    struct Parse;

    template <typename Deserializer, typename To, typename = void>
    struct is_deserializable : std::false_type {};

    template <typename Deserializer, typename To>
    struct is_deserializable<
        Deserializer,
        To,
        std::void_t<decltype(std::declval<Deserialize<Deserializer, To>>().into(std::declval<To &>()))>> : std::true_type {};

    template <typename Deserializer, typename To>
    static constexpr bool is_deserializable_v = is_deserializable<Deserializer, To>::value;
} // namespace cppxx::serde

#endif
