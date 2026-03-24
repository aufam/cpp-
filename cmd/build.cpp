#include "context.h"
#include <stdexcept>
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <filesystem>

namespace fs = std::filesystem;


void Context::build(const std::vector<std::string> &features) {
    if (no_default_features() && features.empty())
        throw std::runtime_error("no features specified, but `no-default-features` is set");

    if (features.empty())
        return build({"default"});

    pre();

    auto &default_target = targets()["default"];
    for (auto &[name, dep] : dependencies()) {
        auto &d = convert_dep(dep);
        if (!d.optional())
            resolve_remote_dep(name, dep);
    }

    std::unordered_map<std::string, Target> exported_targets;
    std::vector<std::string>                public_flags;
    for (auto &name : features) {
        auto it = targets().find(name);
        if (it == targets().end())
            throw std::runtime_error("cannot find feature `" + name + "`");

        auto &t = convert_feat(it->second);
        apply_workdirs(name, t);

        // std::unordered_map<std::string, std::string> working_dirs;
        // for (auto &[name, dep] : dependencies()) {
        //     auto &d = convert_dep(dep);
        //     if (d.path().empty())
        //         resolve_remote_dep(name, dep);
        //     if (d.subdir().empty())
        //         working_dirs[name] = d.path();
        //     else
        //         working_dirs[name] = d.path() + "/" + d.subdir();
        // }
        //
        // t.apply_dependency_path(working_dirs);

        std::vector<std::string> flags = this->public_flags();
        t.collect_flags(flags, public_flags);
        t.collect_link_flags(this->link_flags());

        std::vector<CompileCommand> commands;
        t.collect_compile_commands(cache(), package(), name, flags, commands);
        for (auto &cc : commands) {
            spdlog::info("Compiling {} -> {}", cc.file(), cc.output());
            cc.compile();
            push_unique(this->link_flags(), fmt::format("{}/{}", cc.directory(), cc.output()));
        }

        compile_commands().insert(compile_commands().end(), commands.begin(), commands.end());
    }

    for (auto &str : public_flags)
        push_unique(this->public_flags(), str);
}

void Context::pre() {
    if (package().name().empty())
        throw std::runtime_error("name is required");

    if (package().version().empty())
        throw std::runtime_error("version is required");

    switch (package().edition()) {
    case 17:
    case 20:
    case 23:
    case 26:
        break;
    default:
        throw std::runtime_error("unsupported edition: " + std::to_string(package().edition()));
    }

    apply_package_placeholders();

    std::ignore = targets()["default"];
    // const bool default_target_is_not_defined = targets().find("default") == targets().end();
    // auto      &default_target                = targets()["default"];
    // if (default_target_is_not_defined) {
    //     Target t;
    //     if (fs::is_directory("src"))
    //         t.src() = {"src/*"};
    //     if (fs::is_directory("include"))
    //         t.inc() = {"public:include"};
    //     default_target += t;
    // }
}

void Context::apply_package_placeholders() {
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

        string_replace(d.version(), "name", name);
        string_replace(d.path(), "name", name);
        string_replace(d.url(), "name", name);
        string_replace(d.git(), "name", name);
        string_replace(d.branch(), "name", name);
        string_replace(d.tag(), "name", name);
        string_replace(d.subdir(), "name", name);

        string_replace(d.version(), "edition", edition);
        string_replace(d.path(), "edition", edition);
        string_replace(d.url(), "edition", edition);
        string_replace(d.git(), "edition", edition);
        string_replace(d.branch(), "edition", edition);
        string_replace(d.tag(), "edition", edition);
        string_replace(d.subdir(), "edition", edition);
    }

    for (auto &[_, feats] : features()) {
        for (auto &feat : feats) {
            string_replace(feat, "name", name);
            string_replace(feat, "version", version);
            string_replace(feat, "edition", edition);
        }
    }

    for (auto &[_, features] : targets()) {
        auto &t = convert_feat(features);
        for (auto &o : t.src()) {
            string_replace(o, "name", name);
            string_replace(o, "version", version);
            string_replace(o, "edition", edition);
        }
        for (auto &o : t.inc()) {
            string_replace(o, "name", name);
            string_replace(o, "version", version);
            string_replace(o, "edition", edition);
        }
        for (auto &o : t.flags()) {
            string_replace(o, "name", name);
            string_replace(o, "version", version);
            string_replace(o, "edition", edition);
        }
        for (auto &o : t.link_flags()) {
            string_replace(o, "name", name);
            string_replace(o, "version", version);
            string_replace(o, "edition", edition);
        }
    }
}

void Context::apply_workdirs(const std::string &feature, Target &target) {
    for (auto &str : target.inc())
        try {
            auto [relative, base] = resolve_workdirs(str);
            str                   = fmt::format("{}/{}", base, relative);
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving inc `{}`: {}", str, e.what()));
        }

    for (auto &str : target.flags())
        try {
            auto [relative, base] = resolve_workdirs(str);
            str                   = fmt::format("{}/{}", base, relative);
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving flag `{}`: {}", str, e.what()));
        }

    for (auto &str : target.link_flags())
        try {
            auto [relative, base] = resolve_workdirs(str);
            str                   = fmt::format("{}/{}", base, relative);
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving link_flag `{}`: {}", str, e.what()));
        }

    target.working_dirs().resize(target.src().size());
    for (size_t i = 0; i < target.src().size(); ++i) {
        auto &str = target.src()[i];
        try {
            auto [relative, base]    = resolve_workdirs(str);
            str                      = relative;
            target.working_dirs()[i] = base;
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving src `{}`: {}", str, e.what()));
        }
    }
}

auto Context::resolve_workdirs(const std::string &str) -> std::pair<std::string, std::string> {
    spdlog::debug("Resolving {}", str);
    if (str.empty()) {
        throw std::invalid_argument("Empty src");
    }

    // ❌ reject absolute paths
    if (str[0] == '/') {
        throw std::invalid_argument("Absolute paths not allowed");
    }

    std::string relative, base;

    if (str[0] == '{') {
        // parse {key}
        auto close = str.find('}');
        if (close == std::string::npos || close + 1 >= str.size() || str[close + 1] != '/') {
            throw std::invalid_argument("Invalid placeholder format");
        }

        std::string key = str.substr(1, close - 1);
        auto        it  = dependencies().find(key);
        if (it == dependencies().end()) {
            throw std::invalid_argument("Unknown dependency key: " + key);
        }

        relative = str.substr(close + 2); // after "}/"
        auto &d  = convert_dep(it->second);
        if (d.path().empty())
            resolve_remote_dep(it->first, it->second);
        if (d.subdir().empty())
            base = d.path();
        else
            base = d.path() + "/" + d.subdir();
    } else {
        if (auto pos = str.find('{'); pos != std::string::npos && pos + 1 < str.size() && str.find('}') != std::string::npos) {
            auto pre              = str.substr(0, pos);
            auto sub              = str.substr(pos);
            auto [relative, base] = resolve_workdirs(sub);
            return {relative, pre + base};
        }

        // plain relative path
        if (str.find('{') != std::string::npos || str.find('}') != std::string::npos) {
            throw std::invalid_argument("Invalid braces in path");
        }

        relative = str;
    }

    // ❌ optional: prevent traversal
    if (relative.find("..") != std::string::npos) {
        throw std::invalid_argument("Path traversal not allowed");
    }

    spdlog::debug("Resolved {}, relative={}, base={}", str, relative, base);
    return {relative, base};
}

static std::pair<std::string, std::string>
resolve(const std::string &src, const std::unordered_map<std::string, std::string> &working_dirs);

void Target::apply_dependency_path(const std::unordered_map<std::string, std::string> &working_dirs) {
    for (auto &str : inc())
        try {
            auto [relative, base] = resolve(str, working_dirs);
            str                   = fmt::format("{}/{}", base, relative);
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving inc `{}`: {}", str, e.what()));
        }

    for (auto &str : flags())
        try {
            auto [relative, base] = resolve(str, working_dirs);
            str                   = fmt::format("{}/{}", base, relative);
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving flag `{}`: {}", str, e.what()));
        }

    for (auto &str : link_flags())
        try {
            auto [relative, base] = resolve(str, working_dirs);
            str                   = fmt::format("{}/{}", base, relative);
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving link_flag `{}`: {}", str, e.what()));
        }

    this->working_dirs().resize(src().size());
    for (size_t i = 0; i < src().size(); ++i) {
        auto &str = src()[i];
        try {
            auto [relative, base]   = resolve(str, working_dirs);
            str                     = relative;
            this->working_dirs()[i] = base;
        } catch (const std::invalid_argument &e) {
            throw std::runtime_error(fmt::format("Error resolving src `{}`: {}", str, e.what()));
        }
    }
}

static std::pair<std::string, std::string>
resolve(const std::string &str, const std::unordered_map<std::string, std::string> &working_dirs) {
    if (str.empty()) {
        throw std::invalid_argument("Empty src");
    }

    // ❌ reject absolute paths
    if (str[0] == '/') {
        throw std::invalid_argument("Absolute paths not allowed");
    }

    std::string relative, base;

    if (str[0] == '{') {
        // parse {key}
        auto close = str.find('}');
        if (close == std::string::npos || close + 1 >= str.size() || str[close + 1] != '/') {
            throw std::invalid_argument("Invalid placeholder format");
        }

        std::string key = str.substr(1, close - 1);
        auto        it  = working_dirs.find(key);
        if (it == working_dirs.end()) {
            throw std::invalid_argument("Unknown working_dir key: " + key);
        }

        relative = str.substr(close + 2); // after "}/"
        base     = it->second;
    } else {
        if (auto pos = str.find('{'); pos != std::string::npos && pos + 1 < str.size() && str.find('}') != std::string::npos) {
            auto pre              = str.substr(0, pos);
            auto sub              = str.substr(pos);
            auto [relative, base] = resolve(sub, working_dirs);
            return {relative, pre + base};
        }

        // plain relative path
        if (str.find('{') != std::string::npos || str.find('}') != std::string::npos) {
            throw std::invalid_argument("Invalid braces in path");
        }

        relative = str;
    }

    // ❌ optional: prevent traversal
    if (relative.find("..") != std::string::npos) {
        throw std::invalid_argument("Path traversal not allowed");
    }

    return {relative, base};
}
