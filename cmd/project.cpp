#include "main.h"
#include <algorithm>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <filesystem>

#define f(...)    fmt::format(__VA_ARGS__)
#define ferr(...) std::runtime_error(fmt::format(__VA_ARGS__))
namespace fs = std::filesystem;

void Project::build(const std::vector<std::string> &features) {
    if (no_default_features() && features.empty())
        throw ferr("Error building {:?}: no features specified, but `no-default-features` is set", package().name());

    if (features.empty())
        return build({"default"});

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

    for (auto &[name, dep] : dependencies()) {
        try {
            resolve_remote_dep(name, convert_dep(dep));
        } catch (const std::exception &e) {
            throw ferr("Error building dependency `{}` of package `{}`: {}", name, package().name(), e.what());
        }
    }
}

void Project::resolve_remote_dep(const std::string &name, Dependency &d) {
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
        p.targets()             = targets();

        if (p.dependencies().find("default") != p.dependencies().end()) {
            p.apply_package_placeholders();
            resolve_remote_dep("default", convert_dep(p.dependencies().at("default")));
            return;
            // TODO: features?
        } else {
            try {
                p.build(d.features());
            } catch (const std::exception &e) {
                throw ferr("Error building dependency package={} `{}`: {}", package().name(), name, e.what());
            }

            compile_commands().insert(compile_commands().end(), p.compile_commands().begin(), p.compile_commands().end());
            public_inc().insert(public_inc().end(), p.public_inc().begin(), p.public_inc().end());
            public_flags().insert(public_flags().end(), p.public_flags().begin(), p.public_flags().end());
            link_flags().insert(link_flags().end(), p.link_flags().begin(), p.link_flags().end());

            // TODO: handle default target
            return;
        }
    }

    collect_meta(name, d);
}

void Project::collect_meta(const std::string &name, Dependency &d) {
    std::sort(d.features().begin(), d.features().end());
    std::string feature_name = fmt::format("{}", fmt::join(d.features(), "-"));
    if (d.default_features().value_or(true))
        feature_name = "default-" + feature_name;
    if (!feature_name.empty())
        feature_name = "-";

    auto &target = targets().release();

    fs::path cache       = this->cache();
    fs::path working_dir = fs::path(d.path()) / d.subdir();
    if (working_dir.empty())
        working_dir = fs::current_path();
    fs::path build_dir = cache / "build" / target.id() / package().name() / feature_name;

    if (d.src().empty() && fs::is_directory(working_dir / "src"))
        d.src() = {"src/*"};
    if (d.inc().empty() && fs::is_directory(working_dir / "include"))
        d.inc() = {"public:include"};

    std::vector<std::string> flags;
    for (auto &str : d.flags()) {
        if (str.rfind("public:", 0) == 0) {
            auto f = str.substr(std::string("public:").size());
            flags.push_back(f);
            public_flags().push_back(f);
        } else {
            flags.push_back(str);
        }
    }
    for (auto &str : d.inc()) {
        if (str.rfind("public:", 0) == 0) {
            auto inc = "-I" + (working_dir / str.substr(std::string("public:").size())).string();
            flags.push_back(inc);
            public_inc().push_back(inc);
        } else {
            flags.push_back("-I" + (working_dir / str).string());
        }
    }
    for (auto &str : d.link_flags()) {
        string_replace(str, "working_dir", working_dir.string());
        link_flags().push_back(str);
    }

    spdlog::debug("base={:?} src={:?}", working_dir.string(), d.src());
    try {
        auto expanded = expand_path(working_dir.string(), d.src());
        for (fs::path entry : expanded) {
            CompileCommand cc;
            cc.directory() = build_dir.string();
            cc.output()    = entry.string() + ".o";
            cc.file()      = (working_dir / entry).string();
            if (auto ext = entry.extension(); ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".cppm") {
                if (ext == ".cppm" && package().edition() < 20)
                    throw ferr(
                        "C++ modules are not supported in edition {}, but {} is used", package().edition(), entry.string()
                    );

                cc.command() =
                    f("{} -std=c++{} {} -o {} -c {}",
                      target.cpp(),
                      package().edition(),
                      fmt::join(flags, " "),
                      cc.output(),
                      cc.file());

                cc.compile();
                link_flags().push_back((working_dir / cc.output()).string());
                compile_commands().push_back(cc);
            } else if (ext == ".c") {
                cc.command() = f("{} {} -o {} -c {}", target.c(), fmt::join(flags, " "), cc.output(), cc.file());
                cc.compile();
                link_flags().push_back((working_dir / cc.output()).string());
                compile_commands().push_back(cc);
            }
        }
    } catch (std::exception &e) {
        throw ferr("Cannot resolve dep={:?}, src={}: {}", name, d.src(), e.what());
    }
}

void Project::apply_package_placeholders() {
    auto &name    = package().name();
    auto &version = package().version();
    auto  edition = std::to_string(package().edition());

    for (auto &[_, dep] : dependencies()) {
        auto &d = convert_dep(dep);
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
    }

    for (auto &[_, feats] : features()) {
        for (auto &feat : feats) {
            string_replace(feat, "name", name);
            string_replace(feat, "version", version);
            string_replace(feat, "edition", edition);
        }
    }
}
