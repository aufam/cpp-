#include <cpp++/toml/marzer_toml.h>
#include <cpp++/json/yy_json.h>
#include <cpp++/cli/cli11.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "context.h"

int main(int argc, char **argv) {
    auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>("cpp++", std::move(sink)));
    spdlog::set_level(spdlog::level::trace);

    Context ctx;
    cppxx::cli::cli11::parse("c++ plusplus", argc, argv, ctx);
    cppxx::toml::marzer_toml::parse_from_file("./packages.toml", ctx.packages());
    cppxx::toml::marzer_toml::parse_from_file("./cpp++.toml", ctx);

    // if (ctx.cache().empty())
    ctx.cache() = std::getenv("HOME") + std::string("/.cpp++");

    try {
        ctx.build();
    } catch (std::exception &e) {
        spdlog::error("Failed to build: {}", e.what());
        exit(1);
    }

    std::cout << cppxx::json::yy_json::dump(ctx, YYJSON_WRITE_PRETTY_TWO_SPACES) << '\n';
    return 0;
}
