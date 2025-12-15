#ifndef CPPXX_SQL_SQL_H
#define CPPXX_SQL_SQL_H

#include <cpp++/tag.h>
#include <string>
#include <tuple>
#include <optional>

#ifndef BOOST_PFR_HPP
#    include <boost/pfr.hpp>
#endif

// TODO: implement perfect forwarding


/*
 * Forward declarations
 */
namespace cppxx::sql {
    class Connection;

    template <typename Row>
    class Rows;

    template <typename Params, typename Row>
    struct Statement;

    template <typename T>
    class Column;

    template <typename T>
    class Alias;
} // namespace cppxx::sql


/*
 * Specialization of tag traits for sql::Column
 */
namespace cppxx {
    template <typename T>
    struct is_tagged<sql::Column<T>> : std::true_type {};

    template <typename T>
    struct remove_tag<sql::Column<T>> {
        using type = typename sql::Column<T>::type;
    };
} // namespace cppxx


/*
 * Forward declarations of helpers
 */
namespace cppxx::sql::detail {
    // TODO: implement generic?
    struct GenericStatement {};
    struct GenericRows {};

    template <size_t i>
    struct repeated_placeholders;

    template <typename Tuple, template <typename> typename Pred>
    struct filter_tuple;

    template <typename Tuple, template <typename> typename Pred>
    using filter_tuple_t = typename filter_tuple<Tuple, Pred>::type;

    template <typename Tuple, template <typename> typename Pred>
    struct apply_tuple;

    template <typename Tuple, template <typename> typename Pred>
    using apply_tuple_t = typename apply_tuple<Tuple, Pred>::type;
} // namespace cppxx::sql::detail


/*
 * Implementations
 */
namespace cppxx::sql {
    class Connection {
    public:
        virtual ~Connection() = default;

        // TODO: implement generic?
        template <typename Params, typename Row>
        Rows<Row> operator()(const Statement<Params, Row> &statement);

    protected:
        // TODO: implement generic?
        virtual detail::GenericRows execute(const detail::GenericStatement &) {
            return {};
        };
    };

    template <typename Row>
    class Rows {
    public:
        virtual void next()          = 0;
        virtual bool is_done() const = 0;
        virtual Row  get() const     = 0;
        virtual ~Rows()              = default;

        template <typename F>
        void for_each(F &&fn) {
            for (; !is_done(); next()) {
                std::apply(fn, get());
            }
        }
    };

    template <typename Params = std::tuple<>, typename Row = std::tuple<>>
    struct Statement {
        using params_type = Params;
        using row_type    = Row;

        std::string query;
        Params      params = {};

        template <typename Other>
        auto operator+(const Other &other) const {
            return Statement<
                decltype(std::tuple_cat(params_type{}, typename Other::params_type{})),
                decltype(std::tuple_cat(row_type{}, typename Other::row_type{}))>{
                query + other.query, std::tuple_cat(params, other.params)
            };
        }

        template <typename Other>
        auto operator&&(const Other &other) const {
            return Statement<>{"("} + *this + Statement<>{" and "} + other + Statement<>{")"};
        }

        template <typename Other>
        auto operator||(const Other &other) const {
            return Statement<>{"("} + *this + Statement<>{" or "} + other + Statement<>{")"};
        }

        auto operator!() const {
            return Statement<>{"not ("} + *this + Statement<>{")"};
        }

        template <typename Col, typename... Cols>
        auto select(const Col &col, const Cols &...cols) const {
            static_assert(sizeof...(cols) + 1 == std::tuple_size_v<Params>, "Number of values must match the number of columns");
            if constexpr (sizeof...(Cols) == 0)
                return Statement<>{query + " select " + col.name()};
            else
                return Statement<>{query + " select " + col.name() + ((std::string(", ") + cols.name()) + ...)};
        };

        template <typename Other, typename... Rest>
        auto set(const Other &other, const Rest &...rest) const {
            if constexpr (sizeof...(Rest) == 0)
                return *this + Statement<>{" set "} + other;
            else
                return *this + Statement<>{" set "} + other + ((Statement<>{", "} + rest) + ...);
        }

        template <typename T>
        auto from(const T &) const {
            return *this + Statement<>{std::string(" from ") + T::TableName};
        }

        template <typename Col>
        auto to(const Col &col) const {
            return *this + Statement<>{std::string(" to ") + col.name()};
        }

        template <typename Col>
        auto add(const Col &col) const {
            return *this + Statement<>{std::string(" add ") + col.name()};
        }

        template <typename Col>
        auto drop_column(const Col &col) const {
            return *this + Statement<>{std::string(" drop column ") + col.name()};
        }

        template <typename Condition>
        auto where(const Condition &condition) const {
            return *this + Statement<>{" where "} + condition;
        }

        template <typename... Rest>
        auto values(const Params &params, const Rest &...res) const {
            if constexpr (sizeof...(res) == 0)
                return Statement<Params>{
                    query + " values (" + detail::repeated_placeholders<std::tuple_size_v<Params>>::value() + ")", params
                };
            else
                return Statement<Params>{
                           query + " values (" + detail::repeated_placeholders<std::tuple_size_v<Params>>::value() + ")", params
                       } +
                       ((Statement<Params>{", (" + detail::repeated_placeholders<std::tuple_size_v<Params>>::value() + ")", res}
                        ) +
                        ...);
        }

        template <typename Col, typename... Cols>
        auto order_by(const Col &col, const Cols &...cols) const {
            if constexpr (sizeof...(Cols) == 0)
                return *this + Statement<>{" order by " + col.name()};
            else
                return *this + Statement<>{" order by " + col.name() + ((std::string(", ") + cols.name()) + ...)};
        };

        auto limit(std::optional<size_t> val) const {
            if (val.has_value())
                return *this + Statement<>{" limit " + std::to_string(*val)};
            else
                return *this;
        };

        auto offset(std::optional<size_t> val) const {
            if (val.has_value())
                return *this + Statement<>{" offset " + std::to_string(*val)};
            else
                return *this;
        };
    };

    template <typename T = void>
    class Constant; // TODO

    template <typename T = void>
    class Alias {
        std::string name_;

    public:
        using type = T;
        explicit Alias(const std::string &name)
            : name_(name) {}
        const std::string name() const {
            return name_;
        }

        template <typename U>
        auto as(const Alias<U> &alias) {
            return Alias<T>(name() + " as " + alias.name());
        }
    };

    template <typename T>
    class Column : public Tag<T> {
    public:
        using Tag<T>::Tag;

        std::string column() const {
            return std::string(this->get_tag("sql"));
        }

        std::string name() const {
            auto col = column();
            auto pos = col.find(' ');
            return pos == std::string::npos ? col : col.substr(0, pos);
        }

        template <typename U>
        auto as(const Alias<T> &alias) {
            return name() + " as " + alias.name();
        }

        template <typename U>
        auto operator+(const Column<U> &other) const {
            return Alias<decltype(std::declval<T>() + std::declval<U>())>('(' + name() + " + " + other.name() + ')');
        }

        template <typename U>
        auto operator-(const Column<U> &other) const {
            return Alias<decltype(std::declval<T>() - std::declval<U>())>(name() + " - " + other.name());
        }

        template <typename U>
        auto operator*(const Column<U> &other) const {
            return Alias<decltype(std::declval<T>() * std::declval<U>())>(name() + " * " + other.name());
        }

        template <typename U>
        auto operator/(const Column<U> &other) const {
            return Alias<decltype(std::declval<T>() / std::declval<U>())>(name() + " / " + other.name());
        }

        Statement<std::tuple<T>> operator=(const T &val) const {
            return {name() + " = ?", {val}};
        }
        Statement<std::tuple<T>> operator+(const T &val) const {
            return {name() + " + ?", {val}};
        }
        Statement<std::tuple<T>> operator-(const T &val) const {
            return {name() + " - ?", {val}};
        }
        Statement<std::tuple<T>> operator*(const T &val) const {
            return {name() + " * ?", {val}};
        }
        Statement<std::tuple<T>> operator/(const T &val) const {
            return {name() + " / ?", {val}};
        }

        Statement<std::tuple<T>> operator==(const T &val) const {
            return {name() + " = ?", {val}};
        }
        Statement<std::tuple<T>> operator!=(const T &val) const {
            return {name() + " != ?", {val}};
        }
        Statement<std::tuple<T>> operator>(const T &val) const {
            return {name() + " > ?", {val}};
        }
        Statement<std::tuple<T>> operator<(const T &val) const {
            return {name() + " < ?", {val}};
        }
        Statement<std::tuple<T>> operator>=(const T &val) const {
            return {name() + " >= ?", {val}};
        }
        Statement<std::tuple<T>> operator<=(const T &val) const {
            return {name() + " <= ?", {val}};
        }

        template <typename Params, typename Row>
        auto operator=(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " = "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator+(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " + "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator-(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " - "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator*(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " * "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator/(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " / "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator==(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " = "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator!=(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " != "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator>(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " > "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator<(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " < "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator>=(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " >= "} + stmt;
        }

        template <typename Params, typename Row>
        auto operator<=(const Statement<Params, Row> &stmt) const {
            return Statement<>{name() + " <= "} + stmt;
        }

        const struct Asc {
            const Column *col;
            std::string   name() const {
                return col->name() + " asc";
            }
        } asc{this};

        const struct Desc {
            const Column *col;
            std::string   name() const {
                return col->name() + " desc";
            }
        } desc{this};
    };

    template <typename T>
    struct Schema {
        using columns_type = detail::apply_tuple_t<
            detail::filter_tuple_t<decltype(boost::pfr::structure_to_tuple(std::declval<T>())), cppxx::is_tagged>,
            cppxx::remove_tag>;

        static std::string name() {
            return T::TableName;
        }

        static std::string columns() {
            std::string res = "(";
            boost::pfr::for_each_field(T{}, [&](auto &&field, size_t i) {
                if constexpr (is_tagged_v<std::decay_t<decltype(field)>>) {
                    if (i == 0)
                        res += std::string(field.get_tag("sql"));
                    else
                        res += std::string(", ") + std::string(field.get_tag("sql"));
                }
            });
            res += ")";
            return res;
        }
    };

    template <typename Table>
    inline static const Statement<> create_table = {
        std::string("create table ") + Schema<Table>::name() + " " + Schema<Table>::columns()
    };

    template <typename Table>
    inline static const Statement<> create_table_if_not_exists = {
        std::string("create table if not exists ") + Schema<Table>::name() + " " + Schema<Table>::columns()
    };

    template <typename Table>
    inline static const Statement<> update = {std::string("update ") + Schema<Table>::name()};

    template <typename Table, typename Col, typename... Cols>
    auto insert_into(const Col &col, const Cols &...cols) {
        if constexpr (sizeof...(Cols) == 0)
            return Statement<std::tuple<typename Col::type>>{
                std::string("insert into ") + Schema<Table>::name() + " (" + col.name() + ")"
            };
        else
            return Statement<std::tuple<typename Col::type, typename Cols::type...>>{
                std::string("insert into ") + Schema<Table>::name() + " (" + col.name() +
                ((std::string(", ") + cols.name()) + ...) + ")"
            };
    }

    template <typename Col, typename... Cols>
    auto select(const Col &col, const Cols &...cols) {
        if constexpr (sizeof...(Cols) == 0)
            return Statement<std::tuple<>, std::tuple<typename Col::type>>{std::string("select ") + col.name()};
        else
            return Statement<std::tuple<>, std::tuple<typename Col::type, typename Cols::type...>>{
                std::string("select ") + col.name() + ((std::string(", ") + cols.name()) + ...)
            };
    };

    template <typename Table>
    auto select_all_from(const Table &) {
        return Statement<std::tuple<>, typename Schema<Table>::columns_type>{"select * from " + Schema<Table>::name()};
    };

    template <typename Table>
    auto delete_from(const Table &) {
        return Statement<std::tuple<>, typename Schema<Table>::columns_type>{"delete from " + Schema<Table>::name()};
    };
} // namespace cppxx::sql


/*
 * Helpers Implementations
 */
namespace cppxx::sql::detail {
    template <size_t i>
    struct repeated_placeholders {
        static std::string value() {
            return repeated_placeholders<i - 1>::value() + ", ?";
        }
    };

    template <>
    struct repeated_placeholders<0> {
        static std::string value() {
            return "";
        }
    };

    template <>
    struct repeated_placeholders<1> {
        static std::string value() {
            return "?";
        }
    };

    template <template <typename> typename Pred, typename... Ts>
    struct filter_tuple<std::tuple<Ts...>, Pred> {
    private:
        template <typename T>
        using keep = std::bool_constant<Pred<T>::value>;

        template <typename, typename>
        struct append;

        template <typename... Us, typename T>
        struct append<std::tuple<Us...>, T> {
            using type = std::tuple<Us..., T>;
        };

        template <typename Result, typename... Rest>
        struct impl;

        template <typename Result>
        struct impl<Result> {
            using type = Result;
        };

        template <typename Result, typename T, typename... Rest>
        struct impl<Result, T, Rest...> {
            using next = std::conditional_t<Pred<T>::value, typename append<Result, T>::type, Result>;
            using type = typename impl<next, Rest...>::type;
        };

    public:
        using type = typename impl<std::tuple<>, Ts...>::type;
    };

    template <template <typename> typename Pred, typename... Ts>
    struct apply_tuple<std::tuple<Ts...>, Pred> {
        using type = std::tuple<typename Pred<Ts>::type...>;
    };
} // namespace cppxx::sql::detail
#endif
