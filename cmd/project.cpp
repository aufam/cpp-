#include "main.h"
#include <algorithm>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <cpp++/toml/toruniina_toml.h>

#define f(...)    fmt::format(__VA_ARGS__)
#define ferr(...) std::runtime_error(fmt::format(__VA_ARGS__))
namespace fs = std::filesystem;

void Project::build(const std::vector<std::string> &features, bool subpackage) {
    if (no_default_features() && features.empty())
        throw ferr("Error building {:?}: no features specified, but `no-default-features` is set", package().name());

    if (features.empty())
        return build({"default"}, subpackage);

    if (package().name().empty())
        throw ferr("Error building {:?}: name is required", package().name());

    if (package().version().empty())
        throw ferr("Error building {:?}: version is required", package().name());

    switch (package().edition()) {
    case 17:
    case 20:
    case 23:
    case 26:
        break;
    default:
        throw ferr("Error building {:?}: unsupported edition: {}", package().name(), package().edition());
    }

    apply_package_placeholders();

    if (!lib().empty())
        resolve_remote_dep(package().name(), lib());
    else if (!subpackage) {
        fs::path working_dir = fs::current_path();
        if (lib().src().empty() && fs::is_directory(working_dir / "src"))
            lib().src() = {"src/*"};
        if (lib().inc().empty() && fs::is_directory(working_dir / "include"))
            lib().inc() = {"public:include"};
    }

    std::vector<std::string> resolved;
    for (auto &[name, dep] : dependencies()) {
        auto &d = convert_dep(dep);
        if (name == "default" && no_default_features())
            continue;
        if (d.optional() && std::find(features.begin(), features.end(), name) == features.end())
            continue;

        try {
            resolve_remote_dep(name, d);
        } catch (const std::exception &e) {
            throw ferr("Error building dependency `{}` of package `{}`: {}", name, package().name(), e.what());
        }
        resolved.push_back(name);
    }

    for (auto &name : resolved) {
        auto &d = convert_dep(dependencies().at(name));
        if (d.empty())
            continue;
        try {
            collect_meta(name, d);
        } catch (const std::exception &e) {
            throw ferr("Error collecting meta of dependency `{}` of package `{}`: {}", name, package().name(), e.what());
        }
    }
}

void Project::resolve_remote_dep(const std::string &name, Dependency &d) {
    constexpr auto toml_version = cppxx::toml::toruniina_toml::spec::v(1, 1, 0);

    auto build_subpackage = [&](Project &p) -> Dependency & {
        p.cache()               = cache();
        p.no_default_features() = !d.default_features().value_or(true);
        p.targets()             = targets();
        try {
            p.build(d.features(), true);
        } catch (const std::exception &e) {
            throw ferr("Error building dependency package={} `{}`: {}", p.package().name(), name, e.what());
        }
        return p.lib();
    };

    if (d.empty()) {
        lib() += d;
        return;
    }

    if (!d.url().empty()) {
        spdlog::info("resolving path of {}: {}", name, d.url());
        d.path() = resolve_path(cache(), d.url());
    } else if (!d.path().empty()) {
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

        auto p = it->second;
        if (p.lib().empty())
            throw std::runtime_error("The package `" + name + "` does not have a library target");

        p.package().version() = d.version();
        auto &lib             = build_subpackage(p);
        lib.version()         = d.version();
        d                     = std::move(lib);
        resolve_remote_dep(name, d);
        return;
    }

    if (auto sub = fs::path(d.path()) / d.subdir() / "cpp++.toml"; fs::exists(sub)) {
        auto  p   = cppxx::toml::toruniina_toml::parse_from_file<Project>(sub.string(), toml_version);
        auto &lib = build_subpackage(p);
        d         = std::move(lib);
    }
}

void Project::collect_meta(const std::string &name, Dependency &d) {
    std::sort(d.features().begin(), d.features().end());
    std::string feature_name = fmt::format("{}", fmt::join(d.features(), "-"));
    if (d.default_features().value_or(true))
        feature_name = "default-" + feature_name;
    if (!feature_name.empty())
        feature_name = "-";

    fs::path working_dir = fs::path(d.path()) / d.subdir();
    if (working_dir.empty())
        throw ferr("working_dir is empty for dep={}", name);

    if (!d.pre().empty()) {
        spdlog::info("running pre command for dep={}: {}", name, d.pre());
        std::string cmd = fmt::format("cd '{}' && {}", working_dir.string(), d.pre());
        if (std::system(cmd.c_str()) != 0)
            throw ferr("pre command failed for dep={}: {}", name, d.pre());
    }

    if (d.src().empty() && fs::is_directory(working_dir / "src"))
        d.src() = {"src/*"};
    if (d.inc().empty() && fs::is_directory(working_dir / "include"))
        d.inc() = {"public:include"};

    auto    &target    = targets().release();
    fs::path cache     = this->cache();
    fs::path build_dir = cache / "build" / target.id() / (name + "-" + d.version()) / feature_name;
    spdlog::info("build_dir={:?}", build_dir.string());

    std::vector<std::string> flags;
    for (auto &str : d.flags()) {
        if (str.rfind("public:", 0) == 0) {
            auto f = str.substr(std::string("public:").size());
            push_unique(flags, f);
            push_unique(lib().flags(), f);
        } else {
            push_unique(flags, str);
        }
    }
    for (auto &str : d.inc()) {
        if (str.rfind("public:", 0) == 0) {
            auto inc = "-I" + (working_dir / str.substr(std::string("public:").size())).string();
            push_unique(flags, inc);
            push_unique(lib().flags(), inc);
        } else {
            push_unique(flags, "-I" + (working_dir / str).string());
        }
    }
    for (auto &str : d.link_flags()) {
        string_replace(str, "working_dir", working_dir.string());
        push_unique(lib().link_flags(), str);
    }

    spdlog::debug("base={:?} src={:?}", working_dir.string(), d.src());
    try {
        auto expanded = expand_path(working_dir.string(), d.src());
        for (fs::path entry : expanded) {
            CompileCommand cc;
            cc.directory() = build_dir.string();
            cc.output()    = entry.string() + ".o";
            cc.depfile()   = entry.string() + ".d";
            cc.file()      = (working_dir / entry).string();
            if (auto ext = entry.extension(); ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".cppm") {
                if (ext == ".cppm" && package().edition() < 20)
                    throw ferr(
                        "C++ modules are not supported in edition {}, but {} is used", package().edition(), entry.string()
                    );

                cc.command() =
                    f("{} -std=c++{} {} -o '{}' -c '{}' -MMD -MP -MF '{}'",
                      target.cpp(),
                      package().edition(),
                      fmt::join(flags, " "),
                      cc.output(),
                      cc.file(),
                      cc.depfile());

                cc.compile();
                push_unique(lib().link_flags(), (working_dir / cc.output()).string());
                compile_commands().push_back(cc);
            } else if (ext == ".c") {
                cc.command() =
                    f("{} {} -o '{}' -c '{}' -MMD -MP -MF '{}'",
                      target.c(),
                      fmt::join(flags, " "),
                      cc.output(),
                      cc.file(),
                      cc.depfile());
                cc.compile();
                push_unique(lib().link_flags(), (working_dir / cc.output()).string());
                compile_commands().push_back(cc);
            }
        }
    } catch (std::exception &e) {
        throw ferr("Cannot resolve dep={:?}, src={}: {}", name, d.src(), e.what());
    }
}
