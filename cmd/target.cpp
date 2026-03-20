#include "context.h"
#include "spdlog/spdlog.h"
#include <fmt/ranges.h>
#include <filesystem>

namespace fs = std::filesystem;


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

    std::vector<std::string> flags;
    flags.insert(flags.end(), public_flags().begin(), public_flags().end());

    for (auto &flag : target.flags()) {
        if (std::string pub = "public:"; flag.rfind(pub, 0) == 0) {
            auto f = flag.substr(pub.size());
            flags.push_back(f);
            public_flags().push_back(f);
        } else {
            flags.push_back(flag);
        }
    }

    for (auto &include : public_flags()) {
        flags.push_back("-I" + include);
    }

    for (auto &include : target.inc()) {
        if (std::string pub = "public:"; include.rfind(pub, 0) == 0) {
            auto inc = "-I" + include.substr(pub.size());
            flags.push_back(inc);
            public_inc().push_back(inc);
        } else {
            flags.push_back("-I" + include);
        }
    }

    std::vector<std::string> entries;
    for (auto &src : target.src())
        try {
            spdlog::debug("expanding {:?}", src);
            auto expanded = expand_path(src);
            entries.insert(entries.end(), expanded.begin(), expanded.end());
        } catch (std::exception &e) {
            throw std::runtime_error(fmt::format("Cannot resolve target {:?}: {}", name, e.what()));
        }

    fs::path out_dir = fs::path(cache()) / "build" / package().name() / package().version();
    for (fs::path entry : entries) {
        CompileCommand cc;
        cc.directory() = out_dir;
        cc.output()    = entry.filename().string() + ".o";
        cc.file()      = entry.string();
        if (auto ext = entry.extension(); ext == ".cpp" || ext == ".cxx" || ext == ".cc") {
            cc.command() = fmt::format("c++ -std=c++17 {} -o {} -c {}", fmt::join(flags, " "), cc.output(), cc.file());
            compile_commands().push_back(cc);
        } else if (ext == ".c") {
            cc.command() = fmt::format("cc {} -o {} -c {}", fmt::join(flags, " "), cc.output(), cc.file());
            compile_commands().push_back(cc);
        } else {
            continue;
        }

        link_flags().push_back(cc.directory() + "/" + cc.output());
        compile_commands().push_back(cc);
    }
}

Target &Target::operator+=(const Target &other) {
    for (auto &str : other.src())
        push_unique(src(), str);

    for (auto &str : other.inc())
        push_unique(inc(), str);

    for (auto &str : other.flags())
        push_unique(flags(), str);

    for (auto &str : other.link_flags())
        push_unique(link_flags(), str);

    return *this;
}

void Target::collect_compile_commands(
    const std::string              &cache,
    const Package                  &package,
    const std::string              &name,
    const std::vector<std::string> &flags,
    std::vector<CompileCommand>    &v
) const {
    for (size_t i = 0; i < this->src().size(); ++i) {
        auto    &src  = this->src()[i];
        fs::path base = working_dirs()[i];
        if (base.empty())
            base = fs::current_path();

        spdlog::debug("base={:?} src={:?}", base.string(), src);
        try {
            auto expanded = expand_path((base / src).string());
            for (fs::path entry : expanded) {
                CompileCommand cc;
                cc.directory() = fmt::format("{}/build/{}-{}/{}", cache, package.name(), package.version(), name);
                cc.output()    = fs::relative(entry, base).string() + ".o";
                cc.file()      = entry.string();
                if (auto ext = entry.extension(); ext == ".cpp" || ext == ".cxx" || ext == ".cc" || ext == ".cppm") {
                    if (ext == ".cppm" && package.edition() < 20)
                        throw std::runtime_error(
                            fmt::format(
                                "C++ modules are not supported in edition {}, but {} is used", package.edition(), entry.string()
                            )
                        );

                    cc.command() = fmt::format(
                        "c++ -std=c++{} {} -o {} -c {}", package.edition(), fmt::join(flags, " "), cc.output(), cc.file()
                    );
                    v.push_back(cc);
                } else if (ext == ".c") {
                    cc.command() = fmt::format("cc {} -o {} -c {}", fmt::join(flags, " "), cc.output(), cc.file());
                    v.push_back(cc);
                }
            }
        } catch (std::exception &e) {
            throw std::runtime_error(fmt::format("Cannot resolve src {:?}: {}", src, e.what()));
        }
    }
}

void push_unique(std::vector<std::string> &v, const std::string &str) {
    for (auto &s : v)
        if (s == str)
            return;
    v.push_back(str);
}

void Target::collect_flags(std::vector<std::string> &flags, std::vector<std::string> &public_flags) const {
    for (auto &str : this->inc())
        if (std::string pub = "public:"; str.rfind(pub, 0) == 0) {
            push_unique(public_flags, "-I" + str.substr(pub.size()));
            push_unique(flags, "-I" + str.substr(pub.size()));
        } else {
            push_unique(flags, "-I" + str.substr(pub.size()));
        }

    for (auto &str : this->flags())
        if (std::string pub = "public:"; str.rfind(pub, 0) == 0) {
            push_unique(public_flags, str.substr(pub.size()));
            push_unique(flags, str.substr(pub.size()));
        } else {
            push_unique(flags, str.substr(pub.size()));
        }
}

void Target::collect_link_flags(std::vector<std::string> &v) const {
    for (auto &str : link_flags())
        push_unique(v, str);
}
