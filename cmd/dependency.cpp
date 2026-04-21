#include "main.h"

Dependency &Dependency::operator+=(const Dependency &other) {
    src().insert(src().end(), other.src().begin(), other.src().end());
    inc().insert(inc().end(), other.inc().begin(), other.inc().end());
    flags().insert(flags().end(), other.flags().begin(), other.flags().end());
    link_flags().insert(link_flags().end(), other.link_flags().begin(), other.link_flags().end());
    return *this;
}
