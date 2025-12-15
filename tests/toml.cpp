#include <cpp++/toml/marzer_toml.h>
#include <gtest/gtest.h>

using namespace cppxx;

struct Person {
    // required fields
    Tag<std::string> name = "toml:`name`";
    Tag<int>         age  = "toml:`age`";

    // optional field
    Tag<std::optional<std::string>> address = "toml:`address`";

    // fallback if missing
    Tag<std::string> department = {"toml:`department,skipmissing`", "unset"};

    // omit when empty (go-style omitempty)
    Tag<int> salary = "toml:`salary,omitempty`";

    // RFC3339 UTC only: YYYY-MM-DDTHH:MM:SSZ
    Tag<std::tm> created_at = "toml:`createdAt`";

    // ignored field
    int dummy = 42;
};

namespace {

    constexpr const char *toml_full = R"toml(
name = "Sucipto"
age = 24
address = "Jakarta"
department = "Engineering"
salary = 1000
createdAt = 2024-01-02T03:04:05Z
)toml";

    constexpr const char *toml_missing_department = R"toml(
name = "Sucipto"
age = 24
address = "Jakarta"
salary = 1000
createdAt = 2024-01-02T03:04:05Z
)toml";

} // namespace

TEST(cppxx, marzer_toml_parse_full) {
    Person p = cppxx::toml::marzer_toml::parse<Person>(toml_full);

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


TEST(cppxx, marzer_toml_parse_missing_department) {
    Person p = cppxx::toml::marzer_toml::parse<Person>(toml_missing_department);

    ASSERT_TRUE(p.address().has_value());
    EXPECT_EQ(*p.address(), "Jakarta");

    // fallback value
    EXPECT_EQ(p.department(), "unset");
}
