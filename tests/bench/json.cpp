#include <cpp++/json/yy_json.h>
#include <cpp++/json/nlohmann_json.h>
#include <benchmark/benchmark.h>


template <typename T>
using Tag = cppxx::Tag<T>;

namespace {
    struct Schema {
        struct User {
            Tag<int>                      id       = "json:`id`";
            Tag<std::string>              username = "json:`username`";
            Tag<std::string>              email    = "json:`email`";
            Tag<std::vector<std::string>> roles    = "json:`roles`";

            struct Profile {
                Tag<int>         age     = "json:`age`";
                Tag<std::string> country = "json:`country`";

                struct Preferences {
                    Tag<bool> dark_mode = "json:`darkMode`";
                    struct Notifications {
                        Tag<bool> email = "json:`email`";
                        Tag<bool> push  = "json:`push`";
                    };
                    Tag<Notifications> notifications = "json:`notifications`";
                };
                Tag<Preferences> preferences = "json:`preferences`";
            };
            Tag<Profile> profile = "json:`profile`";
        };
        Tag<User> user = "json:`user`";

        struct Meta {
            Tag<std::string> request_id = "json:`requestId`";
            Tag<std::tm>     timestamp  = "json:`timestamp`";
        };
        Tag<Meta> meta = "json:`meta`";
    };
} // namespace

static const char *data = R"json({
    "user": {
        "id": 12345,
        "username": "sucipto",
        "email": "sucipto@makmursejahtera.co.id",
        "roles": ["admin", "editor"],
        "profile": {
            "age": 30,
            "country": "Indonesia",
            "preferences": {
                "darkMode": true,
                "notifications": {
                    "email": true,
                    "push": false
                }
            }
        }
    },
    "meta": {
        "requestId": "abc-123-xyz",
        "timestamp": "2026-02-12T10:00:00Z"
    }
})json";

static void cppxx_yyjson_serialization(benchmark::State &state) {
    Schema s;
    cppxx::json::yy_json::parse(data, s);
    for (auto _ : state) {
        std::ignore = cppxx::json::yy_json::dump(s);
    }
}
BENCHMARK(cppxx_yyjson_serialization);

static void nlohmann_json_serialization(benchmark::State &state) {
    Schema s = nlohmann::json::parse(data);
    for (auto _ : state) {
        nlohmann::json j = s;
        std::ignore      = j.dump();
    }
}
BENCHMARK(nlohmann_json_serialization);

static void cppxx_yyjson_deserialization(benchmark::State &state) {
    std::string payload = data;
    for (auto _ : state) {
        Schema s;
        cppxx::json::yy_json::parse(payload, s);
    }
}
BENCHMARK(cppxx_yyjson_deserialization);

static void nlohmann_json_deserialization(benchmark::State &state) {
    std::string payload = data;
    for (auto _ : state) {
        Schema s = nlohmann::json::parse(payload);
    }
}
BENCHMARK(nlohmann_json_deserialization);
