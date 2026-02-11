#include <cpp++/fmt.h>
#include <cpp++/sql/sql.h>

namespace sql = cppxx::sql;

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

int main() {
    {
        auto stmt = sql::create_table<Product>;
        fmt::println("{}", stmt);
    }
    {
        auto stmt = sql::create_table<User>;
        fmt::println("{}", stmt);
    }

    // just placeholders
    const Product products;
    const User    users;

    {
        auto stmt = sql::update<Product> //
                        .set(products.price = 9.99, products.stock = products.stock - 10)
                        .where(products.id == 42);
        fmt::println("{}", stmt);
    }
    {
        auto stmt = sql::insert_into<User>(users.name, users.age) //
                        .values(std::tuple{"Sucipto", 20}, std::tuple{"Sugeng", 25});
        fmt::println("{}", stmt);
    }
    {
        auto stmt = sql::select_all_from(users).where(users.name == "Sugeng");
        fmt::println("{}", stmt);
    }
    {
        auto stmt = sql::select(products.price, products.stock)
                        .from(products)
                        .where(products.price > 4.99 or products.stock <= 10)
                        .order_by(products.stock, products.price.desc);
        fmt::println("{}", stmt);
    }
    {
        auto stmt = sql::insert_into<User>(users.id, users.age)
                        .select(products.id, products.stock)
                        .from(products)
                        .where(products.id == 42);
        fmt::println("{}", stmt);
    }
}
