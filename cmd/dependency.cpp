#include "context.h"
#include <cpp++/defer.h>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h>
#include <filesystem>

namespace fs = std::filesystem;

Dependency &Context::convert_dep(Dep &dep) {
    if (auto *version = std::get_if<std::string>(&dep)) {
        Dependency d{};
        d.version() = *version;
        dep         = d;
    }

    return std::get<Dependency>(dep);
}

void Context::resolve_dep(const std::string &name, Context::Dep &dep) {
    auto &d = convert_dep(dep);
    if (d.optional()) {
        return; // resolve later
    }

    auto remove_prefix = [](std::string input) {
        for (std::string_view prefix : {"https://", "http://", "sftp://", "ftp://"}) {
            if (input.rfind(prefix, 0) == 0)
                input.erase(0, prefix.size());
        }
        return input;
    };
    std::string out_dir;
    if (!d.path().empty()) {
        spdlog::info("resolving path of {}: {}", name, d.path());
        out_dir  = (fs::path(cache()) / "build" / remove_prefix(d.path())).string();
        d.path() = resolve_path(cache(), d.path());
    } else if (!d.git().empty()) {
        auto tag = d.tag().empty() ? d.branch() : d.tag();
        spdlog::info("resolving git of {}: {} {}", name, d.git(), tag);
        d.path() = git_clone(cache(), d.git(), tag);
        out_dir  = (fs::path(cache()) / "build" / remove_prefix(d.git()) / tag).string();
    } else if (d.version().empty()) {
        throw std::runtime_error("path|git|version is not defined");
    } else {
        spdlog::info("resolving version of {}: {}", name, d.version());
        out_dir = (fs::path(cache()) / "build" / name / d.version()).string();
        auto it = packages().find(name);
        if (it == packages().end())
            throw std::runtime_error("Cannot find `" + name + "` in the package list");

        auto p    = it->second;
        p.cache() = cache();
        p.cache() = cache();

        apply_version_to_packages(d.version(), p);

        p.resolve(d.features());

        // TODO: remove duplicate?
        for (auto &cc : p.compile_commands())
            compile_commands().push_back(cc);
        for (auto &i : p.public_inc())
            public_inc().push_back(i);
        for (auto &f : p.public_flags())
            public_flags().push_back(f);
        for (auto &f : p.link_flags())
            link_flags().push_back(f);
        for (auto &o : p.objects())
            objects().push_back(o);

        return;
    }

    {
        std::string  command = fmt::format("echo \"{}\"", d.path());
        auto         pipe    = popen(command.c_str(), "r");
        cppxx::defer _       = [&]() { pclose(pipe); };

        char buffer[4096];
        d.path() = fgets(buffer, sizeof(buffer), pipe);
        d.path().pop_back();
    }

    std::string include;
    if (auto inc = fs::path(d.path()) / "include"; fs::is_directory(inc)) {
        public_inc().push_back(inc.string());
        include = "-I" + inc.string() + " ";
    }

    fs::path src = fs::path(d.path()) / "src";
    if (!fs::is_directory(src)) {
        return;
    }

    for (const auto &entry : fs::recursive_directory_iterator(src)) {
        if (!entry.is_regular_file())
            continue;

        CompileCommand cc;
        cc.directory() = out_dir;
        cc.output()    = fs::relative(entry.path(), src).string() + ".o";
        cc.file()      = entry.path().string();
        if (auto ext = entry.path().extension(); ext == ".cpp" || ext == ".cxx" || ext == ".cc") {
            cc.command() = fmt::format("c++ -std=c++17 {}-o {} -c {}", include, cc.output(), cc.file());
            compile_commands().push_back(cc);
        } else if (ext == ".c") {
            cc.command() = fmt::format("cc {}-o {} -c {}", include, cc.output(), cc.file());
            compile_commands().push_back(cc);
        } else {
            continue;
        }

        fs::path    parent_path = (fs::path(cc.directory()) / cc.output()).parent_path();
        std::string cmd         = fmt::format(
            R"(mkdir -p "{0}" && cd "{1}" && [ ! -e "{2}" ] || [ "{2}" -ot "{3}" ] && {4})",
            parent_path.string(),
            cc.directory(),
            cc.output(),
            cc.file(),
            cc.command()
        );
        spdlog::info("cmd = {}", cmd);
        if (int res = std::system(cmd.c_str()); res)
            throw std::runtime_error(fmt::format("Failed to compile {:?}, return code: {}", cc.file(), res));
    }
}

void Context::apply_version_to_packages(const std::string &version, Context &dep_package) {
    for (auto &[_, dep] : dep_package.dependencies()) {
        auto &d     = convert_dep(dep);
        d.version() = fmt::format(fmt::runtime(d.version()), fmt::arg("version", version));
        d.path()    = fmt::format(fmt::runtime(d.path()), fmt::arg("version", version));
        d.url()     = fmt::format(fmt::runtime(d.url()), fmt::arg("version", version));
        d.git()     = fmt::format(fmt::runtime(d.git()), fmt::arg("version", version));
        d.branch()  = fmt::format(fmt::runtime(d.branch()), fmt::arg("version", version));
        d.tag()     = fmt::format(fmt::runtime(d.tag()), fmt::arg("version", version));
        d.subdir()  = fmt::format(fmt::runtime(d.subdir()), fmt::arg("version", version));
    }

    for (auto &[_, feat] : dep_package.features()) {
        feat = fmt::format(fmt::runtime(feat), fmt::arg("version", version));
    }

    for (auto &[_, t] : dep_package.targets()) {
        for (auto &o : t.src())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t.inc())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t.flags())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t.link_flags())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
    }

    for (auto &t : dep_package.bin()) {
        for (auto &o : t.src())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t.inc())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t.flags())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t.link_flags())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
    }

    if (auto &t = dep_package.lib(); t.has_value()) {
        for (auto &o : t->src())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t->inc())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t->flags())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
        for (auto &o : t->link_flags())
            o = fmt::format(fmt::runtime(o), fmt::arg("version", version));
    }
}
