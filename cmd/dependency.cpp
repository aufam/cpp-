#include "context.h"
#include <cpp++/defer.h>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h>
#include <filesystem>

namespace fs = std::filesystem;


void Context::resolve_remote_dep(const std::string &name, Context::Dep &dep) {
    auto &d = convert_dep(dep);

    const bool default_target_is_not_defined = targets().find("default") == targets().end();

    auto &default_target = targets()["default"];

    if (!d.path().empty()) {
        spdlog::info("resolving path of {}: {}", name, d.path());
        d.path() = resolve_path(cache(), d.path());
    } else if (!d.git().empty()) {
        auto tag = d.tag().empty() ? d.branch() : d.tag();
        spdlog::info("resolving git of {}: {} {}", name, d.git(), tag);
        d.path() = git_clone(cache(), d.git(), tag);
    } else if (d.version().empty()) {
        throw std::runtime_error("path|git|version is not defined");
    } else {
        spdlog::info("resolving version of {}: {}", name, d.version());
        auto it = packages().find(name);
        if (it == packages().end())
            throw std::runtime_error("Cannot find `" + name + "` in the package list");

        auto p                  = it->second;
        p.cache()               = cache();
        p.no_default_features() = !d.default_features().value_or(true);
        p.package().version()   = d.version();

        apply_version_to_packages(d.version(), p);

        p.resolve_feats(d.features());

        for (auto &[target_name, t] : p.targets()) {
            if (target_name != "default") {
                targets()[name + "/" + target_name] = t;
            } else {
                targets()[name] = t;
            }
        }

        if (!p.no_default_features()) {
            default_target += targets()[name];
        }

        return;
    }

    if (default_target_is_not_defined) {
        Target t;
        if (fs::is_directory(d.path() + "/src"))
            t.src() = {"{" + name + "}" + "/src/*"};
        if (fs::is_directory(d.path() + "/include"))
            t.inc() = {"public:{" + name + "}" + "/include"};
        default_target += t;
    }
}

Dependency &convert_dep(Context::Dep &dep) {
    if (auto *version = std::get_if<std::string>(&dep)) {
        Dependency d{};
        d.version() = *version;
        dep         = d;
    }

    return std::get<Dependency>(dep);
}

void apply_version_to_packages(const std::string &version, Context &dep_package) {
    for (auto &[_, dep] : dep_package.dependencies()) {
        auto &d = convert_dep(dep);
        string_replace(d.version(), "version", version);
        string_replace(d.path(), "version", version);
        string_replace(d.url(), "version", version);
        string_replace(d.git(), "version", version);
        string_replace(d.branch(), "version", version);
        string_replace(d.tag(), "version", version);
        string_replace(d.subdir(), "version", version);
    }

    for (auto &[_, feats] : dep_package.features()) {
        for (auto &feat : feats)
            string_replace(feat, "version", version);
    }

    for (auto &[_, t] : dep_package.targets()) {
        for (auto &o : t.src())
            string_replace(o, "version", version);
        for (auto &o : t.inc())
            string_replace(o, "version", version);
        for (auto &o : t.flags())
            string_replace(o, "version", version);
        for (auto &o : t.link_flags())
            string_replace(o, "version", version);
    }

    for (auto &t : dep_package.bin()) {
        for (auto &o : t.src())
            string_replace(o, "version", version);
        for (auto &o : t.inc())
            string_replace(o, "version", version);
        for (auto &o : t.flags())
            string_replace(o, "version", version);
        for (auto &o : t.link_flags())
            string_replace(o, "version", version);
    }

    if (auto &t = dep_package.lib(); t.has_value()) {
        for (auto &o : t->src())
            string_replace(o, "version", version);
        for (auto &o : t->inc())
            string_replace(o, "version", version);
        for (auto &o : t->flags())
            string_replace(o, "version", version);
        for (auto &o : t->link_flags())
            string_replace(o, "version", version);
    }
}
