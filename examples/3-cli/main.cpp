#include <cpp++/fmt.h>
#include <cpp++/cli/cli11.h>

template <typename T>
using Tag = cppxx::Tag<T>;

struct App {
    Tag<std::string> my_name = "fmt:`name`    opt:`n|name,    help=Name to greet, env=NAME`";
    Tag<bool>        verbose = "fmt:`verbose` opt:`v|verbose, help=Enable verbose output`";

    enum LogLevel { Debug, Info, Warn };
    Tag<LogLevel> log_level = {"fmt,opt:`log-level, skipmissing, help=Log level`", Info};

    struct Greet {};
    Tag<Greet> greet = "fmt,opt:`greet,help=Greet the name`";

    struct Add {
        Tag<int> num1 = "fmt,opt:`num1,positional,help=First number`";
        Tag<int> num2 = "fmt,opt:`num2,positional,help=Second number`";
    };
    Tag<Add> add = "fmt,opt:`add,help=Add two numbers`";
};

int main(int argc, char **argv) {
    auto [app, subcommands] = cppxx::cli::cli11::parse_with_subcommands<App>("cli example", argc, argv);

    if (app.verbose())
        fmt::println("app = {}", app);

    if (subcommands.empty()) {
        fmt::println("{}: You are {}", app.log_level(), app.my_name());
    } else if (const std::string &sub = subcommands.front(); sub == "greet") {
        fmt::println("{}: Hello {}", app.log_level(), app.my_name());
    } else if (sub == "add") {
        auto [a, b] = std::tuple(app.add().num1(), app.add().num2());
        fmt::println("{}: {} + {} = {}", app.log_level(), a, b, a + b);
    }
}
