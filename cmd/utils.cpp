#include "main.h"
#include <cpp++/defer.h>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>

Dependency &convert_dep(Project::Dep &dep) {
    if (auto *version = std::get_if<std::string>(&dep)) {
        Dependency d{};
        d.version() = *version;
        dep         = d;
    }

    return std::get<Dependency>(dep);
}

void string_replace(std::string &str, const std::string &key, const std::string &value) {
    std::string pattern = "{" + key + "}";
    for (size_t pos = str.find(pattern); pos != std::string::npos; pos = str.find(pattern))
        str.replace(pos, pattern.length(), value);
}
