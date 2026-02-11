#include <cpp++/fmt.h>
#include <cpp++/json/yy_json.h>
#include <cpp++/toml/marzer_toml.h>


template <typename T>
using Tag = cppxx::Tag<T>;


struct Data {
    struct Inner {
        Tag<std::string> text = "fmt,json,toml:`text`";
        Tag<float>       pi   = {"fmt,json,toml:`pi,skipmissing`", 3.14f};
    };

    Tag<int>                        number = "fmt,json,toml:`number`";
    Tag<std::tm>                    dt     = "fmt:`dt` json:`dateTime` toml:`date-time`";
    Tag<Inner>                      inner  = "fmt,json,toml:`inner`";
    Tag<std::optional<std::string>> text   = "fmt,json,toml:`text,omitempty`";
    int                             dummy;
};

static_assert(std::is_aggregate_v<Data> && std::is_aggregate_v<Data::Inner>);

int main() {
    Data data; // .dummy is expected to be undefined

    const char *jdoc = R"json({
        "number": 42,
        "dateTime": "2023-10-27T10:00:00Z",
        "inner": {
            "text": "from json"
        }
    })json";

    cppxx::json::yy_json::parse(jdoc, data);
    fmt::println("data = {}", data);
    fmt::println("json = {}", cppxx::json::yy_json::dump(data));


    const char *tdoc = R"toml(
        number = 37
        date-time = 2023-10-27T10:30:00Z

        [inner]
        text = "From toml"
        pi = 3.0
    )toml";

    cppxx::toml::marzer_toml::parse(tdoc, data);
    fmt::println("data = {}", data);
    fmt::println("=== toml ===\n{}", cppxx::toml::marzer_toml::dump(data));
}
