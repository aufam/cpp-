#include "context.h"
#include <cpp++/defer.h>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h>
#include <filesystem>

namespace fs = std::filesystem;


void Context::resolve_remote_dep(const std::string &name, Context::Dep &dep) {
    // TODO: resolve remote for defaults and resolve_remote for optional

    auto &d = convert_dep(dep);

    const bool default_target_is_not_defined = targets().find(name) == targets().end();

    auto &default_target = convert_feat(targets()[name]);

    if (!d.path().empty()) {
        spdlog::info("resolving path of {}: {}", name, d.path());
        d.path() = resolve_path(cache(), d.path());
    } else if (!d.git().empty()) {
        auto &tag = d.tag().empty() ? d.branch() : d.tag();
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

        try {
            p.build(d.features());
        } catch (const std::exception &e) {
            throw std::runtime_error(
                fmt::format("Error building dependency package={} `{}`: {}", package().name(), name, e.what())
            );
        }

        compile_commands().insert(compile_commands().end(), p.compile_commands().begin(), p.compile_commands().end());
        public_inc().insert(public_inc().end(), p.public_inc().begin(), p.public_inc().end());
        public_flags().insert(public_flags().end(), p.public_flags().begin(), p.public_flags().end());
        link_flags().insert(link_flags().end(), p.link_flags().begin(), p.link_flags().end());

        // TODO: handle default target
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
Target &Context::convert_feat(Context::Feat &feat) {
    if (auto *feats = std::get_if<std::vector<std::string>>(&feat)) {
        Target t{};
        for (auto &name : *feats)
            try {
                t += convert_feat(targets().at(name));
            } catch (std::out_of_range &e) {
                throw std::runtime_error("Cannot find feature `" + name + "`");
            }
        feat = t;
    }

    return std::get<Target>(feat);
}
