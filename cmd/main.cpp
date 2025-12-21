#include <cpp++/json/yy_json.h>
#include <cpp++/json/nlohmann_json.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>

template <typename T>
struct fmt::formatter<cppxx::Tag<T>, char, std::enable_if_t<fmt::is_formattable<T, char>::value>> : fmt::formatter<T> {
    fmt::context::iterator format(const cppxx::Tag<T> &v, fmt::context &c) const {
        if (cppxx::serde::TagInfo ti = cppxx::serde::get_tag_info(v, "fmt"); ti.key != "") {
            fmt::context::iterator out = c.out();

            out = fmt::format_to(out, ti.key);
            out = fmt::format_to(out, "=");
        }
        return fmt::formatter<T>::format(v.get_value(), c);
    }
};

template <typename S>
struct fmt::formatter<S, char, std::enable_if_t<std::is_aggregate_v<S> && !std::is_same_v<S, std::tm>>> : fmt::formatter<int> {
    fmt::context::iterator format(const S &v, fmt::context &c) const {
        fmt::context::iterator out = c.out();
        return fmt::format_to(out, "{}", boost::pfr::structure_tie(v));
    }
};

template <typename T>
struct fmt::formatter<std::optional<T>> : fmt::formatter<T> {
    fmt::context::iterator format(const std::optional<T> &v, fmt::context &c) const {
        if (v.has_value()) {
            return fmt::formatter<T>::format(*v, c);
        }
        fmt::context::iterator out = c.out();
        return fmt::format_to(out, "null");
    }
};

template <typename... T>
struct fmt::formatter<std::variant<T...>> : fmt::formatter<int> {
    fmt::context::iterator format(const std::variant<T...> &v, fmt::context &c) const {
        fmt::context::iterator out = c.out();
        return std::visit([&](const auto &var) { return fmt::format_to(out, "{}", var); }, v);
    }
};

using cppxx::Tag;

struct Person {
    // required fields
    Tag<std::string> name = "json,fmt:`name`";
    Tag<int>         age  = "json,fmt:`age`";

    // optional field
    Tag<std::optional<std::string>> address = "json,fmt:`address`";

    // fallback if missing
    Tag<std::string> department = {"json,fmt:`department,skipmissing`", "unset"};

    // omit when empty (go-style omitempty)
    Tag<int> salary = "json,fmt:`salary,omitempty`";

    // RFC3339 UTC only: YYYY-MM-DDTHH:MM:SSZ
    Tag<std::tm> created_at = "json,fmt:`createdAt`";

    // ignored field
    int dummy = 42;
};

int main() {
    std::tuple<cppxx::Tag<int>, cppxx::Tag<std::variant<int, std::string>>> doc = {
        {"json:`num,omitempty,skipmissing` fmt:`num`"},
        {"json:`test`", 5},
    };

    cppxx::tuple_for_each(doc, [](auto &item, size_t i) { fmt::println("TUPLE FOR EACH TEST {}: {}", i, item); });

    Person p       = {};
    p.created_at() = []() {
        auto t = time(nullptr);
        return *std::gmtime(&t);
    }();

    fmt::println("yyjson p = {}", cppxx::json::yy_json::dump(p));

    fmt::println("yyjson = {}", cppxx::json::yy_json::dump(doc));
    fmt::println("nlohmann_json = {}", nlohmann::json(doc).dump());

    std::get<0>(doc).get_value() = 10;
    std::get<1>(doc).get_value() = "test";

    cppxx::json::yy_json::parse(cppxx::json::yy_json::dump(doc), doc);
    nlohmann::json::parse(cppxx::json::yy_json::dump(doc)).get_to(doc);

    fmt::println("doc = {}", doc);
    fmt::println("person = {}", p);
}
