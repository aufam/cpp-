#ifndef CPPXX_SERDE_ERROR_H
#define CPPXX_SERDE_ERROR_H

#include <exception>
#include <string>

namespace cppxx::serde {
    class error : public std::exception {
    protected:
        mutable std::string what_;

    public:
        std::string context;
        std::string msg;

        explicit error(std::string msg)
            : msg(std::move(msg)) {}

        error(std::string_view key, std::string msg)
            : context("." + std::string(key))
            , msg(std::move(msg)) {}

        error(size_t key, std::string msg)
            : context("[" + std::to_string(key) + "]")
            , msg(std::move(msg)) {}

        error &add_context(std::string_view key) & {
            context = "." + std::string(key) + std::move(context);
            return *this;
        }

        error &add_context(size_t key) & {
            context = "[" + std::to_string(key) + "]" + std::move(context);
            return *this;
        }

        const char *what() const noexcept override {
            what_ = "Error at " + (context.empty() ? "<root>" : context) + ": " + msg;
            return what_.c_str();
        }
    };

    class type_mismatch_error : public error {
    public:
        std::string expected_type;

        type_mismatch_error(const std::string &expected_type, const std::string &got)
            : error("Type mismatch error: expect `" + expected_type + "` got `" + got + "`")
            , expected_type(expected_type) {}
    };

    class size_mismatch_error : public error {
    public:
        size_mismatch_error(size_t expect, size_t got)
            : error("Size mismatch error: expect `" + std::to_string(expect) + "` got `" + std::to_string(got) + "`") {}
    };
} // namespace cppxx::serde

#endif
