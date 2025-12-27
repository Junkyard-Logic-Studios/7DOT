#include <filesystem>
#include <vector>
#include <gtest/gtest.h>
#include "fight/level.hpp"
using namespace fight;



class LevelTestSuite : public testing::TestWithParam<std::string> 
{
public:
    static const std::vector<std::string>& getAllFilePaths()
    {
        static std::vector<std::string> paths;
        if (!paths.empty())
            return paths;
        
        for (auto& entry : std::filesystem::recursive_directory_iterator(ASSET_DIR "Levels"))
            if (entry.path().extension() == ".oel")
                paths.push_back(entry.path().string());
        
        std::sort(paths.begin(), paths.end());

        return paths;
    }
};


TEST_P(LevelTestSuite, ConstructorTest)
{
    Level level(GetParam());

    ASSERT_EQ(level.width, 32);
    ASSERT_EQ(level.height, 24);
}


INSTANTIATE_TEST_SUITE_P(
    AllSmoke, 
    LevelTestSuite,
    testing::ValuesIn(LevelTestSuite::getAllFilePaths()),
    [](const testing::TestParamInfo<LevelTestSuite::ParamType>& info)
    {
        std::string name = info.param;
        auto from = name.find_last_of('-') + 2;
        auto to = name.find_last_of(".oel") - 3;
        name = name.substr(from, to - from);
        name.erase(name.find_last_of('/'), 1);
        name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());
        return name;
    }
);
