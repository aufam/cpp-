#include "config.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

void Config::resolve(const std::vector<std::string> &features) {
    for (auto &[name, dep] : dependencies()) {
        spdlog::info("resolving {}", name);
        try {
            resolve_dep(name, dep);
        } catch (std::exception &e) {
            throw std::runtime_error("Failed to resolve `" + name + "`: " + e.what());
        }
    }
}
