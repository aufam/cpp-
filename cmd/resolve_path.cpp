#include <cpp++/defer.h>
#include <spdlog/spdlog.h>
#include <fmt/ranges.h>
#include <filesystem>
#include <unordered_set>

namespace fs = std::filesystem;

static fs::path get_top_level_path_from_tar(const std::string &tar_file) {
    std::vector<std::string>        result;
    std::unordered_set<std::string> unique_entries;

    std::string  command = fmt::format("tar tf \"{}\" | cut -d/ -f1 | uniq", tar_file);
    auto         pipe    = popen(command.c_str(), "r");
    cppxx::defer _       = [&]() { pclose(pipe); };

    if (!pipe)
        throw std::runtime_error(fmt::format("Failed to run tar command for {:?}", tar_file));

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        std::string line(buffer);
        if (line.empty())
            continue;
        if (line.back() == '\n')
            line.pop_back();
        if (unique_entries.insert(line).second)
            result.emplace_back(line);
    }

    if (result.empty())
        throw std::runtime_error(fmt::format("Failed to get the top level path from {:?}", tar_file));

    // TODO: what if the tar_file has multiple paths
    if (result.size() > 1)
        throw std::runtime_error(fmt::format(
            "Multiple top level paths from {:?} are not supported. The paths are: {}", tar_file, fmt::join(result, " ")
        ));

    return result.front();
}

std::string resolve_path(const std::string &cache, const std::string &path_str) {
    const fs::path archive_dir = fs::path(cache) / "archive";
    const fs::path extract_dir = fs::path(cache) / "extracted";

    const fs::path    path      = path_str;
    const std::string extension = path.extension().string();

    const bool is_remote = path_str.rfind("http://", 0) == 0 || path_str.rfind("https://", 0) == 0 ||
                           path_str.rfind("ftp://", 0) == 0 || path_str.rfind("sftp://", 0) == 0;
    const bool is_compressed = extension == ".tar" or extension == ".tgz" or extension == ".gz" or extension == ".tbz2" or
                               extension == ".bz2" or extension == ".xz"; // TODO: zip?

    if (is_remote) {
        const fs::path    archive_path = archive_dir / path.filename();
        const std::string cmd          = fmt::format(
            "[ -d \"{0}\" ] || "
                     "(mkdir -p \"{0}\" && curl -sSfL -o \"{1}\" \"{0}\")",
            archive_path.string(),
            path_str
        );

        if (int res = std::system(cmd.c_str()); res)
            throw std::runtime_error(fmt::format("Failed to download archive from {:?}, return code: {}", path_str, res));

        return resolve_path(cache, archive_path.string());
    }

    if (is_compressed) {
        const fs::path extract_path = extract_dir / get_top_level_path_from_tar(path_str);
        if (not fs::exists(extract_path)) {
            fs::create_directories(extract_dir);
            std::string extract_cmd;
            if (extension == ".tar") {
                extract_cmd = fmt::format("tar -xf \"{}\" -C \"{}\"", path_str, extract_dir.string());
            } else if (extension == ".gz" or extension == ".tgz") {
                extract_cmd = fmt::format("tar -xzf \"{}\" -C \"{}\"", path_str, extract_dir.string());
            } else if (extension == ".bz2" or extension == ".tbz2") {
                extract_cmd = fmt::format("tar -xjf \"{}\" -C \"{}\"", path_str, extract_dir.string());
            } else if (extension == ".xz") {
                extract_cmd = fmt::format("tar -xJf \"{}\" -C \"{}\"", path_str, extract_dir.string());
            } else {
                throw std::runtime_error(fmt::format("Unsupported archive type {:?}", path_str));
            }

            spdlog::info("extracting {:?} to {:?}", path_str, extract_dir.string());
            if (int res = std::system(extract_cmd.c_str()); res)
                throw std::runtime_error(fmt::format("Failed to extract {:?}, return code: {}", path_str, res));
        }

        return resolve_path(cache, extract_path.string());
    }

    if (path.is_absolute() && !fs::exists(path))
        throw std::runtime_error(fmt::format("{:?} does not exist or unresolvable", path_str));

    // TODO: check existance of relative path?
    return path_str;
}
