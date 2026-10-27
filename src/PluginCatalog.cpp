#include "fusionframework/Game.h"
#include "fusionframework/PluginCatalog.h"
#include "fusionframework/PluginCatalogRules.h"
#include "fusionframework/Paths.h"

#include <fstream>

namespace fusionframework::catalog {
namespace {
// TESDataHandler::files is the only list that keeps full and light plugins interleaved in true
// load order (compiledFileCollection splits them into two arrays). The engine rebuilds `files`
// on another thread just after kGameDataReady, so it is used only when every node is non-null and
// its active count equals the compiled collection; otherwise fall back to the compiled arrays
// (full plugins, then light), which are always safe.
std::vector<std::string> ActivePluginsInLoadOrder()
{
    std::vector<std::string> names;
    auto* data = RE::TESDataHandler::GetSingleton();
    if (!data) return names;

    const auto compiledCount = data->compiledFileCollection.files.size() + data->compiledFileCollection.smallFiles.size();
    bool consistent = true;
    for (const auto* file : data->files) {
        if (!file) {
            consistent = false;
            break;
        }
        if (file->compileIndex != 0xFF) names.emplace_back(file->filename);
    }
    if (consistent && names.size() == compiledCount) return names;

    names.clear();
    for (const auto* file : data->compiledFileCollection.files)
        if (file) names.emplace_back(file->filename);
    for (const auto* file : data->compiledFileCollection.smallFiles)
        if (file) names.emplace_back(file->filename);
    return names;
}
}  // namespace

std::vector<CatalogDocument> LoadDocuments(std::string_view owner, std::string_view feature)
{
    namespace fs = std::filesystem;
    std::vector<CatalogDocument> documents;
    const auto directory = paths::GameRoot() / "Data/F4SE/Plugins" / fs::path(owner) / fs::path(std::string(feature));
    std::error_code ec;
    if (!fs::is_directory(directory, ec)) return documents;

    std::vector<std::string> fileNames;
    for (const auto& entry : fs::directory_iterator(directory, ec))
        if (entry.is_regular_file(ec)) fileNames.push_back(entry.path().filename().string());

    const auto active = ActivePluginsInLoadOrder();
    for (const auto& fileName : OrderFiles(fileNames, active)) {
        std::ifstream stream{directory / fileName, std::ios::binary};
        auto parsed = nlohmann::json::parse(stream, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object()) {
            spdlog::warn("plugin catalog: {}/{} is not a valid JSON object; skipped", feature, fileName);
            continue;
        }
        documents.push_back({fileName.substr(0, fileName.size() - 5), std::move(parsed)});
    }
    return documents;
}

}  // namespace fusionframework::catalog
