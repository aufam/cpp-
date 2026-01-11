#include <cpp++/proto/protobuf.h>
#include <cpp++/fmt.h>
#include <iostream>

int main() {
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

    // Expected output for {123, "hello_world"} would be:
    // 08 7b 12 0b 68 65 6c 6c 6f 5f 77 6f 72 6c 64
    // (08=tag for field 1 varint, 7b=123; 12=tag for field 2 length-delimited, 0b=length 11, then "hello_world" bytes)

    return 0;
}
