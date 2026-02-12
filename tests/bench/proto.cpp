#include <benchmark/benchmark.h>
#include <cpp++/proto/protobuf.h>
#include <cpp++/fmt.h>
#include "tests/bench/person.pb.h"

namespace cppxx {
    struct Person {
        Tag<std::string>      name  = "protobuf:`1`";
        Tag<int>              id    = "protobuf:`2`";
        Tag<std::string>      email = "protobuf:`3`";
        Tag<int>              num1  = "protobuf:`4,fixed`";
        Tag<int>              num2  = "protobuf:`5,zigzag`";
        Tag<int>              num3  = "protobuf:`6`";
        Tag<std::vector<int>> nums  = "protobuf:`7`";
    };
} // namespace cppxx

static const char data[] = "\x0a\x07Sucipto"
                           "\x10\x2a"
                           "\x1a\x1dsucipto@makmursejahtera.co.id"
                           "\x25\x01\x00\x00\x00"
                           "\x28\x01"
                           ""
                           "\x3a\x03\x00\x00\x00";

static void cppxx_protobuf_serialization(benchmark::State &state) {
    std::string   buf = std::string(data, sizeof(data) - 1);
    cppxx::Person p;
    cppxx::proto::protobuf::parse(buf, p);
    for (auto _ : state) {
        std::ignore = cppxx::proto::protobuf::dump(p);
    }
}
BENCHMARK(cppxx_protobuf_serialization);

static void google_protobuf_serialization(benchmark::State &state) {
    std::string    buf = std::string(data, sizeof(data) - 1);
    person::Person p;
    if (!p.ParseFromString(buf))
        throw std::runtime_error("protobuf failed to parse");
    for (auto _ : state) {
        std::ignore = p.SerializeAsString();
    }
}
BENCHMARK(google_protobuf_serialization);

static void cppxx_protobuf_deserialization(benchmark::State &state) {
    std::string buf = std::string(data, sizeof(data) - 1);
    for (auto _ : state) {
        cppxx::Person p;
        cppxx::proto::protobuf::parse(buf, p);
    }
}
BENCHMARK(cppxx_protobuf_deserialization);

static void google_protobuf_deserialization(benchmark::State &state) {
    std::string buf = std::string(data, sizeof(data) - 1);
    for (auto _ : state) {
        person::Person p;
        if (!p.ParseFromString(buf))
            throw std::runtime_error("protobuf failed to parse");
    }
}
BENCHMARK(google_protobuf_deserialization);
