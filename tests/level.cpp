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
        std::filesystem::path p(info.param);
        std::string name = p.parent_path().filename().string() + p.stem().string();
        name.erase(std::remove_if(name.begin(), name.end(), isspace), name.end());
        name = name.substr(name.find('-') + 1);
        return name;
    }
);
