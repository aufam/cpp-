#include <cpp++/fmt.h>
#include <cpp++/json/yy_json.h>
#include <cpp++/toml/marzer_toml.h>


template <typename T>
using Tag = cppxx::Tag<T>;

int main() {
    std::tuple inner = {
        Tag<std::string>{"fmt,json,toml:`text`"},
        Tag<float>{"fmt,json,toml:`pi,skipmissing`", 3.14f}, // skip missing field when deserializing
    };

    std::tuple data = {
        Tag<int>{"fmt,json,toml:`number`"},
        Tag<std::tm>{"fmt:`dt` json:`dateTime` toml:`date-time`"}, // different names for different serde
        Tag<decltype(inner)>{"fmt,json,toml:`inner`", inner}, // sub table
        Tag<std::optional<std::string>>{"fmt,json,toml:`text,omitempty`"}, // omit if value.empty() or nullopt when serializing
        int(40)  // untagged
    };


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
