#include <cpp++/sql/sqlite3.h>
#include <gtest/gtest.h>

namespace sql = cppxx::sql;

namespace {
    struct User {
        static constexpr const char *TableName = "Users";

        sql::Column<int>         id   = "sql:`id integer primary key`";
        sql::Column<std::string> name = "sql:`name varchar(32) not null`";
        sql::Column<int>         age  = "sql:`age integer`";
    };

    struct Employee {
        static constexpr const char *TableName = "Employees";

        sql::Column<int>         id     = "sql:`id integer primary key`";
        sql::Column<std::string> name   = "sql:`name varchar(32) not null`";
        sql::Column<int>         salary = "sql:`salary integer`";
        sql::Column<int>         bonus  = "sql:`bonus integer`";

        int total_income() const {
            return salary() + bonus();
        }
    };
} // namespace

TEST(sqlite3, workflow) {
    const User     users{};
    const Employee employees{};
    const auto     total_income = sql::Alias("total_income");

    struct {
        User     u;
        Employee e;
    } Wibowo, Sucipto, Marwoto;

    sql::sqlite3::Connection db(":memory:");

    {
        auto stmt = sql::create_table<User>;
        ASSERT_NO_THROW(db(stmt));
    }

    {
        auto stmt = sql::create_table<Employee>;
        ASSERT_NO_THROW(db(stmt));
    }

    {
        Wibowo.u.id()   = 1;
        Wibowo.u.name() = "Wibowo";
        Wibowo.u.age()  = 25;

        Sucipto.u.id()   = 2;
        Sucipto.u.name() = "Sucipto";
        Sucipto.u.age()  = 30;

        Marwoto.u.id()   = 3;
        Marwoto.u.name() = "Marwoto";
        Marwoto.u.age()  = 18;

        auto stmt = sql::insert_into<User>(users.name, users.age)
                        .values(
                            std::tuple{Wibowo.u.name(), Wibowo.u.age()},
                            std::tuple{Sucipto.u.name(), Sucipto.u.age()},
                            std::tuple{Marwoto.u.name(), Marwoto.u.age()}
                        );
        ASSERT_NO_THROW(db(stmt));
    }

    {
        Wibowo.e.id()     = 1;
        Wibowo.e.name()   = Wibowo.u.name();
        Wibowo.e.salary() = Wibowo.u.age();
        Wibowo.e.bonus()  = Wibowo.u.age();

        Sucipto.e.id()     = 2;
        Sucipto.e.name()   = Sucipto.u.name();
        Sucipto.e.salary() = Sucipto.u.age();
        Sucipto.e.bonus()  = Sucipto.u.age();

        Marwoto.e.id()     = 3;
        Marwoto.e.name()   = Marwoto.u.name();
        Marwoto.e.salary() = Marwoto.u.age();
        Marwoto.e.bonus()  = Marwoto.u.age();

        auto stmt = sql::insert_into<Employee>(employees.name, employees.salary, employees.bonus)
                        .select(users.name, users.age, users.age)
                        .from(users);
        ASSERT_NO_THROW(db(stmt));
    }

    {
        auto stmt = sql::update<Employee>.set(employees.salary = employees.salary * 3);
        EXPECT_NO_THROW(db(stmt));
        Wibowo.e.salary() *= 3;
        Sucipto.e.salary() *= 3;
        Marwoto.e.salary() *= 3;
    }

    {
        auto stmt = sql::update<User>.set(users.age = users.age + 3).where(users.name == "Marwoto");
        EXPECT_NO_THROW(db(stmt));
        Marwoto.u.age() += 3;
    }

    {
        auto stmt = sql::update<Employee>.set(employees.bonus = 100).where(employees.name == "Sucipto");
        EXPECT_NO_THROW(db(stmt));
        Sucipto.e.bonus() = 100;
    }

    {
        auto stmt = sql::select_all_from(users).order_by(users.age.desc);
        auto rows = db(stmt);

        std::vector<decltype(stmt)::row_type> result;
        for (; !rows.is_done(); rows.next())
            result.push_back(rows.get());
        ASSERT_EQ(result.size(), 3);

        std::multimap<int, User *, std::greater<>> expect = {
            {Wibowo.u.age(),  &Wibowo.u },
            {Sucipto.u.age(), &Sucipto.u},
            {Marwoto.u.age(), &Marwoto.u}
        };
        ASSERT_EQ(result.size(), expect.size());

        for (auto [ep, rp] = std::make_pair(expect.begin(), result.begin()); rp != result.end(); ++ep, ++rp) {
            auto &user            = *ep->second;
            auto &[id, name, age] = *rp;

            ASSERT_EQ(user.id(), id);
            ASSERT_EQ(user.name(), name);
            ASSERT_EQ(user.age(), age);
        }
    }

    {
        const auto stmt = sql::select(
                              employees.id,
                              employees.name,
                              employees.salary,
                              employees.bonus,
                              (employees.salary + employees.bonus).as(total_income)
        )
                              .from(employees)
                              .order_by(total_income);

        auto rows = db(stmt);

        std::vector<decltype(stmt)::row_type> result;
        for (; !rows.is_done(); rows.next())
            result.push_back(rows.get());
        ASSERT_EQ(result.size(), 3);

        std::multimap<int, Employee *, std::less<>> expect = {
            {Wibowo.e.total_income(),  &Wibowo.e },
            {Sucipto.e.total_income(), &Sucipto.e},
            {Marwoto.e.total_income(), &Marwoto.e}
        };
        ASSERT_EQ(result.size(), expect.size());

        for (auto [ep, rp] = std::make_pair(expect.begin(), result.begin()); rp != result.end(); ++ep, ++rp) {
            auto &[expected_total_income, employee]       = *ep;
            auto &[id, name, salary, bonus, total_income] = *rp;

            ASSERT_EQ(employee->id(), id);
            ASSERT_EQ(employee->name(), name);
            ASSERT_EQ(employee->salary(), salary);
            ASSERT_EQ(employee->bonus(), bonus);
            ASSERT_EQ(expected_total_income, total_income);
        }
    }
}
