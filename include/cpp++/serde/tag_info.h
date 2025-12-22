#ifndef CPPXX_SERDE_H
#define CPPXX_SERDE_H

#include <cpp++/tag.h>
#include <cpp++/tuple.h>
#include <array>

namespace cppxx::serde {
    struct TagInfo {
        std::string_view key         = "";
        bool             skipmissing = false;
        bool             omitempty   = false;
        bool             noserde     = false;
        bool             positional  = false;
        std::string_view help        = "";

        TagInfo() = default;
    };

    template <size_t N>
    struct TagInfoTuple {
        std::array<TagInfo, N> ts     = {};
        bool                   is_obj = true;
    };

    template <typename T>
    constexpr TagInfo get_tag_info(const T &field, std::string_view tag, char separator = ',') {
        TagInfo ti    = {};
        bool    first = true;

        std::string_view sv;
        if constexpr (is_tagged_v<T>)
            sv = field.get_tag(tag);

        while (!sv.empty()) {
            size_t           next = sv.find(separator);
            std::string_view part = sv.substr(0, next);

            if (first)
                (ti.key = part, first = false);
            else if (part == "skipmissing")
                ti.skipmissing = true;
            else if (part == "omitempty")
                ti.omitempty = true;
            else if (part == "noserde")
                ti.noserde = true;
            else if (part == "positional")
                ti.positional = true;
            else if (std::string_view h = "help="; part.size() >= h.size() && part.compare(0, h.size(), h) == 0)
                ti.help = part.substr(h.size());

            if (next == std::string_view::npos)
                break;
            sv.remove_prefix(next + 1);
        }

        return ti;
    }

    template <typename... T>
    constexpr TagInfoTuple<sizeof...(T)>
    get_tag_info_from_tuple(const std::tuple<T...> &fields, std::string_view tag, char separator = ',') {
        TagInfoTuple<sizeof...(T)> ts       = {};
        bool                       is_array = sizeof...(T) > 0;

        tuple_for_each(fields, [&](const auto &field, size_t i) {
            if (const TagInfo &t = ts.ts[i] = get_tag_info(field, tag, separator); t.key != "")
                is_array &= t.positional;
        });

        ts.is_obj = !is_array;
        return ts;
    }
} // namespace cppxx::serde

#endif
