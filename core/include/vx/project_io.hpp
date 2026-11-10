#pragma once

#include <string>

#include "vx/model.hpp"

namespace vx {

// The project manifest: a versioned JSON document.
//
// Times are written as integer nanoseconds, not seconds, so a saved project
// reloads to exactly the same frame it was saved on. Seconds as decimals would
// round, and rounding is how a trim comes back one frame short.
//
// Readers ignore fields they do not know and refuse a version they do not
// understand. That combination is what lets an old build open a newer project
// once the schema only gained optional fields, and stops it from silently
// mangling one that changed shape.
constexpr int kProjectFormatVersion = 1;
extern const char* const kProjectFormatName;

struct ParseResult {
    bool ok = false;
    Project project;
    std::string error;
};

std::string toJson(const Project& project);
ParseResult parseProjectJson(const std::string& json);

bool saveProjectToFile(const Project& project, const std::string& path,
                       std::string* error = nullptr);
bool loadProjectFromFile(const std::string& path, Project& project, std::string* error = nullptr);

}  // namespace vx
