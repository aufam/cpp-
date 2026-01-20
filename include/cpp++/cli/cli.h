#ifndef CPPXX_CLI_H
#define CPPXX_CLI_H

#include <cpp++/serde/tag_info.h>

namespace cppxx::cli {
    template <typename T>
    constexpr serde::TagInfo get_tag_info(const T &field) {
        return serde::get_tag_info(field, "opt");
    }
} // namespace cppxx::cli

#endif
