#pragma once



namespace fight
{
    
    enum class Mode
    {
        LAST_MAN_STANDING,
        HEAD_HUNTERS,
        TEAM_2,
        TEAM_4,
        MAX_ENUM
    };

    inline const char* ModeName(Mode mode)
    {
        switch (mode)
        {
        case Mode::LAST_MAN_STANDING: return "Last Man Standing";
        case Mode::HEAD_HUNTERS:      return "Head Hunters";
        case Mode::TEAM_2:            return "2 Teams Deathmatch";
        case Mode::TEAM_4:            return "4 Teams Deathmatch";
        default: return "[Unknown Mode]";
        }
    }

};  // end namespace fight
