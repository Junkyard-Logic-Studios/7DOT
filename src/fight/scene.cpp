#include <filesystem>
#include <algorithm>
#include "scene.hpp"
#include "context.hpp"
#include "../renderer/fightRenderer.hpp"
#include "../game.hpp"



fight::Scene::Scene(Game& game) :
    _SyncedScene(game)
{
    auto* renderer = new renderer::FightRenderer(_game.getWindow().get(), _game.getRenderer().get(), *this);
    _renderer.reset(static_cast<renderer::_Renderer<State>*>(renderer));
}

const fight::Level& fight::Scene::getLevel(std::size_t index) const
{
    return _levels[index];
}

void fight::Scene::_activate(SceneContext& context)
{
    _fightSelection = static_cast<Context&>(context).fightSelection;
    static_cast<renderer::FightRenderer*>(_renderer.get())->
        setFightSelectionInfo(_fightSelection);

    // load levels for stage
    auto dir = std::filesystem::path(ASSET_DIR "Levels");
    switch (_fightSelection.stage)
    {
    case FightSelectionInfo::Stage::SACRED_GROUND:  
        dir /= std::filesystem::path("00 - Sacred Ground");  break;
    case FightSelectionInfo::Stage::TWILIGHT_SPIRE: 
        dir /= std::filesystem::path("01 - Twilight Spire"); break;
    case FightSelectionInfo::Stage::BACKFIRE:       
        dir /= std::filesystem::path("02 - Backfire");       break;
    case FightSelectionInfo::Stage::FLIGHT:         
        dir /= std::filesystem::path("03 - Flight");         break;
    case FightSelectionInfo::Stage::MIRAGE:         
        dir /= std::filesystem::path("04 - Mirage");         break;
    case FightSelectionInfo::Stage::THORNWOOD:      
        dir /= std::filesystem::path("05 - Thornwood");      break;
    case FightSelectionInfo::Stage::FROSTFANG_KEEP: 
        dir /= std::filesystem::path("06 - Frostfang Keep"); break;
    case FightSelectionInfo::Stage::KINGS_COURT:    
        dir /= std::filesystem::path("07 - Kings Court");    break;
    case FightSelectionInfo::Stage::SUNKEN_CITY:    
        dir /= std::filesystem::path("08 - Sunken City");    break;
    case FightSelectionInfo::Stage::MOONSTONE:      
        dir /= std::filesystem::path("09 - Moonstone");      break;
    case FightSelectionInfo::Stage::TOWERFORGE:     
        dir /= std::filesystem::path("10 - TowerForge");     break;
    case FightSelectionInfo::Stage::ASCENSION:      
        dir /= std::filesystem::path("11 - Ascension");      break;
    };

    std::vector<std::string> files;
    for (auto& entry : std::filesystem::directory_iterator(dir))
        if (entry.path().extension() == ".oel")
            files.push_back(entry.path().string());

    std::sort(files.begin(), files.end());
    _levels.reserve(files.size());
    for (auto& file : files)
        _levels.emplace_back(file);
}

void fight::Scene::_deactivate()
{
    _levels.clear();
}

_Scene::UpdateReturnStatus fight::Scene::computeFollowingState(const State& givenState, State& followingState, tick_t tick)
{ 
    return tick % (3000 / MS_PER_TICK) ? 
        UpdateReturnStatus::STAY : 
        UpdateReturnStatus::SWITCH_SELECTION;
}
