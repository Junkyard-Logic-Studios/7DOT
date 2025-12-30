#pragma once



namespace fight
{
    
    enum class Stage
    {
        SACRED_GROUND  = 0,
        TWILIGHT_SPIRE = 1,
        BACKFIRE       = 2,
        FLIGHT         = 3,
        MIRAGE         = 4,
        THORNWOOD      = 5,
        FROSTFANG_KEEP = 6,
        KINGS_COURT    = 7,
        SUNKEN_CITY    = 8,
        MOONSTONE      = 9,
        TOWERFORGE     = 10,
        ASCENSION      = 11,
        MAX_ENUM
    };


    inline const char* stageToName(Stage stage)
    {
        switch (stage)
        {
        case Stage::SACRED_GROUND:  return "Sacred Ground";  
        case Stage::TWILIGHT_SPIRE: return "Twilight Spire"; 
        case Stage::BACKFIRE:       return "Backfire";       
        case Stage::FLIGHT:         return "Flight";         
        case Stage::MIRAGE:         return "Mirage";         
        case Stage::THORNWOOD:      return "Thornwood";      
        case Stage::FROSTFANG_KEEP: return "Frostfang Keep"; 
        case Stage::KINGS_COURT:    return "Kings Court";    
        case Stage::SUNKEN_CITY:    return "Sunken City";    
        case Stage::MOONSTONE:      return "Moonstone";      
        case Stage::TOWERFORGE:     return "Towerforge";     
        case Stage::ASCENSION:      return "Ascension";
        default: return "[Unknown Stage]";
        };   
    }


    inline Stage stageFromName(const char* name)
    {
        auto fnHash = [](const char* str) constexpr -> int
        {
            int i = 1;
            while (*str != '\0') 
            {
                char c = *str;
                c += (c >= 'A' && c <= 'Z') * ('a' - 'A');
                i *= (*str != ' ') * (long int)c + 1;
                str++;
            }
            return i;
        };

        switch (fnHash(name))
        {
        case fnHash("SacredGround"):
        case fnHash("SacredGroundBG"):  return Stage::SACRED_GROUND;
        case fnHash("TwilightSpire"):
        case fnHash("TwilightSpireBG"): return Stage::TWILIGHT_SPIRE;
        case fnHash("Backfire"):
        case fnHash("BackfireBG"):      return Stage::BACKFIRE;
        case fnHash("Flight"):
        case fnHash("FlightBG"):        return Stage::FLIGHT;
        case fnHash("Mirage"):
        case fnHash("MirageBG"):        return Stage::MIRAGE;
        case fnHash("Thornwood"):
        case fnHash("ThornwoodBG"):     return Stage::THORNWOOD;
        case fnHash("FrostfangKeep"):
        case fnHash("FrostfangKeepBG"): return Stage::FROSTFANG_KEEP;
        case fnHash("KingsCourt"):
        case fnHash("KingsCourtBG"):    return Stage::KINGS_COURT;
        case fnHash("SunkenCity"):
        case fnHash("SunkenCityBG"):    return Stage::SUNKEN_CITY;
        case fnHash("Moonstone"):
        case fnHash("MoonstoneBG"):     return Stage::MOONSTONE;
        case fnHash("TowerForge"):
        case fnHash("TowerForgeBG"):    return Stage::TOWERFORGE;
        case fnHash("Ascension"):
        case fnHash("AscensionBG"):     return Stage::ASCENSION;
        default: return Stage::MAX_ENUM;
        }
    }

};  // end namespace fight
