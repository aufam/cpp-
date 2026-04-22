#include "main.h"
#include <fmt/format.h>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::vector<std::string> parse_depfile(const std::string &path) {
    std::ifstream in(path);
    if (!in)
        return {};

    std::vector<std::string> result;

    std::string line, content;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\\') {
            line.pop_back();
            content += line;
            continue;
        } else {
            content += line;
        }

        // remove "target:"
        auto colon = content.find(':');
        if (colon == std::string::npos) {
            content.clear();
            continue;
        }

        std::string deps = content.substr(colon + 1);

        std::istringstream iss(deps);
        std::string        dep;
        while (iss >> dep) {
            result.push_back(dep);
        }
        content.clear();
    }

    return result;
}

static bool needs_rebuild(const fs::path &directory, const fs::path &output, const fs::path &depfile) {
    const auto out = output.is_absolute() ? output : directory / output;
    const auto dep = depfile.is_absolute() ? depfile : directory / depfile;

    if (!fs::exists(out) || !fs::exists(dep))
        return true;

    auto out_time = fs::last_write_time(out);

    for (auto &d : parse_depfile(dep)) {
        fmt::print("Checking dependency: {}\n", d);
        if (!fs::exists(dep))
            return true;

        if (fs::last_write_time(d) > out_time)
            return true;
    }

    return false;
}

void CompileCommand::compile() const {
    if (!needs_rebuild(fs::path(directory()), fs::path(output()), fs::path(depfile()))) {
        return; // up-to-date
    }

    const std::string cmd = fmt::format(
        "mkdir -p \"{0}\" && cd \"{0}\" && "
        "(mkdir -p \"{1}\" && {2})",
        directory(),
        fs::path(output()).parent_path().string(),
        command()
    );

    if (std::system(cmd.c_str()) != 0) {
        throw std::runtime_error(fmt::format("Failed to compile {}: command={:?}", file(), cmd));
    }
}
