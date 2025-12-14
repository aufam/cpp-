#ifndef CPPXX_DEFER_H
#define CPPXX_DEFER_H

#include <utility>

namespace cppxx {
    template <typename F>
    class defer {
    public:
        static_assert(std::is_invocable_v<F>, "F must be invocable");

        defer(F fn)
            : fn(std::move(fn)) {}

        ~defer() {
            fn();
        }

        F fn;
    };
} // namespace cppxx

#endif
