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
