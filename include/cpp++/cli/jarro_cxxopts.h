#ifndef CPPXX_JSON_YYJSON_H
#define CPPXX_JSON_YYJSON_H

#include <cpp++/cli/cli.h>
#include <cpp++/serde/tag_info.h>
#include <cpp++/serde/deserialize.h>
#include <cpp++/serde/error.h>

#ifndef CXXOPTS_HPP_INCLUDED
#    include <cxxopts.hpp>
#endif

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

namespace cppxx::cli::jarro_cxxopts {
    template <typename To>
    using Deserialize = ::cppxx::serde::Deserialize<cxxopts::OptionValue, To>;

    using Parse = ::cppxx::serde::Parse<cxxopts::Options, std::pair<int, char **>>;

    class parse_help : public serde::error {
    public:
        using serde::error::error;

        [[nodiscard]]
        const char *what() const noexcept override {
            return msg.c_str();
        }
    };
} // namespace cppxx::cli::jarro_cxxopts


namespace cppxx::cli::jarro_cxxopts::detail {
    inline std::string convert_flag_format(std::string_view tag) {
        if (tag.size() <= 1 || tag[1] != '|')
            return std::string(tag);
        else
            return std::string(tag.substr(0, 1)) + "," + std::string(tag.substr(2));
    }

    inline std::string get_parse_result_key(const std::string &flag) {
        if (flag.size() <= 1 || flag[1] != ',')
            return flag;
        else
            return flag.substr(2);
    }
} // namespace cppxx::cli::jarro_cxxopts::detail


namespace cppxx::serde {
    template <>
    struct Parse<cxxopts::Options, std::pair<int, char **>> {
        std::string app_desc;
        int         argc;
        char      **argv;

        template <typename... Ts>
        void into(std::tuple<Ts...> &tpl) const {
            cxxopts::Options     options(argv[0], app_desc);
            cxxopts::OptionAdder add = options.add_options();

            std::vector<std::string> positionals;

            tuple_for_each(tpl, [&](auto &v, size_t) {
                auto &val    = serde::detail::get_underlying_value(v);
                using Tagged = std::decay_t<decltype(v)>;
                using T      = std::decay_t<decltype(val)>;

                if constexpr (is_tagged_v<Tagged> && is_deserializable_v<cxxopts::OptionValue, T>) {
                    TagInfo ti = cli::get_tag_info(v);
                    if (ti.key.empty())
                        return;

                    std::string cxxopts_flag = cli::jarro_cxxopts::detail::convert_flag_format(ti.key);
                    std::string cxxopts_key  = cli::jarro_cxxopts::detail::get_parse_result_key(cxxopts_flag);

                    using D       = Deserialize<cxxopts::OptionValue, T>;
                    bool required = !(ti.skipmissing || D::skipmissing());

                    std::shared_ptr<cxxopts::Value> cxxopts_val = cxxopts::value<typename D::cxxopts_type>();
                    std::string                     default_str = required ? "" : D::default_value(val);
                    add(cxxopts_flag,
                        std::string(ti.help) + (default_str.empty() ? "" : (". Default = " + default_str)),
                        cxxopts_val,
                        D::type_str() + (required ? " REQUIRED" : ""));
                    if (ti.positional)
                        positionals.emplace_back(cxxopts_key);
                }
            });
            add("h,help", "Print help");

            if (!positionals.empty()) {
                std::string str = [&]() {
                    std::string res = "<";
                    for (auto &positional : positionals)
                        res += positional + ", ";
                    res.pop_back();
                    res.back() = '>';
                    return res;
                }();

                options.parse_positional(positionals);
                options.positional_help(str);
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

                    if constexpr (is_tagged_v<Tagged> && is_deserializable_v<cxxopts::OptionValue, T>) {
                        TagInfo ti = cli::get_tag_info(v);
                        if (ti.key.empty())
                            return;

                        std::string cxxopts_flag = cli::jarro_cxxopts::detail::convert_flag_format(ti.key);
                        std::string cxxopts_key  = cli::jarro_cxxopts::detail::get_parse_result_key(cxxopts_flag);

                        if (parser.count(cxxopts_key) || !(ti.skipmissing || Deserialize<cxxopts::OptionValue, T>::skipmissing()))
                            try {
                                Deserialize<cxxopts::OptionValue, T>{parser[cxxopts_key]}.into(val);
                            } catch (std::exception &e) {
                                throw error(ti.key, e.what());
                            }
                    }
                });
            } catch (const cxxopts::exceptions::exception &e) {
                throw error(e.what());
            }
        }

#ifdef BOOST_PFR_HPP
        template <typename S>
        std::enable_if_t<std::is_aggregate_v<S>> into(S &v) const {
            auto tpl = boost::pfr::structure_tie(v);
            into(tpl);
        }
#endif
    };

    template <typename T>
    struct Deserialize<cxxopts::OptionValue, T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_same_v<T, std::string>>> {
        using cxxopts_type = T;

        static std::string default_value(const T &value) {
            if constexpr (std::is_same_v<T, std::string>)
                return value;
            else if constexpr (std::is_same_v<T, bool>)
                return value ? "true" : "false";
            else
                return std::to_string(value);
        }
        static std::string type_str() {
            if constexpr (std::is_same_v<T, std::string>)
                return "string";
            else if constexpr (std::is_same_v<T, bool>)
                return "bool";
            else if constexpr (std::is_same_v<T, char>)
                return "char";
            else if constexpr (std::is_integral_v<T>)
                return "int";
            else
                return "float";
        }
        static bool skipmissing() {
            return false;
        }
        static T convert(cxxopts_type &v) {
            return std::move(v);
        }

        const cxxopts::OptionValue &val;

        void into(T &v) const {
            v = val.as<cxxopts_type>();
        }
    };

    template <typename T>
    struct Deserialize<cxxopts::OptionValue, std::vector<T>, std::enable_if_t<is_deserializable_v<cxxopts::OptionValue, T>>> {
        using cxxopts_type = std::vector<typename Deserialize<cxxopts::OptionValue, T>::cxxopts_type>;

        static std::string default_value(const std::vector<T> &arr) {
            if (arr.empty())
                return "";

            std::string res = "[";
            for (auto &value : arr)
                res += Deserialize<cxxopts::OptionValue, T>::default_value(value) + ", ";
            res.pop_back();
            res.back() = ']';
            return res;
        }
        static std::string type_str() {
            return "[]" + Deserialize<cxxopts::OptionValue, T>::type_str();
        }
        static bool skipmissing() {
            return false;
        }
        static std::vector<T> convert(cxxopts_type &arr) {
            std::vector<T> res;
            for (auto &v : arr)
                res.push_back(Deserialize<cxxopts::OptionValue, T>::convert(v));
            return res;
        }

        const cxxopts::OptionValue &val;

        void into(std::vector<T> &arr) const {
            auto cxxopts_arr = val.as<cxxopts_type>();
            arr              = convert(cxxopts_arr);
        }
    };

    template <typename T>
    struct Deserialize<cxxopts::OptionValue, std::optional<T>, std::enable_if_t<is_deserializable_v<cxxopts::OptionValue, T>>> {
        using cxxopts_type = T;

        static std::string default_value(const std::optional<T> &value) {
            if (!value.has_value())
                return "";
            return Deserialize<cxxopts::OptionValue, T>::default_value(*value);
        }
        static std::string type_str() {
            return Deserialize<cxxopts::OptionValue, T>::type_str() + "?";
        }
        static bool skipmissing() {
            return true;
        }
        static std::optional<T> convert(cxxopts_type &v) {
            return v;
        }

        const cxxopts::OptionValue &val;

        void into(std::optional<T> &v) const {
            v = val.as<cxxopts_type>();
        }
    };

#ifdef NEARGYE_MAGIC_ENUM_HPP
    template <typename S>
    struct Deserialize<cxxopts::OptionValue, S, std::enable_if_t<std::is_enum_v<S>>>
        : Deserialize<cxxopts::OptionValue, std::string> {

        static std::string default_value(const S &value) {
            return std::string(magic_enum::enum_name(value));
        }
        static std::string type_str() {
            std::string str;
            for (auto &name : magic_enum::enum_names<S>()) {
                str += "\"" + std::string(name) + "\"|";
            }
            str.pop_back();
            return str;
        }
        static S convert(cxxopts_type &str) {
            auto e = magic_enum::enum_cast<S>(str);
            if (!e.has_value()) {
                std::string what = "invalid value `" + str + "`, expect " + type_str();
                throw error(std::move(what));
            }
            return *e;
        }

        const cxxopts::OptionValue &val;

        void into(S &v) const {
            auto cxxopts_val = val.as<cxxopts_type>();
            v                = convert(cxxopts_val);
        }
    };
#endif
} // namespace cppxx::serde

#endif
