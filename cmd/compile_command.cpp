#include "context.h"
#include <fmt/format.h>
#include <filesystem>

namespace fs = std::filesystem;

void CompileCommand::compile() const {
    const std::string cmd = fmt::format(
        "mkdir -p \"{0}\" && cd \"{0}\" && [ -f \"{1}\" ] || "
        "(mkdir -p \"{2}\" && {3})",
        directory(),
        output(),
        fs::path(output()).parent_path().string(),
        command()
    );

    if (std::system(cmd.c_str()) != 0) {
        throw std::runtime_error(fmt::format("Failed to compile {}: command={:?}", file(), cmd));
    }
}
