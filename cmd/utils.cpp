#include "main.h"
#include <algorithm>

Dependency &Dependency::operator+=(const Dependency &other) {
    if (this == &other)
        return *this;

    push_unique(src(), other.src());
    push_unique(inc(), other.inc());
    push_unique(flags(), other.flags());
    push_unique(link_flags(), other.link_flags());
    return *this;
}

bool Dependency::empty() const {
    return version().empty() && path().empty() && url().empty() && git().empty();
}

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

void push_unique(std::vector<std::string> &vec, const std::string &value) {
    if (std::find(vec.begin(), vec.end(), value) == vec.end())
        vec.push_back(value);
}

void push_unique(std::vector<std::string> &vec, const std::vector<std::string> &values) {
    for (const auto &value : values)
        push_unique(vec, value);
}

void Project::apply_package_placeholders() {
    auto &name    = package().name();
    auto &version = package().version();
    auto  edition = std::to_string(package().edition());

    auto apply_dep = [&](Dependency &d) {
        string_replace(d.version(), "version", version);
        string_replace(d.path(), "version", version);
        string_replace(d.url(), "version", version);
        string_replace(d.git(), "version", version);
        string_replace(d.branch(), "version", version);
        string_replace(d.tag(), "version", version);
        string_replace(d.subdir(), "version", version);
        for (auto &str : d.features())
            string_replace(str, "version", version);
        for (auto &str : d.src())
            string_replace(str, "version", version);
        for (auto &str : d.inc())
            string_replace(str, "version", version);
        for (auto &str : d.flags())
            string_replace(str, "version", version);
        for (auto &str : d.link_flags())
            string_replace(str, "version", version);

        string_replace(d.version(), "name", name);
        string_replace(d.path(), "name", name);
        string_replace(d.url(), "name", name);
        string_replace(d.git(), "name", name);
        string_replace(d.branch(), "name", name);
        string_replace(d.tag(), "name", name);
        string_replace(d.subdir(), "name", name);
        for (auto &str : d.features())
            string_replace(str, "name", name);
        for (auto &str : d.src())
            string_replace(str, "name", name);
        for (auto &str : d.inc())
            string_replace(str, "name", name);
        for (auto &str : d.flags())
            string_replace(str, "name", name);
        for (auto &str : d.link_flags())
            string_replace(str, "name", name);

        string_replace(d.version(), "edition", edition);
        string_replace(d.path(), "edition", edition);
        string_replace(d.url(), "edition", edition);
        string_replace(d.git(), "edition", edition);
        string_replace(d.branch(), "edition", edition);
        string_replace(d.tag(), "edition", edition);
        string_replace(d.subdir(), "edition", edition);
        for (auto &str : d.features())
            string_replace(str, "edition", edition);
        for (auto &str : d.src())
            string_replace(str, "edition", edition);
        for (auto &str : d.inc())
            string_replace(str, "edition", edition);
        for (auto &str : d.flags())
            string_replace(str, "edition", edition);
        for (auto &str : d.link_flags())
            string_replace(str, "edition", edition);
    };
    apply_dep(lib());

    for (auto &[_, dep] : dependencies()) {
        apply_dep(convert_dep(dep));
    }

    for (auto &[_, feats] : features()) {
        for (auto &feat : feats) {
            string_replace(feat, "name", name);
            string_replace(feat, "version", version);
            string_replace(feat, "edition", edition);
        }
    }
}
