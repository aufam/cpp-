#include "context.h"
#include <fmt/ranges.h>
#include <spdlog/spdlog.h>
#include <stdexcept>


void Context::resolve_feats(const std::vector<std::string> &features) {
    if (no_default_features() && features.empty())
        throw std::runtime_error("no features specified, but `no-default-features` is set");

    if (features.empty())
        return resolve_feats({"default"});

    for (auto &[name, dep] : dependencies())
        resolve_remote_dep(name, dep);

    for (auto &name : features) {
        spdlog::debug("resolving target: {}", name);
        if (auto it = targets().find(name); it != targets().end()) {
            resolve_target(it->first, convert_feat(it->second));
        } else {
            throw std::runtime_error("cannot resolve feature `" + name + "`");
        }
    }
}
