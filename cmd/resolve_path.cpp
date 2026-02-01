#include <cpp++/defer.h>
#include <regex>
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
        throw std::runtime_error(
            fmt::format(
                "Multiple top level paths from {:?} are not supported. The paths are: {}", tar_file, fmt::join(result, " ")
            )
        );

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
        const auto     get_tar_flag = [&]() {
            if (extension == ".tar") {
                return "-xf";
            } else if (extension == ".gz" or extension == ".tgz") {
                return "-xzf";
            } else if (extension == ".bz2" or extension == ".tbz2") {
                return "-xjf";
            } else if (extension == ".xz") {
                return "-xJf";
            } else {
                throw std::runtime_error(fmt::format("Unsupported archive type {:?}", path_str));
            }
        };

        std::string cmd = fmt::format(
            "[ -d \"{0}\" ] || "
            "(mkdir -p \"{1}\" && echo extracting {1:?} to {2:?} && tar {3} \"{1}\" && - C \"{1}\")",
            extract_path.string(),
            extract_dir.string(),
            path_str,
            get_tar_flag()
        );

        if (int res = std::system(cmd.c_str()); res)
            throw std::runtime_error(fmt::format("Failed to extract {:?}, return code: {}", path_str, res));

        return resolve_path(cache, extract_path.string());
    }

    if (path.is_absolute() && !fs::exists(path))
        throw std::runtime_error(fmt::format("{:?} does not exist or unresolvable", path_str));

    // TODO: check existance of relative path?
    return path_str;
}


std::vector<std::string> expand_path(const std::string &pattern) {
    std::vector<std::string> result;

    bool        recursive        = pattern.find("**") != std::string::npos;
    auto        last_slash       = pattern.rfind('/');
    std::string dir              = last_slash != std::string::npos ? pattern.substr(0, last_slash) : ".";
    std::string filename_pattern = pattern.substr(last_slash + 1);

    // Translate glob to regex:
    // - **  -> .*
    // - *   -> [^/]*   (matches any except slash)
    // - ?   -> .       (any single character)
    std::string regex_str;
    for (size_t i = 0; i < filename_pattern.size(); ++i) {
        if (filename_pattern[i] == '*') {
            if (i + 1 < filename_pattern.size() && filename_pattern[i + 1] == '*') {
                regex_str += ".*";
                ++i; // skip next '*'
            } else {
                regex_str += "[^/]*";
            }
        } else if (filename_pattern[i] == '?') {
            regex_str += '.';
        } else if (std::strchr(".^$|()[]{}+\\", filename_pattern[i])) {
            regex_str += '\\';
            regex_str += filename_pattern[i];
        } else {
            regex_str += filename_pattern[i];
        }
    }

    std::regex regex_pattern("^" + regex_str + "$");

    if (recursive) {
        for (const auto &entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), regex_pattern)) {
                result.push_back(entry.path().string());
            }
        }
    } else {
        for (const auto &entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && std::regex_match(entry.path().filename().string(), regex_pattern)) {
                result.push_back(entry.path().string());
            }
        }
    }

    return result;
}
