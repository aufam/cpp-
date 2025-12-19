#ifndef CPPXX_SERDE_H
#define CPPXX_SERDE_H

#include <cpp++/tag.h>

namespace cppxx::serde {
    struct TagInfo {
        std::string_view key         = "";
        bool             skipmissing = false;
        bool             omitempty   = false;
        bool             noserde     = false;
        bool             positional  = false;
        std::string_view help        = "";
    };

    template <typename T>
    constexpr TagInfo get_tag_info(const Tag<T> &field, std::string_view tag, char separator = ',') {
        TagInfo ti    = {};
        bool    first = true;

        for (std::string_view sv = field.get_tag(tag); !sv.empty();) {
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
} // namespace cppxx::serde

#endif
