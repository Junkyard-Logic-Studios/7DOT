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

    inline const char* StageName(Stage stage)
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

};  // end namespace fight
