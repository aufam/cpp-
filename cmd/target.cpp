#include "context.h"
#include <fmt/format.h>


void string_replace(std::string &str, const std::string &key, const std::string &value) {
    std::string pattern = "{" + key + "}";
    for (size_t pos = str.find(pattern); pos != std::string::npos; pos = str.find(pattern))
        str.replace(pos, pattern.length(), value);
}

void Context::resolve_target(const std::string &name, Target &target) {
    for (auto &str : target.src()) {
        string_replace(str, "version", package().version());
        for (auto &[name, dep] : dependencies()) {
            auto &d = convert_dep(dep);
            string_replace(str, name, d.path());
        }
    }
    for (auto &str : target.inc()) {
        string_replace(str, "version", package().version());
        for (auto &[name, dep] : dependencies()) {
            auto &d = convert_dep(dep);
            string_replace(str, name, d.path());
        }
    }
    for (auto &str : target.flags()) {
        string_replace(str, "version", package().version());
        for (auto &[name, dep] : dependencies()) {
            auto &d = convert_dep(dep);
            string_replace(str, name, d.path());
        }
    }
    for (auto &str : target.link_flags()) {
        string_replace(str, "version", package().version());
        for (auto &[name, dep] : dependencies()) {
            auto &d = convert_dep(dep);
            string_replace(str, name, d.path());
        }
    }
}
