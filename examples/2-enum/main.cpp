#include <cpp++/fmt.h>
#include <cpp++/toml/marzer_toml.h>

template <typename T>
using Tag = cppxx::Tag<T>;

int main() {
    enum class Level { High = 10, Medium, Low };
    std::tuple data = {
        Tag<Level>{"fmt,toml:`level`"},
        Tag<std::string>{"fmt,toml:`text`"},
    };

    // access the field using operator()
    std::get<0>(data)() = Level::High;
    std::get<1>(data)() = "Level is high";

    fmt::println("=== toml ===");
    fmt::println("{}", cppxx::toml::marzer_toml::dump(data));

    fmt::println("=== fmt ===");
    fmt::println("{}", std::get<0>(data));
    fmt::println("{:?}", std::get<1>(data));

    const char *toml_doc = R"toml(
        text = "From toml"
        level = "Unknown"
    )toml";

    fmt::println("=== invalid value ===");
    try {
        cppxx::toml::marzer_toml::parse(toml_doc, data);
    } catch (cppxx::serde::error &e) {
        fmt::println("{}", e.what());
    }
}
