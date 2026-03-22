#pragma once

#include <cpp++/tag.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

struct CompileCommand;

struct Package {
    cppxx::Tag<std::string> name    = "toml,json:`name`";
    cppxx::Tag<std::string> version = "toml,json:`version,skipmissing,omitempty`";
    cppxx::Tag<int>         edition = {"toml,json:`edition,skipmissing`", 17};
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

    cppxx::Tag<std::vector<std::string>> working_dirs = "json:`workingDirs,skipmissing,omitempty`";

    Target &operator+=(const Target &other);

    Target operator+(const Target &other) const {
        Target self = *this;
        self += other;
        return self;
    }

    void apply_dependency_path(const std::unordered_map<std::string, std::string> &working_dirs);

    void collect_compile_commands(
        const std::string              &cache,
        const Package                  &package,
        const std::string              &name,
        const std::vector<std::string> &flags,
        std::vector<CompileCommand>    &commands
    ) const;

    void collect_flags(std::vector<std::string> &flags, std::vector<std::string> &public_flags) const;
    void collect_link_flags(std::vector<std::string> &link_flags) const;
};

struct CompileCommand {
    cppxx::Tag<std::string> file      = "json:`file`";
    cppxx::Tag<std::string> directory = "json:`directory`";
    cppxx::Tag<std::string> command   = "json:`command`";
    cppxx::Tag<std::string> output    = "json:`output`";

    void compile() const;
};

struct Context {
    using Dep  = std::variant<std::string, Dependency>;
    using Feat = std::variant<std::vector<std::string>, Target>;

    cppxx::Tag<std::unordered_map<std::string, Context>> packages     = "toml:`packages,skipmissing,omitempty`";
    cppxx::Tag<Package>                                  package      = "toml,json:`package`";
    cppxx::Tag<std::unordered_map<std::string, Dep>>     dependencies = "toml,json:`dependencies,skipmissing,omitempty`";

    cppxx::Tag<std::unordered_map<std::string, std::vector<std::string>>> features = "toml,json:`features,skipmissing,omitempty`";
    cppxx::Tag<std::unordered_map<std::string, Feat>>                     targets  = "toml,json:`targets,skipmissing,omitempty`";
    cppxx::Tag<std::optional<Target>>                                     lib      = "toml,json:`lib,skipmissing,omitempty`";
    cppxx::Tag<std::vector<Target>>                                       bin      = "toml,json:`bin,skipmissing,omitempty`";

    cppxx::Tag<std::string> cache               = "opt:`cache,env=CPPXX_CACHE`";
    cppxx::Tag<bool>        no_default_features = "opt:`no-default-features,help=Disable default features`";

    cppxx::Tag<std::vector<CompileCommand>> compile_commands = "json:`compile_commands`";
    cppxx::Tag<std::vector<std::string>>    public_inc       = "json:`public_inc`";
    cppxx::Tag<std::vector<std::string>>    public_flags     = "json:`public_flags`";
    cppxx::Tag<std::vector<std::string>>    link_flags       = "json:`link_flags`";

public:
    void build(const std::vector<std::string> &features = {});
    void resolve_feats(const std::vector<std::string> &features = {});

private:
    void pre();
    void apply_package_placeholders();
    void apply_workdirs(const std::string &name, Target &target);

    auto resolve_workdirs(const std::string &str) -> std::pair<std::string, std::string>;
    void resolve_remote_dep(const std::string &name, Dep &dep);

    void resolve_target(const std::string &name, Target &target);

    Target &convert_feat(Context::Feat &feat);
};

Dependency &convert_dep(Context::Dep &dep);

std::string resolve_path(const std::string &cache, const std::string &path);
std::string git_clone(const std::string &cache, const std::string &git, const std::string &tag);

std::vector<std::string> expand_path(const std::string &pattern);

void string_replace(std::string &str, const std::string &key, const std::string &value);
void push_unique(std::vector<std::string> &v, const std::string &str);
