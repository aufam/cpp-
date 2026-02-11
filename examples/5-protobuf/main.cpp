#include <cpp++/proto/protobuf.h>
#include <cpp++/fmt.h>

template <typename T>
using Tag = cppxx::Tag<T>;

struct Person {
    Tag<std::string> name  = "fmt:`name`    protobuf:`1`";
    Tag<int>         id    = "fmt:`id`      protobuf:`2`";
    Tag<std::string> email = "fmt:`email`   protobuf:`3`";
};

int main() {
    Person p;
    p.name()  = "Sucipto";
    p.id()    = 42;
    p.email() = "sucipto@makmursejahtera.co.id";
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

    auto pp = cppxx::proto::protobuf::parse<Person>(buf);
    fmt::println("person = {}", pp);
}
