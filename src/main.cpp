#include <Geode/Geode.hpp>
#include <Geode/modify/LevelTools.hpp>

using namespace geode::prelude;

struct CustomLevel {
    std::string name;
    int stars;
    int difficulty;
};

static std::unordered_map<int, CustomLevel> g_custom = {
    {1, {"Мой Stereo Madness", 5, 3}},
    {2, {"Back On Track X", 10, 4}},
    {3, {"Polargeist Pro", 7, 2}},
};

class $modify(MyLevelTools, LevelTools) {
    static gd::string getName(int id) {
        auto it = g_custom.find(id);
        if (it != g_custom.end())
            return gd::string(it->second.name);
        return LevelTools::getName(id);
    }

    static int getStars(int id) {
        auto it = g_custom.find(id);
        if (it != g_custom.end())
            return it->second.stars;
        return LevelTools::getStars(id);
    }

    static int getDifficulty(int id) {
        auto it = g_custom.find(id);
        if (it != g_custom.end())
            return it->second.difficulty;
        return LevelTools::getDifficulty(id);
    }
};

$on_mod(Loaded) {
    log::info("Main Levels List Change loaded");
}
