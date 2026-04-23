#include "StageMapCatalog.hpp"
#include "pugixml.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>



namespace
{

    int selectableStageCount()
    {
        return static_cast<int>(fight::Stage::MAX_ENUM);
    }

}


void renderer::StageMapCatalog::load(const std::string& xmlPath)
{
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(xmlPath.c_str());
    if (!result)
        throw std::runtime_error("Failed to parse file: " + xmlPath);

    _entries.clear();
    std::vector<bool> seen(selectableStageCount(), false);

    for (auto theme : doc.child("ThemeData").children("Theme"))
    {
        std::string themeId = theme.attribute("id").as_string();
        fight::Stage stage = fight::stageFromName(themeId.c_str());
        int stageIndex = static_cast<int>(stage);
        if (stage == fight::Stage::MAX_ENUM || stageIndex < 0 || stageIndex >= selectableStageCount())
            continue;

        std::string icon = theme.child("Icon").text().as_string();
        auto mapPosition = theme.child("MapPosition");
        if (icon.empty() || !mapPosition)
            throw std::runtime_error("Missing stage map metadata for theme: " + themeId);

        if (seen[stageIndex])
            throw std::runtime_error("Duplicate stage map metadata for theme: " + themeId);

        _entries.push_back({
            stage,
            themeId,
            "towerIcons/" + icon,
            {
                mapPosition.attribute("x").as_float(),
                mapPosition.attribute("y").as_float()
            }
        });
        seen[stageIndex] = true;
    }

    for (int i = 0; i < selectableStageCount(); i++)
    {
        if (!seen[i])
            throw std::runtime_error(
                "Missing stage map metadata for stage: " +
                std::string(fight::stageToName(static_cast<fight::Stage>(i))));
    }

    std::sort(_entries.begin(), _entries.end(),
        [](const StageMapEntry& lhs, const StageMapEntry& rhs)
        {
            return static_cast<int>(lhs.stage) < static_cast<int>(rhs.stage);
        });
}


const renderer::StageMapEntry* renderer::StageMapCatalog::find(fight::Stage stage) const
{
    auto it = std::find_if(_entries.begin(), _entries.end(),
        [stage](const StageMapEntry& entry)
        {
            return entry.stage == stage;
        });

    return it == _entries.end() ? nullptr : &*it;
}


const std::vector<renderer::StageMapEntry>& renderer::StageMapCatalog::entries() const
{
    return _entries;
}
