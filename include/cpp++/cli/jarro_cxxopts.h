#ifndef CPPXX_JSON_YYJSON_H
#define CPPXX_JSON_YYJSON_H

#include <cpp++/cli/cli.h>
#include <cpp++/serde/deserialize.h>

#ifndef CXXOPTS_HPP_INCLUDED
#    include <cxxopts.hpp>
#endif

namespace cppxx::cli::jarro_cxxopts {
    struct Deserializer {
        TagInfo t;
    };

    template <typename To, typename Enable = void>
    using Deserialize = ::cppxx::serde::Deserialize<Deserializer, To, Enable>;
} // namespace cppxx::cli::jarro_cxxopts


#endif
