#ifndef CPPXX_TAG_H
#define CPPXX_TAG_H

#include <string_view>

namespace cppxx {
    /// A lightweight metadata wrapper for struct fields, inspired by Go-style struct tags.
    ///
    /// @code
    /// struct User {
    ///     Tag<std::string> name = "json:`name` opt:`name,positional,help=Specify user's name`";
    ///     Tag<int> age = "json:`age` opt:`a,age,help=specify user's age`";
    ///     Tag<std::optional<std::string>> email = {
    ///         "json:`email` opt:`email`", "user@example.com"
    ///     }; // with default value
    /// };
    /// @endcode
    template <typename T>
    class Tag {
    protected:
        const char *tag;
        T           value = {};

    public:
        using type = T;

        constexpr Tag(const char *tag)
            : tag(tag) {}

        constexpr Tag(const char *tag, const T &value)
            : tag(tag)
            , value(value) {}

        constexpr Tag(const char *tag, T &&value)
            : tag(tag)
            , value(std::move(value)) {}

        void operator=(const char *) = delete;

        constexpr const T &operator()() const & {
            return value;
        }
        constexpr T &operator()() & {
            return value;
        }
        constexpr T &&operator()() && {
            return std::move(value);
        }

        constexpr const T &get_value() const & {
            return value;
        }
        constexpr T &get_value() & {
            return value;
        }
        constexpr T &&get_value() && {
            return std::move(value);
        }

        std::string_view get_tag(std::string_view key) const {
            const std::string_view tag = this->tag;

            for (size_t pos = 0; pos < tag.size();) {
                // find next ":`"
                const size_t sep = tag.find(":`", pos);
                if (sep == std::string_view::npos)
                    break;

                // left part before ":`"
                std::string_view keys = tag.substr(pos, sep - pos);

                // right part until next backtick
                size_t end = tag.find('`', sep + 2);
                if (end == std::string_view::npos)
                    break;

                std::string_view value = tag.substr(sep + 2, end - (sep + 2));
                for (size_t start = 0; start < keys.size();) {
                    size_t comma = keys.find(',', start);

                    std::string_view one = keys.substr(start, (comma == std::string_view::npos ? keys.size() : comma) - start);
                    if (one == key)
                        return value;

                    if (comma == std::string_view::npos)
                        break;

                    start = comma + 1;
                }

                // advance past this key–value pair
                pos = end + 1;
                // skip whitespace if any
                while (pos < tag.size() && tag[pos] == ' ')
                    pos++;
            }

            return {};
        }
    };

    template <typename T>
    struct is_tagged : std::false_type {};

    template <typename T>
    struct is_tagged<Tag<T>> : std::true_type {};

    template <typename T>
    inline static constexpr bool is_tagged_v = is_tagged<T>::value;


    template <typename T>
    struct remove_tag;

    template <typename T>
    struct remove_tag<Tag<T>> {
        using type = typename Tag<T>::type;
    };

    template <typename T>
    using remove_tag_t = typename remove_tag<T>::type;
} // namespace cppxx

#endif
