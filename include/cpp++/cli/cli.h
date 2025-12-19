#ifndef CPPXX_CLI_H
#define CPPXX_CLI_H

#include <cpp++/tag.h>
#include <cctype>

namespace cppxx::cli {
    struct TagInfo {
        char             key_char = '\0';
        std::string_view key_str;
        bool             positional = false;
        std::string_view help;

        template <typename T>
        static constexpr TagInfo from_tagged(const Tag<T> &field) {
            TagInfo opt = {};

            for (std::string_view sv = field.get_tag("opt"); !sv.empty();) {
                auto pos = sv.find(',');
                auto p   = sv.substr(0, pos);

                if (p.size() == 1 && std::isalpha(static_cast<unsigned char>(p[0])))
                    opt.key_char = p[0];
                else if (std::string_view help_prefix = "help=";
                         p.size() >= help_prefix.size() && p.compare(0, help_prefix.size(), help_prefix) == 0)
                    opt.help = p.substr(help_prefix.size());
                else if (p == "positional")
                    opt.positional = true;
                else
                    opt.key_str = p;

                if (pos == std::string_view::npos)
                    break;
                sv.remove_prefix(pos + 1);
            }

            return opt;
        }
    };
} // namespace cppxx::cli

#endif
