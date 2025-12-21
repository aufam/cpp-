#ifndef CPPXX_SERDE_DESERIALIZE_H
#define CPPXX_SERDE_DESERIALIZE_H

namespace cppxx::serde {
    template <typename Deserializer, typename To, typename Enable = void>
    struct Deserialize;

    template <typename Deserializer, typename From, typename Enable = void>
    struct Parse;
} // namespace cppxx::serde

#endif
