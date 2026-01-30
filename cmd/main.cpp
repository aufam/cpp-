#include <cpp++/proto/protobuf.h>
#include <cpp++/toml/marzer_toml.h>
#include <cpp++/json/yy_json.h>
#include <cpp++/cli/cli11.h>
#include <cpp++/defer.h>
#include <cpp++/fmt.h>
#include "config.h"

template <typename T>
using Tag = cppxx::Tag<T>;

enum class Level { High = 10, Medium, Low };
struct Data {
    Tag<float>       pi    = {"protobuf:`1`  fmt,toml,opt:`pi,skipmissing`    ", 0.314f};
    Tag<int>         num   = " protobuf:`2`  fmt,toml,opt:`num`               ";
    Tag<std::string> hello = {"protobuf:`3`  fmt,toml,opt:`hello`             ", "hello world"};
    Tag<Level>       level = " protobuf:`4`  fmt,toml,opt:`level`             ";

    Tag<std::variant<int, std::string>> var = "fmt,toml,opt:`var`";

    struct Subdata {
        Tag<std::vector<std::string>> names = "protobuf:`1` fmt,toml:`names` opt:`names,positional`";
    };
    Tag<Subdata> sub = "protobuf:`5` fmt,toml:`sub` opt:`sub,positional,help=This is subcommand`";

    // always be omitted since no serialize/deserialize overload for this type
    Tag<std::nullopt_t> null      = {"protobuf:`32`  fmt,toml,opt:`asdfsa`", std::nullopt};
    Tag<std::monostate> monostate = "fmt:`monostate`";
};

int main(int argc, char **argv) {
    Config cfg;
    cppxx::toml::marzer_toml::parse_from_file("./cpp++.toml", cfg);

    cfg.cache() = "/home/aufa/.cache/cpp++";
    cfg.resolve();

    // std::cout << cppxx::json::yy_json::dump(cfg, YYJSON_WRITE_PRETTY_TWO_SPACES) << std::endl;
    return 0;

    Data data;
    fmt::println("default = {}", data);

    // protobuf
    {
        std::string serialized_data = cppxx::proto::protobuf::dump(data);
        fmt::println("proto = {:02x}", fmt::join(serialized_data, " "));
    }

    // toml
    {
        const char *config_path = "temp.toml";
        const char *config      = R"toml(
            num = 42
            hello = "hello from toml"
            level = "Medium"
            var = 10

            [sub]
            names = ["Bowo", "Charles"]
        )toml";

        FILE *f = fopen(config_path, "wb");
        fwrite(config, 1, strlen(config), f);
        fclose(f);
        cppxx::defer _ = [config_path]() { remove(config_path); };

        cppxx::toml::marzer_toml::parse_from_file(config_path, data);
        fmt::println("toml = {}", data);
    }

    // argparse
    {
        cppxx::cli::cli11::parse("app description", argc, argv, data);
        fmt::println("args = {}", data);
    }

    return 0;
}
