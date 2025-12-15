#ifndef CPPXX_SERDE_SERIALIZE_H
#define CPPXX_SERDE_SERIALIZE_H

namespace cppxx::serde {
    template <typename Serializer, typename From, typename Enable = void>
    struct Serialize;

    template <typename Serializer, typename To, typename Enable = void>
    struct Dump;
} // namespace cppxx::serde

#endif
