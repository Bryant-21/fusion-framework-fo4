#pragma once

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace fusionframework::catalog {

struct CatalogDocument {
    std::string plugin;  // file name with extension, as the load order spells it
    nlohmann::json json;
};

// Documents of Data/F4SE/Plugins/<owner>/<feature>/<plugin>.json whose plugin is active, in load
// order. Unparseable files are logged and skipped; a missing folder yields an empty list.
// Call at kGameDataReady or later.
std::vector<CatalogDocument> LoadDocuments(std::string_view owner, std::string_view feature);

}  // namespace fusionframework::catalog
