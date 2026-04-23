#pragma once

#include <cpp++/tag.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct CompileCommand;

struct Target {
    cppxx::Tag<std::string> id  = "toml,json:`id`";
    cppxx::Tag<std::string> cpp = {"toml,json:`c++,skipmissing`", "c++"};
    cppxx::Tag<std::string> c   = {"toml,json:`c,skipmissing`", "c"};

    static Target Release() {
        Target t;
        t.id()  = "release";
        t.cpp() = "c++ -O3 -DNDEBUG -fPIC";
        t.c()   = "cc -O3 -DNDEBUG -fPIC";
        return t;
    }

    static Target Debug() {
        Target t;
        t.id()  = "debug";
        t.cpp() = "c++ -g -fPIC";
        t.c()   = "cc -g -fPIC";
        return t;
    }
};

struct Package {
    cppxx::Tag<std::string>              name        = "toml,json:`name`";
    cppxx::Tag<std::string>              version     = "toml,json:`version,skipmissing,omitempty`";
    cppxx::Tag<int>                      edition     = {"toml,json:`edition,skipmissing`", 17};
    cppxx::Tag<std::vector<std::string>> authors     = "toml,json:`authors,skipmissing,omitempty`";
    cppxx::Tag<std::string>              description = "toml,json:`description,skipmissing,omitempty`";
    cppxx::Tag<std::string>              license     = "toml,json:`license,skipmissing,omitempty`";

    cppxx::Tag<std::unordered_map<std::string, std::string>> vars = "toml,json:`vars,skipmissing,omitempty`";
};

struct Dependency {
    cppxx::Tag<std::string> name = "toml,json:`name,skipmissing,omitempty`";

    cppxx::Tag<std::string> version = "toml,json:`version,skipmissing,omitempty,oneof=version|path|url|git`";
    cppxx::Tag<std::string> path    = "toml,json:`path,skipmissing,omitempty,oneof=version|path|url|git`";
    cppxx::Tag<std::string> url     = "toml,json:`url,skipmissing,omitempty,oneof=version|path|url|git`";
    cppxx::Tag<std::string> git     = "toml,json:`git,skipmissing,omitempty,oneof=version|path|url|git`";
    cppxx::Tag<std::string> branch  = "toml,json:`branch,skipmissing,omitempty,oneof=branch|tag`";
    cppxx::Tag<std::string> tag     = "toml,json:`tag,skipmissing,omitempty,oneof=branch|tag`";
    cppxx::Tag<std::string> subdir  = "toml,json:`subdir,skipmissing,omitempty`";

    cppxx::Tag<std::vector<std::string>> features         = "toml,json:`features,skipmissing,omitempty`";
    cppxx::Tag<bool>                     optional         = "toml,json:`optional,skipmissing,omitempty`";
    cppxx::Tag<std::optional<bool>>      default_features = "toml:`default-features,skipmissing,omitempty`"
                                                            "json:`defaultFeatures,skipmissing,omitempty`";

    cppxx::Tag<std::vector<std::string>> src        = "toml,json:`src,skipmissing,omitempty`";
    cppxx::Tag<std::vector<std::string>> inc        = "toml,json:`inc,skipmissing,omitempty`";
    cppxx::Tag<std::vector<std::string>> lib        = "toml,json:`lib,skipmissing,omitempty`";
    cppxx::Tag<std::vector<std::string>> flags      = "toml,json:`flags,skipmissing,omitempty`";
    cppxx::Tag<std::vector<std::string>> link_flags = "toml:`link-flags,skipmissing,omitempty`"
                                                      "json:`linkFlags,skipmissing,omitempty`";

    cppxx::Tag<std::string> pre = "toml,json:`pre,skipmissing,omitempty`";

    std::optional<int> cpp_standard = std::nullopt;

    Dependency &operator+=(const Dependency &other);
    bool        empty() const;
};

using Lib = Dependency;

struct CompileCommand {
    cppxx::Tag<std::string> file      = "json:`file`";
    cppxx::Tag<std::string> directory = "json:`directory`";
    cppxx::Tag<std::string> command   = "json:`command`";
    cppxx::Tag<std::string> output    = "json:`output`";
    cppxx::Tag<std::string> depfile   = "";

    void compile() const;
};

struct Project {
    using Dep = std::variant<std::string, Dependency>;

    struct Targets {
        cppxx::Tag<Target> release = {"toml,json:`release,skipmissing`", Target::Release()};
        cppxx::Tag<Target> debug   = {"toml,json:`debug,skipmissing", Target::Debug()};
    };
    cppxx::Tag<Targets> targets = "toml,json:`targets,skipmissing`";

    cppxx::Tag<std::unordered_map<std::string, Project>> packages = "toml:`packages,skipmissing,omitempty`";
    cppxx::Tag<Package>                                  package  = "toml,json:`package`";

    cppxx::Tag<std::unordered_map<std::string, Dep>> dependencies = "toml,json:`dependencies,skipmissing,omitempty`";
    cppxx::Tag<Lib>                                  lib          = "toml,json:`lib,skipmissing`";
    cppxx::Tag<std::unordered_map<std::string, std::vector<std::string>>> features = "toml,json:`features,skipmissing,omitempty`";

    cppxx::Tag<std::string> cache               = "opt:`cache,env=CPPXX_CACHE`";
    cppxx::Tag<bool>        no_default_features = "opt:`no-default-features,help=Disable default features`";

    enum class LogLevel {
        trace,
        debug,
        info,
        warn,
        err,
        critical,
        off,
    };
    cppxx::Tag<LogLevel> log_level = {"opt:`log-level,skipmissing`", LogLevel::err};

    cppxx::Tag<std::vector<CompileCommand>> compile_commands = "json:`compile_commands`";

    std::unordered_map<std::string, Project> *ppackages = nullptr;

    void build(const std::vector<std::string> &features = {}, bool subpackage = false);

private:
    void apply_package_placeholders();
    void resolve_remote_dep(const std::string &name, Dependency &dep);
    void collect_meta(const std::string &name, Dependency &dep);
};

Dependency &convert_dep(Project::Dep &dep);

std::string resolve_path(const std::string &cache, const std::string &path);
std::string git_clone(const std::string &cache, const std::string &git, const std::string &tag);

std::vector<std::string> expand_path(const std::string &working_dir, std::vector<std::string> sources);

void push_unique(std::vector<std::string> &vec, const std::string &value);
void push_unique(std::vector<std::string> &vec, const std::vector<std::string> &values);
