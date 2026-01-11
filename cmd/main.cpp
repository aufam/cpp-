#include <cpp++/proto/protobuf.h>
#include <cpp++/fmt.h>
#include <cpp++/cli/jarro_cxxopts.h>
#include <iostream>

int main(int argc, char **argv) {
    auto inner = std::make_tuple(
        cppxx::Tag<int>{"               protobuf:`40` fmt:`num`  ", 123},
        cppxx::Tag<std::string>{"       protobuf:`2`  fmt:`hello`", "hello world"},
        cppxx::Tag<std::optional<int>>{"              fmt:`opt`  ", std::nullopt}
    );

    auto data = std::make_tuple(
        cppxx::Tag<float>{"          protobuf:`1` fmt:`pi`   ", 0.314f},
        cppxx::Tag<decltype(inner)>{"protobuf:`2` fmt:`inner`", inner}
    );

    fmt::println("fmt = {}", data);

    std::string serialized_data = cppxx::proto::protobuf::dump(data);

    std::cout << "Serialized data (hex): ";
    for (char c : serialized_data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (static_cast<int>(static_cast<unsigned char>(c))) << " ";
    }
    std::cout << '\n';

    auto args = std::make_tuple(
        cppxx::Tag<int>{"                     opt,fmt:`num`"},
        cppxx::Tag<std::string>{"             opt,fmt:`str`"},
        cppxx::Tag<std::optional<std::string>>{"             opt,fmt:`opt`"},
        cppxx::Tag<std::vector<std::string>>{"opt,fmt:`vec,positional`"}
    );
    try {
        cppxx::cli::jarro_cxxopts::Parse{"test", argc, argv}.into(args);
    } catch (cppxx::cli::jarro_cxxopts::parse_help &e) {
        fmt::println("{}", e.what());
        return 0;
    }

    fmt::println("args = {}", args);

    return 0;
}
