#ifndef CPPXX_CLI_CLI11_H
#define CPPXX_CLI_CLI11_H

#include <cpp++/cli/cli.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>

#include <CLI/CLI.hpp>
#include <variant>

#ifndef BOOST_PFR_HPP
#    if __has_include(<boost/pfr.hpp>)
#        include <boost/pfr.hpp>
#    endif
#endif

#ifndef NEARGYE_MAGIC_ENUM_HPP
#    if __has_include(<magic_enum/magic_enum.hpp>)
#        include <magic_enum/magic_enum.hpp>
#    endif
#endif

namespace cppxx::cli::cli11 {
    using Parse = ::cppxx::serde::Parse<CLI::App, std::pair<int, char **>>;

    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<CLI::App, To>;

    template <typename To>
    using is_deserializable = ::cppxx::serde::is_deserializable<CLI::App, To>;

    template <typename T>
    void parse(const std::string &app_desc, int argc, char **argv, T &v);

    template <typename T>
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &app_desc, int argc, char **argv);

} // namespace cppxx::cli::cli11

namespace cppxx::cli::cli11::detail {
    inline std::string convert_flag_format(std::string_view input, bool positional) {
        if (input.size() <= 1 || input[1] != '|')
            return (positional ? "" : "--") + std::string(input);
        else if (positional)
            return std::string(input.substr(2));
        else
            return "-" + std::string(input.substr(0, 1)) + ",--" + std::string(input.substr(2));
    }

    class DeserializeDispatcher {
    public:
        explicit DeserializeDispatcher(CLI::App &app)
            : app(app) {}

        CLI::App   &app;
        std::string option_name;
        std::string help_string;
        bool        positional = false;
        bool        required   = false;
    };

    template <typename T>
    class DeserializeDispatcherFor : public DeserializeDispatcher {
        using DeserializeDispatcher::DeserializeDispatcher;

        virtual void into(T &v) const = 0;
    };
} // namespace cppxx::cli::cli11::detail


namespace cppxx::serde {
    template <>
    struct Parse<CLI::App, std::pair<int, char **>> {
        std::string    app_desc;
        int            argc;
        mutable char **argv;

        template <typename... Ts>
        void into(std::tuple<Ts...> &tpl) const {
            CLI::App app{app_desc, argv[0]};
            argv = app.ensure_utf8(argv);
            app.allow_windows_style_options();

            tuple_for_each(tpl, [&](auto &v, size_t) {
                auto &val    = serde::detail::get_underlying_value(v);
                using Tagged = std::decay_t<decltype(v)>;
                using T      = std::decay_t<decltype(val)>;

                if constexpr (is_tagged_v<Tagged> && is_deserializable_v<CLI::App, T>) {
                    serde::TagInfo ti = cppxx::cli::get_tag_info(v);
                    if (ti.key.empty())
                        return;

                    Deserialize<CLI::App, T> d(app);
                    d.option_name = cppxx::cli::cli11::detail::convert_flag_format(ti.key, ti.positional);
                    d.help_string = std::string(ti.help);
                    d.positional  = ti.positional;
                    d.required    = !ti.skipmissing;
                    d.into(val);
                }
            });

            try {
                app.parse(argc, argv);
            } catch (const CLI::ParseError &e) {
                ::exit(app.exit(e));
            };
        }

#ifdef BOOST_PFR_HPP
        template <typename S>
        std::enable_if_t<std::is_aggregate_v<S>> into(S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            into(tpl);
        }
#endif
    };

    template <>
    struct Deserialize<CLI::App, bool> : public cli::cli11::detail::DeserializeDispatcherFor<bool> {
        using cli::cli11::detail::DeserializeDispatcherFor<bool>::DeserializeDispatcherFor;

        void into(bool &v) const override {
            this->app.add_flag(this->option_name, v, this->help_string);
        }
    };

    template <typename T>
    struct Deserialize<
        CLI::App,
        T,
        std::enable_if_t<(std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) && !std::is_same_v<T, bool>>>
        : public cli::cli11::detail::DeserializeDispatcherFor<T> {
        using cli::cli11::detail::DeserializeDispatcherFor<T>::DeserializeDispatcherFor;

        void into(T &v) const override {
            CLI::Option *opt = this->app.add_option(this->option_name, v, this->help_string);
            if (this->required)
                opt->required(this->required);
            else
                opt->default_val(v);
        }
    };

    template <typename T>
    struct Deserialize<
        CLI::App,
        std::vector<T>,
        std::enable_if_t<(std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) && !std::is_same_v<T, bool>>>
        : public cli::cli11::detail::DeserializeDispatcherFor<std::vector<T>> {
        using cli::cli11::detail::DeserializeDispatcherFor<std::vector<T>>::DeserializeDispatcherFor;

        void into(std::vector<T> &v) const override {
            CLI::Option *opt = this->app.add_option(this->option_name, v, this->help_string);
            if (!v.empty())
                opt->default_val(v);
        }
    };

    template <typename T>
    struct Deserialize<
        CLI::App,
        std::optional<T>,
        std::enable_if_t<(std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) && !std::is_same_v<T, bool>>>
        : public cli::cli11::detail::DeserializeDispatcherFor<std::optional<T>> {
        using cli::cli11::detail::DeserializeDispatcherFor<std::optional<T>>::DeserializeDispatcherFor;

        void into(std::optional<T> &v) const override {
            CLI::Option *opt = this->app.template add_option<std::optional<T>, T>(this->option_name, v, this->help_string);
            if (v.has_value())
                opt->default_val(*v);
        }
    };


    template <typename... T>
    struct Deserialize<
        CLI::App,
        std::variant<T...>,
        std::enable_if_t<(((std::is_arithmetic_v<T> || std::is_same_v<T, std::string>) && !std::is_same_v<T, bool>) && ...)>>
        : public cli::cli11::detail::DeserializeDispatcherFor<std::variant<T...>> {
        using cli::cli11::detail::DeserializeDispatcherFor<std::variant<T...>>::DeserializeDispatcherFor;

        void into(std::variant<T...> &v) const override {
            CLI::Option *opt = this->app.template add_option_function<std::string>(
                this->option_name,
                [&v, this](const std::string &str) {
                    bool done = false;
                    (
                        [&]() {
                            if (!done) {
                                auto element = T{};
                                done         = CLI::detail::lexical_cast(str, element);
                                if (done)
                                    v = std::move(element);
                            }
                        }(),
                        ...
                    );
                    if (!done)
                        throw type_mismatch_error("variant", "unknown"); // TODO
                },
                this->help_string
            );

            if (this->required)
                opt->required(this->required);
            else
                std::visit([opt](auto &val) { opt->default_val(val); }, v);
        }
    };

    template <typename... Ts>
    struct Deserialize<CLI::App, std::tuple<Ts...>> : public cli::cli11::detail::DeserializeDispatcherFor<std::tuple<Ts...>> {
        using cli::cli11::detail::DeserializeDispatcherFor<std::tuple<Ts...>>::DeserializeDispatcherFor;

        void into(std::tuple<Ts...> &tpl) const override {
            CLI::App *sub = this->app.add_subcommand(this->option_name, this->help_string);

            tuple_for_each(tpl, [&](auto &v, size_t) {
                auto &val    = serde::detail::get_underlying_value(v);
                using Tagged = std::decay_t<decltype(v)>;
                using T      = std::decay_t<decltype(val)>;

                if constexpr (is_tagged_v<Tagged> && is_deserializable_v<CLI::App, T>) {
                    serde::TagInfo ti = cppxx::cli::get_tag_info(v);
                    if (ti.key.empty())
                        return;

                    Deserialize<CLI::App, T> d(*sub);
                    d.option_name = cppxx::cli::cli11::detail::convert_flag_format(ti.key, ti.positional);
                    d.help_string = std::string(ti.help);
                    d.positional  = ti.positional;
                    d.required    = !ti.skipmissing;
                    d.into(val);
                }
            });
        }
    };

#ifdef BOOST_PFR_HPP
    template <typename S>
    struct Deserialize<CLI::App, S, std::enable_if_t<std::is_aggregate_v<S>>>
        : public cli::cli11::detail::DeserializeDispatcherFor<S> {
        using cli::cli11::detail::DeserializeDispatcherFor<S>::DeserializeDispatcherFor;

        void into(S &v) const override {
            CLI::App *sub = this->app.add_subcommand(this->option_name, this->help_string);

            auto tpl = boost::pfr::structure_tie(v);

            tuple_for_each(tpl, [&](auto &v, size_t) {
                auto &val    = serde::detail::get_underlying_value(v);
                using Tagged = std::decay_t<decltype(v)>;
                using T      = std::decay_t<decltype(val)>;

                if constexpr (is_tagged_v<Tagged> && is_deserializable_v<CLI::App, T>) {
                    serde::TagInfo ti = cppxx::cli::get_tag_info(v);
                    if (ti.key.empty())
                        return;

                    Deserialize<CLI::App, T> d(*sub);
                    d.option_name = cppxx::cli::cli11::detail::convert_flag_format(ti.key, ti.positional);
                    d.help_string = std::string(ti.help);
                    d.positional  = ti.positional;
                    d.required    = !ti.skipmissing;
                    d.into(val);
                }
            });
        }
    };

#endif

#ifdef NEARGYE_MAGIC_ENUM_HPP
    template <typename S>
    struct Deserialize<CLI::App, S, std::enable_if_t<std::is_enum_v<S>>>
        : public cli::cli11::detail::DeserializeDispatcherFor<S> {
        using cli::cli11::detail::DeserializeDispatcherFor<S>::DeserializeDispatcherFor;

        void into(S &v) const override {
            CLI::Option *opt = generic_into(v, this->app, this->option_name, this->help_string);
            if (this->required)
                opt->required(this->required);
            else
                opt->default_str(std::string(magic_enum::enum_name(v)));
        }

        enum class Method { Assign, Emplace, EmplaceBack };
        template <Method m = Method::Assign, typename T>
        static CLI::Option *generic_into(T &v, CLI::App &app, const std::string &option_name, const std::string &help_string) {
            CLI::Option *opt = app.template add_option_function<std::string>(
                option_name,
                [&v](const std::string &str) {
                    auto e = magic_enum::enum_cast<S>(str);
                    if (!e.has_value()) {
                        std::string what = "invalid value `" + str + "`, expected one of {";
                        for (auto &name : magic_enum::enum_names<S>()) {
                            what += std::string(name) + ",";
                        }
                        what += "}";
                        throw error(std::move(what));
                    }
                    if constexpr (m == Method::Emplace)
                        v.emplace(*e);
                    else if constexpr (m == Method::EmplaceBack)
                        v.emplace_back(*e);
                    else
                        v = *e;
                },
                help_string
            );

            std::vector<std::string> enum_names;
            for (auto &name : magic_enum::enum_names<S>()) {
                enum_names.push_back(std::string(name));
            }
            opt->check(CLI::IsMember(enum_names));

            return opt;
        }
    };

    template <typename S>
    struct Deserialize<CLI::App, std::optional<S>, std::enable_if_t<std::is_enum_v<S>>>
        : public cli::cli11::detail::DeserializeDispatcherFor<std::optional<S>> {
        using cli::cli11::detail::DeserializeDispatcherFor<std::optional<S>>::DeserializeDispatcherFor;

        void into(std::optional<S> &v) const override {
            CLI::Option *opt = Deserialize<CLI::App, S>::template generic_into<Deserialize<CLI::App, S>::Method::Emplace>(
                v, this->app, this->option_name, this->help_string
            );

            if (v.has_value())
                opt->default_str(std::string(magic_enum::enum_name(*v)));
        }
    };

    // TODO
    // template <typename S>
    // struct Deserialize<CLI::App, std::vector<S>, std::enable_if_t<std::is_enum_v<S>>>
    //     : public cli::cli11::detail::DeserializeDispatcherFor<std::vector<S>> {
    //     using cli::cli11::detail::DeserializeDispatcherFor<std::vector<S>>::DeserializeDispatcherFor;
    //
    //     void into(std::vector<S> &v) const override {
    //         CLI::Option *opt = Deserialize<CLI::App, S>::template generic_into<Deserialize<CLI::App,
    //         S>::Method::EmplaceBack>(
    //             v, this->app, this->option_name, this->help_string
    //         );
    //         opt->expected(0, magic_enum::enum_names<S>().size());
    //
    //         if (this->required)
    //             opt->required(this->required);
    //         else if (!v.empty()) {
    //             // std::vector<std::string> enum_names;
    //             // for (auto &e : v) {
    //             //     enum_names.push_back(std::string(magic_enum::enum_name(e)));
    //             // }
    //             // opt->default_val(enum_names);
    //         }
    //     }
    // };
#endif
} // namespace cppxx::serde

namespace cppxx::cli::cli11 {
    template <typename T>
    void parse(const std::string &app_desc, int argc, char **argv, T &v) {
        Parse{app_desc, argc, argv}.into(v);
    }

    template <typename T>
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &app_desc, int argc, char **argv) {
        T v;
        Parse{app_desc, argc, argv}.into(v);
        return v;
    }
} // namespace cppxx::cli::cli11
#endif
