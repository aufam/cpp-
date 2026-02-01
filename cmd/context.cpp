#include "context.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

void Context::resolve(const std::vector<std::string> &features) {
    for (auto &[name, dep] : dependencies()) {
        auto &d = convert_dep(dep);
        if (d.optional())
            continue; // resolve later

        spdlog::info("resolving {}", name);
        try {
            resolve_dep(name, dep);
        } catch (std::exception &e) {
            throw std::runtime_error("Failed to resolve `" + name + "`: " + e.what());
        }
    }

    for (auto &[name, target] : targets()) {
        if (name == "default" && no_default_features())
            continue;

        bool found = false;
        for (const auto &feature : features)
            if (feature == name) {
                found = true;
                break;
            }
        if (!found)
            throw std::runtime_error("Feature `" + name + "` not found in provided features.");

        // If the feature is found and it's a target, process it.
        // This is where you would typically add logic to enable or configure the target
        // based on the fact that its feature was requested.
        // For example, you might mark it as 'enabled' or set its configuration.
        spdlog::info("Processing target: {}", name);
        // Example: target.enable(); // If your Target object has such a method
    }
}
