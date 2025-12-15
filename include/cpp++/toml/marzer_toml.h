#ifndef CPPXX_TOML_MARZER_TOML_H
#define CPPXX_TOML_MARZER_TOML_H

#include <cpp++/toml/toml.h>
#include <cpp++/serde/deserialize.h>
#include <array>
#include <variant>
#include <tuple>
#include <unordered_map>
#include <ctime>

#ifndef TOMLPLUSPLUS_HPP
#    include <toml++/toml.h>
#endif

#ifndef NEARGYE_MAGIC_ENUM_HPP
#    include <magic_enum/magic_enum.hpp>
#endif

namespace cppxx::toml::marzer_toml {
    struct Deserializer;

    template <typename To, typename Enable = void>
    using Deserialize = ::cppxx::serde::Deserialize<Deserializer, To, Enable>;

    template <typename T>
    void parse(const std::string &str, T &val);

    template <typename T>
    void parse_from_file(const std::string &path, T &val);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse(const std::string &str);

    template <typename T>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<T>, T> parse_from_file(const std::string &path);
} // namespace cppxx::toml::marzer_toml

namespace cppxx::toml::marzer_toml {
    class error;

    struct Deserializer {
        const ::toml::node    *node;
        ::cppxx::toml::TagInfo t = {};

        using error = ::cppxx::toml::marzer_toml::error;

        template <typename T>
        static constexpr bool is_aggregate_contains_tagged_v = cppxx::toml::detail::is_aggregate_contains_tagged_v<T>;
    };

    class error : public std::exception {
        mutable std::string what_;

    public:
        std::string context;
        std::string msg;

        explicit error(std::string msg)
            : msg(std::move(msg)) {}

        error(std::string_view key, std::string msg)
            : context("." + std::string(key))
            , msg(std::move(msg)) {}

        error(size_t key, std::string msg)
            : context("[" + std::to_string(key) + "]")
            , msg(std::move(msg)) {}

        error add_context(std::string_view key) & {
            error e(std::move(msg));
            e.context = "." + std::string(key) + std::move(context);
            return e;
        }

        error add_context(size_t key) & {
            error e(std::move(msg));
            e.context = "[" + std::to_string(key) + "]" + std::move(context);
            return e;
        }

        const char *what() const noexcept override {
            what_ = "marzer_toml error at " + (context.empty() ? "<root>" : context) + ": " + msg;
            return what_.c_str();
        }

        static error type_mismatch_error(const char *expect, const ::toml::node *node) {
            return error(
                "Type mismatch error: expect `" + std::string(expect) + "` got `" +
                std::string(magic_enum::enum_name(node->type())) + "`"
            );
        }

        static error size_mismatch_error(size_t expect, size_t got) {
            return error("Size mismatch error: expect `" + std::to_string(expect) + "` got `" + std::to_string(got) + "`");
        }
    };
} // namespace cppxx::toml::marzer_toml


namespace cppxx::toml::marzer_toml::detail {
    template <typename S, typename Enable = std::enable_if_t<cppxx::toml::detail::is_toml_table<S>::value>>
    void parse(const std::string &str, S &s, bool from_file) {
        auto doc = from_file ? ::toml::parse_file(str) : ::toml::parse(str);
        if constexpr (cppxx::toml::detail::is_aggregate_contains_tagged_v<S>) {
            boost::pfr::for_each_field(s, [&](auto &field) {
                using T = std::decay_t<decltype(field)>;
                if constexpr (cppxx::is_tagged_v<T>) {
                    if (auto [key, ti] = cppxx::toml::TagInfo::from_tagged(field); key != "") {
                        auto *node = doc.contains(key) ? &doc.at(key) : nullptr;
                        try {
                            Deserialize<cppxx::remove_tag_t<T>>{node, ti}.into(field.get_value());
                        } catch (error &e) {
                            throw e.add_context(key);
                        }
                    }
                }
            });
        } else {
            using T = typename cppxx::toml::detail::is_toml_table<S>::key_type;
            for (auto [key, node] : doc) {
                auto item = T{};
                try {
                    Deserialize<T>{node}.into(item);
                } catch (error &e) {
                    throw e.add_context(key);
                }
                s.emplace(std::string(key), std::move(item));
            }
        }
    }
} // namespace cppxx::toml::marzer_toml::detail

namespace cppxx::toml::marzer_toml {
    template <typename S>
    void parse(const std::string &doc, S &s) {
        detail::parse(doc, s, false);
    }

    template <typename S>
    void parse_from_file(const std::string &path, S &s) {
        detail::parse(path, s, true);
    }

    template <typename S>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<S>, S> parse(const std::string &doc) {
        S s = {};
        detail::parse(doc, s, false);
        return s;
    }

    template <typename S>
    [[nodiscard]]
    std::enable_if_t<std::is_default_constructible_v<S>, S> parse_from_file(const std::string &path) {
        S s = {};
        detail::parse(path, s, true);
        return s;
    }
} // namespace cppxx::toml::marzer_toml


#define DESERIALIZER ::cppxx::toml::marzer_toml::Deserializer

namespace cppxx::serde {
    template <>
    struct Parse<DESERIALIZER, std::string> {
        const std::string &str;

        template <typename T>
        void into(T &val) const {
            ::cppxx::toml::marzer_toml::parse(str, val);
        }
    };

    template <>
    struct ParseFromFile<DESERIALIZER> {
        const std::string &path;

        template <typename T>
        void into(T &val) const {
            ::cppxx::toml::marzer_toml::parse_from_file(path, val);
        }
    };

    // bool
    template <>
    struct Deserialize<DESERIALIZER, bool> : DESERIALIZER {
        void into(bool &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");
            else if (auto val = node->as_boolean())
                v = val->get();
            else
                throw error::type_mismatch_error("bool", node);
        }
    };

    // int
    template <typename T>
    struct Deserialize<DESERIALIZER, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> : DESERIALIZER {
        void into(T &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");
            else if (auto val = node->as_integer())
                v = (T)val->get();
            else
                throw error::type_mismatch_error("int", node);
        }
    };

    // float
    template <typename T>
    struct Deserialize<DESERIALIZER, T, std::enable_if_t<std::is_floating_point_v<T>>> : DESERIALIZER {
        void into(T &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");
            else if (auto val = node->as_floating_point())
                v = (T)val->get();
            else
                throw error::type_mismatch_error("float", node);
        }
    };

    // string
    template <>
    struct Deserialize<DESERIALIZER, std::string> : DESERIALIZER {
        void into(std::string &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");
            else if (auto val = node->as_string())
                v = val->get();
            else
                throw error::type_mismatch_error("float", node);
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, std::optional<T>, std::enable_if_t<std::is_default_constructible_v<T>>> : DESERIALIZER {
        void into(std::optional<T> &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node) {
                v = std::nullopt;
                return;
            }
            v = T{};
            Deserialize<DESERIALIZER, T>{node}.into(*v);
        }
    };

    // array
    template <typename T, size_t N>
    struct Deserialize<DESERIALIZER, std::array<T, N>> : DESERIALIZER {
        void into(std::vector<T> &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");

            auto arr = node->as_array();
            if (!arr)
                throw error::type_mismatch_error("arr", node);

            const size_t n = arr->size();
            if (n != N)
                throw error::size_mismatch_error(N, n);

            for (size_t i = 0; i < n; ++i)
                try {
                    Deserialize<DESERIALIZER, T>{arr->get(i)}.into(v[i]);
                } catch (error &e) {
                    throw e.add_context(i);
                }
        };
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, std::vector<T>, std::enable_if_t<std::is_default_constructible_v<T>>> : DESERIALIZER {
        void into(std::vector<T> &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");

            auto arr = node->as_array();
            if (!arr)
                throw error::type_mismatch_error("arr", node);

            const size_t n = arr->size();
            v.resize(n);
            for (size_t i = 0; i < n; ++i)
                try {
                    Deserialize<DESERIALIZER, T>{arr->get(i)}.into(v[i]);
                } catch (error &e) {
                    throw e.add_context(i);
                }
        };
    };

    template <typename... T>
    struct Deserialize<DESERIALIZER, std::tuple<T...>> : DESERIALIZER {
        void into(std::tuple<T...> &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");

            auto arr = node->as_array();
            if (!arr)
                throw error::type_mismatch_error("arr", node);

            int i = 0;
            std::apply(
                [&](auto &...item) {
                    (
                        [&]() {
                            try {
                                Deserialize<DESERIALIZER, std::decay_t<decltype(item)>>{arr->get(i)}.into(item);
                            } catch (error &e) {
                                throw e.add_context(i);
                            }
                            i++;
                        }(),
                        ...
                    );
                },
                v
            );
        }
    };

    template <typename... T>
    struct Deserialize<DESERIALIZER, std::variant<T...>> : DESERIALIZER {
        void into(std::variant<T...> &v) const {
            if (!node && t.skipmissing)
                return;
            try_for_each(v, std::index_sequence_for<T...>{});
        }

    protected:
        template <size_t... I>
        void try_for_each(std::variant<T...> &v, std::index_sequence<I...>) const {
            if (!node)
                throw error("Fatal error: node is empty");

            bool done = false;
            (
                [&]() {
                    using Elem = std::tuple_element_t<I, std::tuple<T...>>;
                    try {
                        if (!done) {
                            auto element = Elem{};
                            Deserialize<DESERIALIZER, Elem>{node}.into(element);
                            v    = std::move(element);
                            done = true;
                        }
                    } catch (...) {
                    }
                }(),
                ...
            );
            if (!done)
                throw error::type_mismatch_error("variant", node);
        }
    };

    // table
    template <typename T>
    struct Deserialize<DESERIALIZER, std::unordered_map<std::string, T>> : DESERIALIZER {
        void into(std::unordered_map<std::string, T> &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");

            auto table = node->as_table();
            if (!table)
                throw error::type_mismatch_error("table", node);

            for (auto [key, node] : *table) {
                auto item = T{};
                try {
                    Deserialize<DESERIALIZER, T>{&node}.into(item);
                } catch (error &e) {
                    throw e.add_context(std::string_view(key));
                }
                v.emplace(std::string(key), std::move(item));
            }
        }
    };

    template <typename S>
    struct Deserialize<DESERIALIZER, S, std::enable_if_t<DESERIALIZER::is_aggregate_contains_tagged_v<S>>> : DESERIALIZER {
        void into(S &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");

            auto table = node->as_table();
            if (!table)
                throw error::type_mismatch_error("table", node);

            boost::pfr::for_each_field(v, [&](auto &field) {
                using T = std::decay_t<decltype(field)>;
                if constexpr (cppxx::is_tagged_v<T>) {
                    if (auto [tag, ti] = cppxx::toml::TagInfo::from_tagged(field); tag != "") {
                        auto node = table->contains(tag) ? &table->at(tag) : nullptr;
                        try {
                            Deserialize<DESERIALIZER, remove_tag_t<T>>{node, ti}.into(field.get_value());
                        } catch (error &e) {
                            throw e.add_context(tag);
                        }
                    }
                }
            });
        }
    };

    // enum
    template <typename S>
    struct Deserialize<DESERIALIZER, S, std::enable_if_t<std::is_enum_v<S>>> : DESERIALIZER {
        void into(S &v) const {
            if (!node && t.skipmissing)
                return;

            auto str = std::string();
            Deserialize<DESERIALIZER, std::string>{node}.into(str);

            auto e = magic_enum::enum_cast<S>(str);
            if (!e.has_value()) {
                std::string what = "invalid value `" + str + "`, expected one of {";
                for (std::string_view name : magic_enum::enum_names<S>()) {
                    what += std::string(name) + ",";
                }
                what += "}";
                throw error(std::move(what));
            }
            v = *e;
        }
    };

    // std::tm
    template <>
    struct Deserialize<DESERIALIZER, std::tm> : DESERIALIZER {
        void into(std::tm &v) const {
            if (!node && t.skipmissing)
                return;
            if (!node)
                throw error("Fatal error: node is empty");
            else if (auto val = node->as_time())
                to_tm(val->get(), v);
            else if (auto val = node->as_date())
                to_tm(val->get(), v);
            else if (auto val = node->as_date_time()) {
                to_tm(val->get().time, v);
                to_tm(val->get().date, v);
            } else
                throw error::type_mismatch_error("time", node);
        }

        static void to_tm(const ::toml::date &d, std::tm &tm) {
            tm.tm_year = d.year - 1900; // tm_year is years since 1900
            tm.tm_mon  = d.month - 1;   // tm_mon is 0–11
            tm.tm_mday = d.day;
        }

        static void to_tm(const ::toml::time &t, std::tm &tm) {
            tm.tm_hour = t.hour;
            tm.tm_min  = t.minute;
            tm.tm_sec  = t.second;
        }
    };
} // namespace cppxx::serde

#undef DESERIALIZER
#endif
