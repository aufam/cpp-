#ifndef CPPXX_OPTIONAL_H
#define CPPXX_OPTIONAL_H

#include <optional>
#include <type_traits>

namespace cppxx {
    template <typename T>
    struct is_optional : std::false_type {};

    template <typename T>
    struct is_optional<std::optional<T>> : std::true_type {};

    template <typename T>
    constexpr bool is_optional_v = is_optional<T>::value;
} // namespace cppxx

#endif
