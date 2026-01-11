#ifndef CPPXX_JSON_YYJSON_H
#define CPPXX_JSON_YYJSON_H

#include <cpp++/cli/cli.h>
#include <cpp++/serde/tag_info.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>
#include <cpp++/fmt.h>

#ifndef CXXOPTS_HPP_INCLUDED
#    include <cxxopts.hpp>
#endif

namespace cxxopts {

    template <typename T>
    class values::standard_value<std::optional<T>> : public abstract_value<std::optional<T>> {
    public:
        ~standard_value() override = default;

        standard_value() = default;

        explicit standard_value(std::optional<T> *b)
            : abstract_value<std::optional<T>>(b) {}

        std::shared_ptr<Value> clone() const override {
            return std::make_shared<standard_value<bool>>(*this);
        }
    };
} // namespace cxxopts

namespace cppxx::cli::jarro_cxxopts {
    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<cxxopts::ParseResult, To>;

    using Parse = ::cppxx::serde::Parse<cxxopts::ParseResult, std::pair<int, char **>>;

    class parse_help : public serde::error {
    public:
        using serde::error::error;

        [[nodiscard]]
        const char *what() const noexcept override {
            return msg.c_str();
        }
    };
} // namespace cppxx::cli::jarro_cxxopts

namespace cppxx::serde {
    template <>
    struct Parse<cxxopts::ParseResult, std::pair<int, char **>> {
        std::string app_name;
        int         argc;
        char      **argv;

        template <typename... Ts>
        void into(std::tuple<Ts...> &tpl) const {
            cxxopts::Options         options(argv[0], app_name);
            cxxopts::OptionAdder     add = options.add_options();
            std::vector<std::string> positionals;

            tuple_for_each(tpl, [&](auto &v, size_t) {
                auto &val    = serde::detail::get_underlying_value(v);
                using Tagged = std::decay_t<decltype(v)>;
                using T      = std::decay_t<decltype(val)>;

                if constexpr (is_tagged_v<Tagged>) {
                    serde::TagInfo ti = serde::get_tag_info(v, "opt");
                    add(std::string(ti.key), std::string(ti.help), cxxopts::value<T>(), "enum");
                    if (ti.positional)
                        positionals.emplace_back(ti.key);
                }
            });
            add("h,help", "Print help");

            if (!positionals.empty()) {
                options.parse_positional(positionals);
                options.positional_help("positionals: " + [&, res = std::string()]() mutable {
                    for (auto &positional : positionals)
                        res += positional + ", ";
                    return res;
                }());
                options.show_positional_help();
            }

            try {
                const cxxopts::ParseResult parser = options.parse(argc, argv);
                if (parser.count("help"))
                    throw cli::jarro_cxxopts::parse_help(options.help());

                tuple_for_each(tpl, [&](auto &v, size_t) {
                    auto &val    = serde::detail::get_underlying_value(v);
                    using Tagged = std::decay_t<decltype(v)>;
                    using T      = std::decay_t<decltype(val)>;

                    if constexpr (is_tagged_v<Tagged>) {
                        serde::TagInfo ti = serde::get_tag_info(v, "opt");
                        val               = parser[std::string(ti.key)].as<T>();
                    }
                });
                for (auto &kv : parser) {
                    fmt::println("{} = {}", kv.key(), kv.value());
                }
            } catch (const cxxopts::exceptions::exception &e) {
                throw serde::error(e.what());
            }
        }
    };
} // namespace cppxx::serde

#endif
