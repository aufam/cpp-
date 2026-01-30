#pragma once

#include <cpp++/tag.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct Config;

struct Package {
    cppxx::Tag<std::string> name    = "toml,json:`name`";
    cppxx::Tag<std::string> version = "toml,json:`version,skipmissing,omitempty`";
};

struct Dependency {
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
};

struct Target {
    cppxx::Tag<std::vector<std::string>> src        = "toml,json:`src,skipmissing,omitempty`";
    cppxx::Tag<std::vector<std::string>> inc        = "toml,json:`inc,skipmissing,omitempty`";
    cppxx::Tag<std::vector<std::string>> flags      = "toml,json:`flags,skipmissing,omitempty`";
    cppxx::Tag<std::vector<std::string>> link_flags = "toml:`link-flags,skipmissing,omitempty`"
                                                      "json:`linkFlags,skipmissing,omitempty`";
};

struct CompileCommand {
    cppxx::Tag<std::string> file      = "json:`file`";
    cppxx::Tag<std::string> directory = "json:`directory`";
    cppxx::Tag<std::string> command   = "json:`command`";
    cppxx::Tag<std::string> output    = "json:`output`";
};

struct Config {
    using Dep = std::variant<std::string, Dependency>;

    cppxx::Tag<Package>                                      package      = "toml,json:`package`";
    cppxx::Tag<std::unordered_map<std::string, Dep>>         dependencies = "toml,json:`dependencies,skipmissing,omitempty`";
    cppxx::Tag<std::unordered_map<std::string, std::string>> features     = "toml,json:`features,skipmissing,omitempty`";
    cppxx::Tag<std::unordered_map<std::string, Target>>      targets      = "toml,json:`targets,skipmissing,omitempty`";
    cppxx::Tag<std::optional<Target>>                        lib          = "toml,json:`lib,skipmissing,omitempty`";
    cppxx::Tag<std::vector<Target>>                          bin          = "toml,json:`bin,skipmissing,omitempty`";
    cppxx::Tag<std::unordered_map<std::string, Config>>      packages     = "toml,json:`packages,skipmissing,omitempty`";

    cppxx::Tag<std::string> cache = "opt:`cache,skipmissing,env=CPPXX_CACHE`";

    cppxx::Tag<std::vector<CompileCommand>> compile_commands = "json:`compile_commands`";
    cppxx::Tag<std::vector<std::string>>    public_inc       = "";
    cppxx::Tag<std::vector<std::string>>    public_flags     = "";
    cppxx::Tag<std::vector<std::string>>    link_flags       = "";
    cppxx::Tag<std::vector<std::string>>    objects          = "";

public:
    void resolve(const std::vector<std::string> &features = {});

private:
    Dependency &convert_dep(Dep &dep);

    void resolve_dep(const std::string &name, Dep &dep);
    void apply_version_to_packages(const std::string &version, Config &dep_package);
};


std::string resolve_path(const std::string &cache, const std::string &path);
std::string git_clone(const std::string &cache, const std::string &git, const std::string &tag);
