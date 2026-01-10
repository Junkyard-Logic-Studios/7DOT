#include "fightRenderer.hpp"
#include "pugixml.hpp"
#include <algorithm>
#include <unordered_map>
#include <vector>



struct Background
{
    struct Element
    {
        std::string name;
        float x;
        float y;
        std::string content;
    };

    SDL_Color color;
    std::vector<Element> elements;
};


inline Background& getBackground(fight::Stage stage)
{
    static std::unordered_map<fight::Stage, Background> backgrounds;

    // parse background data XML
    if (backgrounds.empty())
    {
        // load document
        pugi::xml_document doc;
        const char* path = ASSET_DIR "Atlas/GameData/bgData.xml";
        pugi::xml_parse_result result = doc.load_file(path);
        if (!result)
            throw std::runtime_error("Failed to parse file: " + std::string(path));

        // iterate over backgrounds
        for (auto& BG : doc.child("backgrounds").children("BG"))
        {
            auto id = BG.attribute("id").as_string();
            fight::Stage st = fight::stageFromName(id);

            int col = std::stoi(BG.child("Background").attribute("bgColor").as_string(), 0, 16);

            Background background;
            background.color.r = col / 256 / 256;
            background.color.g = col / 256 % 256;
            background.color.b = col % 256;
            background.color.a = 255;

            for (auto& child : BG.child("Background").children())
                background.elements.emplace_back(
                    std::string(child.name()),
                    child.attribute("x").as_float(),
                    child.attribute("y").as_float(),
                    std::string(child.text().as_string()));

            backgrounds[st] = background;
        }
    }

    return backgrounds[stage];
}


renderer::FightRenderer::FightRenderer(
    SDL_Window* const window, 
    SDL_Renderer* const renderer,
    const fight::Scene& scene
) :
    _Renderer(window, renderer),
    _scene(scene)
{
    _atlas.load(_sdlRenderer, ASSET_DIR "Atlas/atlas.bmp", ASSET_DIR "Atlas/atlas.xml");
    _bgAtlas.load(_sdlRenderer, ASSET_DIR "Atlas/bgAtlas.bmp", ASSET_DIR "Atlas/bgAtlas.xml");

    // trigger loading backgrounds
    getBackground(fight::Stage::SACRED_GROUND);
}

renderer::FightRenderer::~FightRenderer()
{
    _atlas.unload();
    _bgAtlas.unload();
}

void renderer::FightRenderer::pushState(State state)
    { this->_state = state; }

void renderer::FightRenderer::render()
{
    Background& background = getBackground(_scene.getStage());

    // reset drawing
    SDL_SetRenderDrawColor(_sdlRenderer, background.color.r, 
        background.color.g, background.color.b, background.color.a);
    SDL_RenderClear(_sdlRenderer);
    SDL_SetRenderDrawColor(_sdlRenderer, 255, 255, 255, 255);

    int winw, winh;
    SDL_GetWindowSize(_sdlWindow, &winw, &winh);
    SDL_FRect screenRect = { 0.0f, 0.0f, (float)winw, (float)winh };

    auto& level = _scene.getLevel(_state.levelIndex);

    // immediate background elements
    for (auto& elem : background.elements)
    {
        SDL_FRect rect = screenRect;
        rect.x = elem.x;
        rect.y = elem.y;
        if (elem.name == "Backdrop" || elem.name == "WavyLayer")
            _bgAtlas.draw(_sdlRenderer, elem.content, &rect);
    }

    // tileset name
    std::string tsname(fight::stageToName(_scene.getStage()));
    tsname[0] = std::tolower(tsname[0]);
    tsname.erase(std::remove_if(tsname.begin(), tsname.end(), isspace), tsname.end());
    
    // background tilemap
    for (int y = 0; y < level.getHeight(); y++)
        for (int x = 0; x < level.getWidth(); x++)
            _atlas.drawTile(_sdlRenderer, "tilesets/" + tsname + "BG",
                level.getBackgroundAt(x, y), x, y);

    // foreground tilemap
    for (int y = 0; y < level.getHeight(); y++)
        for (int x = 0; x < level.getWidth(); x++)
            _atlas.drawTile(_sdlRenderer, "tilesets/" + tsname,
                level.getSolidAt(x, y), x, y);

    // archers
    for (const auto& archer : _state.archers)
        _atlas.draw(_sdlRenderer, "arrows/laserArrow", archer.position.x * 3.0, 
            archer.position.y * 3.0, 3.0f, !archer.isFacingRight);

    SDL_RenderPresent(_sdlRenderer);
}
