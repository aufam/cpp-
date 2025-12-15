#include <cpp++/sql/sql.h>
#include <gtest/gtest.h>

namespace sql = cppxx::sql;

namespace {
    struct User {
        static constexpr const char *TableName = "Users";

        sql::Column<int>         id   = "sql:`id integer primary key`";
        sql::Column<std::string> name = "sql:`name varchar(32) not null`";
        sql::Column<int>         age  = "sql:`age integer`";
    };

    struct Product {
        static constexpr const char *TableName = "Products";

        sql::Column<int>    id    = "sql:`id integer primary key`";
        sql::Column<double> price = "sql:`price real`";
        sql::Column<int>    stock = "sql:`stock integer`";
    };
} // namespace

TEST(sql, create_table) {
    auto u = sql::create_table<User>;
    EXPECT_EQ(u.query, "create table Users (id integer primary key, name varchar(32) not null, age integer)");
    EXPECT_EQ(u.params, std::tuple<>{});
    EXPECT_EQ(decltype(u)::row_type{}, std::tuple<>{});

    auto p = sql::create_table<Product>;
    EXPECT_EQ(p.query, "create table Products (id integer primary key, price real, stock integer)");
    EXPECT_EQ(p.params, std::tuple<>{});
    EXPECT_EQ(decltype(p)::row_type{}, std::tuple<>{});
}


TEST(sql, update) {
    const Product products;

    auto p = sql::update<Product>.set(products.price = 9.99, products.stock = products.stock - 10).where(products.id == 42);
    EXPECT_EQ(p.query, "update Products set price = ?, stock = stock - ? where id = ?");
    EXPECT_EQ(p.params, (std::tuple<double, int, int>{9.99, 10, 42}));
    EXPECT_EQ(decltype(p)::row_type{}, std::tuple<>{});
}

TEST(sql, insert) {
    const User users;

    auto u = sql::insert_into<User>(users.name, users.age).values(std::tuple{"Sucipto", 20}, std::tuple{"Sugeng", 25});
    EXPECT_EQ(u.query, "insert into Users (name, age) values (?, ?), (?, ?)");
    EXPECT_EQ(u.params, (std::tuple<std::string, int, std::string, int>{"Sucipto", 20, "Sugeng", 25}));
    EXPECT_EQ(decltype(u)::row_type{}, std::tuple<>{});
}


TEST(sql, select) {
    const User    users;
    const Product products;

    auto s = sql::select_all_from(users).where(users.name == "Sugeng");
    EXPECT_EQ(s.query, "select * from Users where name = ?");
    EXPECT_EQ(s.params, (std::tuple<std::string>{"Sugeng"}));
    EXPECT_EQ(decltype(s)::row_type{}, (std::tuple<int, std::string, int>{0, "", 0}));

    auto p = sql::select(products.price, products.stock)
                 .from(products)
                 .where(products.price > 4.99 or products.stock <= 10)
                 .order_by(products.stock, products.price.desc);
    EXPECT_EQ(p.query, "select price, stock from Products where (price > ? or stock <= ?) order by stock, price desc");
    EXPECT_EQ(p.params, (std::tuple<double, int>{4.99, 10}));
    EXPECT_EQ(decltype(p)::row_type{}, (std::tuple<double, int>{0.0, 0}));
}

TEST(sql, insert_from_select) {
    const User    users;
    const Product products;

    auto s =
        sql::insert_into<User>(users.id, users.age).select(products.id, products.stock).from(products).where(products.id == 42);
    EXPECT_EQ(s.query, "insert into Users (id, age) select id, stock from Products where id = ?");
    EXPECT_EQ(s.params, (std::tuple<int>{42}));
    EXPECT_EQ(decltype(s)::row_type{}, std::tuple<>{});
}
