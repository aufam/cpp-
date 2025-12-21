#ifndef CPPXX_SQL_SQLITE3_H
#define CPPXX_SQL_SQLITE3_H

#include <cpp++/sql/sql.h>
#include <cpp++/serde/serialize.h>
#include <cpp++/serde/deserialize.h>
#include <string>
#include <optional>
#include <ctime>

#ifndef SQLITE3_H
#    include <sqlite3.h>
#endif


/*
 * Forward declarations
 */
namespace cppxx::sql::sqlite3 {
    class Connection;

    template <typename Row>
    class Rows;

    struct Serializer;

    struct Deserializer;

    template <typename From, typename Enable = void>
    using Serialize = ::cppxx::serde::Serialize<Serializer, From, Enable>;

    template <typename To, typename Enable = void>
    using Deserialize = ::cppxx::serde::Deserialize<Deserializer, To, Enable>;

    class error;

    struct Serializer {
        sqlite3_stmt *stmt;
        int           index;

        using error = cppxx::sql::sqlite3::error;
    };

    struct Deserializer {
        sqlite3_stmt *stmt;
        int           index;

        using error = cppxx::sql::sqlite3::error;
    };
} // namespace cppxx::sql::sqlite3


/*
 * Helper declarations
 */
namespace cppxx::sql::sqlite3 {
    class error : public std::exception {
        mutable std::string what_;

    public:
        std::string title, content, query;
        int         ret;

        explicit error(
            const std::string &title, const std::string &content = "", const std::string &query = "", int ret = SQLITE_OK
        )
            : title(title)
            , content(content)
            , query(query)
            , ret(ret) {}

        const char *what() const noexcept override {
            what_ = title;
            if (!content.empty()) {
                what_ += ": ";
                what_ += content;
            }
            bool has_query = false;
            if (!query.empty()) {
                has_query = true;
                what_ += ": query=\"";
                what_ += query;
                what_ += '\"';
            }
            if (ret != SQLITE_OK) {
                what_ += has_query ? " ret=" : ": ret=";
                what_ += std::to_string(ret);
            }
            return what_.c_str();
        }
    };
} // namespace cppxx::sql::sqlite3


/*
 * Implementations
 */
namespace cppxx::sql::sqlite3 {
    template <typename Row>
    class Rows : public cppxx::sql::Rows<Row> {
        friend class Connection;

    protected:
        Rows(struct sqlite3 *db, sqlite3_stmt *stmt, const std::string &query)
            : db(db)
            , stmt(stmt)
            , query(query) {
            try {
                next();
            } catch (...) {
                sqlite3_finalize(stmt);
                throw;
            }
        }

    public:
        Rows(const Rows &) = delete;

        Rows(Rows &&other) noexcept
            : db(other.db)
            , stmt(std::exchange(other.stmt, nullptr))
            , query(std::move(other.query))
            , ret(std::exchange(other.ret, SQLITE_DONE)) {}

        virtual ~Rows() {
            if (stmt) {
                sqlite3_finalize(stmt);
                stmt = nullptr;
            }
        }

        void next() override {
            ret = sqlite3_step(stmt);
            if (ret != SQLITE_DONE && ret != SQLITE_ROW)
                throw error("Failed to step", sqlite3_errmsg(db), query, ret);
        }

        Row get() const override {
            if (ret != SQLITE_ROW)
                throw error("Failed to step", "not a row", query, ret);

            return get_all(std::make_index_sequence<std::tuple_size_v<Row>>());
        }

        bool is_done() const override {
            return ret == SQLITE_DONE;
        }

        sqlite3_int64 get_last_insert_rowid() const {
            return sqlite3_last_insert_rowid(db);
        }

    protected:
        struct sqlite3 *db;
        sqlite3_stmt   *stmt;
        std::string     query;
        int             ret;

        template <std::size_t... I>
        auto get_all(std::index_sequence<I...>) const {
            return std::make_tuple(Deserialize<std::tuple_element_t<I, Row>>{stmt, int(I)}.into()...);
        }
    };

    class Connection : public cppxx::sql::Connection {
    public:
        Connection(const Connection &) = delete;

        Connection(Connection &&other) noexcept
            : db(std::exchange(other.db, nullptr))
            , stmt(std::exchange(other.stmt, nullptr)) {}

        Connection(const std::string &filename) {
            int ret = sqlite3_open(filename.c_str(), &db);
            if (ret != SQLITE_OK) {
                std::string content = sqlite3_errmsg(db);
                sqlite3_close(db);
                throw error("Cannot open \"" + filename + "\"", content, "", ret);
            }
        }

        template <typename Params, typename Row>
        Rows<Row> operator()(const Statement<Params, Row> &statement) {
            int ret = sqlite3_prepare_v2(db, statement.query.c_str(), -1, &stmt, nullptr);
            if (ret != SQLITE_OK) {
                std::string content = sqlite3_errmsg(db);
                throw error("Failed to prepare statement", content, statement.query, ret);
            }

            std::apply(
                [&](auto &&...args) {
                    int i = 1;
                    (Serialize<std::decay_t<decltype(args)>>{stmt, i++}.from(args), ...);
                },
                statement.params
            );

            return {db, stmt, statement.query};
        }

        ~Connection() override {
            sqlite3_close(db);
        }

    protected:
        struct sqlite3      *db;
        struct sqlite3_stmt *stmt;
    };
} // namespace cppxx::sql::sqlite3


#define SERIALIZER   ::cppxx::sql::sqlite3::Serializer
#define DESERIALIZER ::cppxx::sql::sqlite3::Deserializer

/*
 * Helper Implementations
 */
namespace cppxx::serde {
    template <>
    struct Serialize<SERIALIZER, int> : SERIALIZER {
        void from(int value) const {
            sqlite3_bind_int(stmt, index, value);
        }
    };

    template <typename T>
    struct Serialize<SERIALIZER, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, int>>> : SERIALIZER {
        void from(sqlite3_int64 value) const {
            sqlite3_bind_int64(stmt, index, value);
        }
    };

    template <>
    struct Serialize<SERIALIZER, std::tm> : SERIALIZER {
        void from(std::tm value) const {
#if defined(_WIN32)
            auto t = _mkgmtime(&value);
#else
            auto t = ::timegm(&value);
#endif
            sqlite3_bind_int64(stmt, index, t);
        }
    };

    template <>
    struct Serialize<SERIALIZER, double> : SERIALIZER {
        void from(double value) const {
            sqlite3_bind_double(stmt, index, value);
        }
    };

    template <>
    struct Serialize<SERIALIZER, std::string> : SERIALIZER {
        void from(const std::string &value) const {
            sqlite3_bind_text(stmt, index, value.c_str(), (int)value.size(), SQLITE_STATIC);
        }
    };

    template <>
    struct Serialize<SERIALIZER, std::vector<uint8_t>> : SERIALIZER {
        void from(const std::vector<uint8_t> &value) const {
            sqlite3_bind_blob(stmt, index, (void *)value.data(), (int)value.size(), SQLITE_STATIC);
        }
    };

    template <typename T>
    struct Serialize<SERIALIZER, std::optional<T>> : SERIALIZER {
        void from(const std::optional<T> &value) const {
            if (!value.has_value())
                sqlite3_bind_null(stmt, index);
            else
                Serialize<SERIALIZER, T>{stmt, index}.from(*value);
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, int> : DESERIALIZER {
        int into() const {
            return sqlite3_column_int(stmt, index);
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, int>>> : DESERIALIZER {
        T into() const {
            return (T)sqlite3_column_int64(stmt, index);
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, std::tm> : DESERIALIZER {
        std::tm into() const {
            const auto t   = Deserialize<DESERIALIZER, std::time_t>{stmt, index}.into();
            std::tm    out = {};
#if defined(_WIN32)
            _gmtime64_s(&out, &t);
#else
            ::gmtime_r(&t, &out);
#endif
            return out;
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, double> : DESERIALIZER {
        double into() const {
            return sqlite3_column_double(stmt, index);
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, std::string> : DESERIALIZER {
        std::string into() const {
            return (const char *)sqlite3_column_text(stmt, index);
        }
    };

    template <>
    struct Deserialize<DESERIALIZER, std::vector<uint8_t>> : DESERIALIZER {
        std::vector<uint8_t> into() const {
            const auto *data = static_cast<const uint8_t *>(sqlite3_column_blob(stmt, index));
            const int   size = sqlite3_column_bytes(stmt, index);
            if (!data || size <= 0)
                return {};
            return {data, data + size};
        }
    };

    template <typename T>
    struct Deserialize<DESERIALIZER, std::optional<T>> : DESERIALIZER {
        std::optional<T> into(sqlite3_stmt *stmt, int index) const {
            if (sqlite3_column_type(stmt, index) == SQLITE_NULL)
                return std::nullopt;
            else
                return Deserialize<DESERIALIZER, T>{stmt, index}.into();
        }
    };
} // namespace cppxx::serde

#undef SERIALIZER
#undef DESERIALIZER
#endif
