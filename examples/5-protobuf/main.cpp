#include <cpp++/proto/protobuf.h>
#include <cpp++/fmt.h>

template <typename T>
using Tag = cppxx::Tag<T>;

struct Person {
    Tag<std::string> name  = "fmt:`name`    protobuf:`1`";
    Tag<int>         id    = "fmt:`id`      protobuf:`2`";
    Tag<std::string> email = "fmt:`email`   protobuf:`3`";
    Tag<int>         num1  = "fmt:`num1`    protobuf:`4,fixed`";
    Tag<int>         num2  = "fmt:`num2`    protobuf:`5,zigzag`";
    Tag<int>         num3  = "fmt:`num3`    protobuf:`6`";

    Tag<std::vector<int>> nums = "fmt:`nums`    protobuf:`7`";
};

int main() {
    Person p;
    p.name()  = "Sucipto";
    p.id()    = 42;
    p.email() = "sucipto@makmursejahtera.co.id";
    p.num1()  = 1;
    p.num2()  = -1;
    p.num3()  = 0;
    p.nums()  = {0, 0, 0};
    fmt::println("person = {}", p);

    std::string buf;
    {
        buf = cppxx::proto::protobuf::dump(p);
        fmt::println("proto = {:02x}", fmt::join(buf, " "));
    }
    {
        std::string buf = cppxx::proto::protobuf::dump(std::tie(p.name));
        fmt::println("p.name = {:02x}", fmt::join(buf, " "));
    }
    {
        std::string buf = cppxx::proto::protobuf::dump(std::tie(p.id));
        fmt::println("p.id = {:02x}", fmt::join(buf, " "));
    }
    {
        std::string buf = cppxx::proto::protobuf::dump(std::tie(p.email));
        fmt::println("p.email = {:02x}", fmt::join(buf, " "));
    }
    {
        std::string buf = cppxx::proto::protobuf::dump(std::tie(p.num1));
        fmt::println("p.num1 = {:02x}", fmt::join(buf, " "));
    }
    {
        std::string buf = cppxx::proto::protobuf::dump(std::tie(p.num2));
        fmt::println("p.num2 = {:02x}", fmt::join(buf, " "));
    }
    {
        std::string buf = cppxx::proto::protobuf::dump(std::tie(p.num3));
        fmt::println("p.num3 = {:02x}", fmt::join(buf, " "));
    }
    {
        std::string buf = cppxx::proto::protobuf::dump(std::tie(p.nums));
        fmt::println("p.num3 = {:02x}", fmt::join(buf, " "));
    }

    auto pp = cppxx::proto::protobuf::parse<Person>(buf);
    fmt::println("person = {}", pp);

    auto e = cppxx::proto::protobuf::parse<Person>("");
    fmt::println("empty = {}", e);
}
