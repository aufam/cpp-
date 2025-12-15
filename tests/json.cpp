#include <cpp++/json/yy_json.h>
#include <cpp++/json/nlohmann_json.h>
#include <gtest/gtest.h>


using namespace cppxx;

struct Person {
    // required fields
    Tag<std::string> name = "json:`name`";
    Tag<int>         age  = "json:`age`";

    // optional field
    Tag<std::optional<std::string>> address = "json:`address`";

    // fallback if missing
    Tag<std::string> department = {"json:`department,skipmissing`", "unset"};

    // omit when empty (go-style omitempty)
    Tag<int> salary = "json:`salary,omitempty`";

    // RFC3339 UTC only: YYYY-MM-DDTHH:MM:SSZ
    Tag<std::tm> created_at = "json:`createdAt`";

    // ignored field
    int dummy = 42;
};

static_assert(std::is_aggregate_v<Person>, "Person must be pure aggregate");

namespace {

    constexpr const char *json_full = R"json(
{
  "name": "Sucipto",
  "age": 24,
  "address": "Jakarta",
  "department": "Engineering",
  "salary": 1000,
  "createdAt": "2024-01-02T03:04:05Z"
}
)json";


    constexpr const char *json_missing_department = R"json(
{
  "name": "Sucipto",
  "age": 24,
  "address": "Jakarta",
  "salary": 1000,
  "createdAt": "2024-01-02T03:04:05Z"
}
)json";

} // namespace


TEST(cppxx, yy_json_parse_full) {
    Person p = json::yy_json::parse<Person>(json_full);

    EXPECT_EQ(p.name(), "Sucipto");
    EXPECT_EQ(p.age(), 24);

    ASSERT_TRUE(p.address().has_value());

    EXPECT_EQ(*p.address(), "Jakarta");
    EXPECT_EQ(p.department(), "Engineering");
    EXPECT_EQ(p.salary(), 1000);

    const std::tm &t = p.created_at();

    EXPECT_EQ(t.tm_year + 1900, 2024);
    EXPECT_EQ(t.tm_mon + 1, 1);
    EXPECT_EQ(t.tm_mday, 2);

    EXPECT_EQ(t.tm_hour, 3);
    EXPECT_EQ(t.tm_min, 4);
    EXPECT_EQ(t.tm_sec, 5);
}


TEST(cppxx, yy_json_parse_missing_department) {
    Person p = json::yy_json::parse<Person>(json_missing_department);

    ASSERT_TRUE(p.address().has_value());
    EXPECT_EQ(*p.address(), "Jakarta");

    // fallback value
    EXPECT_EQ(p.department(), "unset");
}


TEST(cppxx, yy_json_dump_omitempty) {
    Person p;
    p.name() = "Sucipto";
    p.age()  = 24;

    std::string dumped = json::yy_json::dump(p);

    EXPECT_NE(dumped.find("\"name\":\"Sucipto\""), std::string::npos);
    EXPECT_NE(dumped.find("\"age\":24"), std::string::npos);

    EXPECT_NE(dumped.find("\"address\":null"), std::string::npos);
    EXPECT_NE(dumped.find("\"department\":\"unset\""), std::string::npos);

    // omitted fields
    EXPECT_EQ(dumped.find("salary"), std::string::npos);
}


TEST(cppxx, nlohmann_json_parse_full) {
    Person p = nlohmann::json::parse(json_full);

    EXPECT_EQ(p.name(), "Sucipto");
    EXPECT_EQ(p.age(), 24);

    ASSERT_TRUE(p.address().has_value());
    EXPECT_EQ(*p.address(), "Jakarta");

    EXPECT_EQ(p.department(), "Engineering");
    EXPECT_EQ(p.salary(), 1000);
}


TEST(cppxx, nlohmann_json_dump_omitempty) {
    Person p;
    p.name() = "Sucipto";
    p.age()  = 24;

    std::string dumped = nlohmann::json(p).dump();

    EXPECT_NE(dumped.find("\"name\":\"Sucipto\""), std::string::npos);
    EXPECT_NE(dumped.find("\"age\":24"), std::string::npos);

    EXPECT_NE(dumped.find("\"address\":null"), std::string::npos);
    EXPECT_NE(dumped.find("\"department\":\"unset\""), std::string::npos);

    EXPECT_EQ(dumped.find("salary"), std::string::npos);
}


TEST(cppxx, yy_json_round_trip) {
    Person      p1     = json::yy_json::parse<Person>(json_full);
    std::string dumped = json::yy_json::dump(p1);
    Person      p2     = json::yy_json::parse<Person>(dumped);

    EXPECT_EQ(p2.name(), p1.name());
    EXPECT_EQ(p2.age(), p1.age());
    EXPECT_EQ(p2.department(), p1.department());
}
