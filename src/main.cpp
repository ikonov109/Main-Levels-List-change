#include <Geode/Geode.hpp>
#include <Geode/modify/LevelPage.hpp>

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

class $modify(MyLevelPage, LevelPage) {
    void updateDynamicPage(GJGameLevel* level) {
        LevelPage::updateDynamicPage(level);

        if (!level) return;

        int id = level->m_levelID;
        auto it = g_custom.find(id);
        if (it == g_custom.end()) return;

        auto& data = it->second;

        if (auto nameLabel = this->getChildByID("level-name")) {
            if (auto lbl = typeinfo_cast<CCLabelBMFont*>(nameLabel)) {
                lbl->setString(data.name.c_str());
            }
        }

        level->m_levelName = data.name;
        level->m_stars = data.stars;
        level->m_difficulty = (GJDifficulty)data.difficulty;
    }
};
