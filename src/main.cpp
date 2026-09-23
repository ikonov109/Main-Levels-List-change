#include <Geode/Geode.hpp>
#include <Geode/modify/LevelTools.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <matjson.hpp>
#include <fstream>
#include <sstream>

using namespace geode::prelude;

struct CustomLevel {
    std::string name = "";
    int stars = 0;
    int difficulty = 0;
};

static std::unordered_map<int, CustomLevel> g_custom;
static std::string g_configPath;

// ===================== JSON =====================

static void saveConfig() {
    auto obj = matjson::Value::object();
    for (auto& [id, lvl] : g_custom) {
        auto levelObj = matjson::Value::object();
        levelObj["name"] = lvl.name;
        levelObj["stars"] = lvl.stars;
        levelObj["difficulty"] = lvl.difficulty;
        obj[std::to_string(id)] = levelObj;
    }
    std::ofstream out(g_configPath);
    out << obj.dump(4);
    out.close();
}

static void loadConfig() {
    g_configPath = (Mod::get()->getSaveDir() / "config.json").string();

    if (!std::filesystem::exists(g_configPath)) {
        // Пример: кастомные ID 9001, 9002
        g_custom[1] = {"My Stereo Madness", 5, 3};
        g_custom[2] = {"Back On Track X", 10, 4};
        g_custom[3] = {"Polargeist Pro", 7, 2};
        saveConfig();
        return;
    }

    std::ifstream in(g_configPath);
    std::stringstream buf;
    buf << in.rdbuf();
    in.close();

    auto res = matjson::parse(buf.str());
    if (!res.isOk()) return;

    auto root = res.unwrap();
    if (!root.isObject()) return;

    for (auto& [key, val] : root) {
        int id = 0;
        try { id = std::stoi(key); } catch (...) { continue; }

        CustomLevel lvl;
        lvl.name = val["name"].asString().unwrapOr("");
        lvl.stars = val["stars"].asInt().unwrapOr(0);
        lvl.difficulty = val["difficulty"].asInt().unwrapOr(0);
        g_custom[id] = lvl;
    }
}

// ===================== LevelTools =====================

class $modify(MyLevelTools, LevelTools) {
    // Расширяем список ID официальных уровней
    static gd::unordered_set<int> getLevelList() {
        auto list = LevelTools::getLevelList();
        for (auto& [id, lvl] : g_custom) {
            // Добавляем только ID, которых нет в оригинальном списке
            if (id > 1000) {
                list.insert(id);
            }
        }
        return list;
    }

    // Для кастомных ID создаём уровень с нуля
    static GJGameLevel* getLevel(int id, bool noString) {
        auto it = g_custom.find(id);

        if (it != g_custom.end() && id > 1000) {
            // Создаём новый уровень
            auto level = GJGameLevel::create();
            if (!level) return LevelTools::getLevel(id, noString);

            level->m_levelID = id;
            level->m_levelName = it->second.name;
            level->m_stars = it->second.stars;
            level->m_difficulty = (GJDifficulty)it->second.difficulty;
            level->m_levelType = GJLevelType::Main;
            level->m_levelDesc = "Custom level";
            level->m_creatorName = "Mod";
            level->m_audioTrack = 0;
            level->m_songID = 0;
            level->m_coins = 0;
            level->m_levelLength = 3;
            level->m_isUploaded = false;
            level->m_levelString = "";

            log::info("Created custom level ID {}: {}", id, it->second.name);
            return level;
        }

        auto level = LevelTools::getLevel(id, noString);
        if (!level) return level;

        // Подмена существующих уровней (1-21)
        if (it != g_custom.end()) {
            level->m_levelName = it->second.name;
            level->m_stars = it->second.stars;
            level->m_difficulty = (GJDifficulty)it->second.difficulty;
        }
        return level;
    }
};

// ===================== LevelPage (fallback) =====================

class $modify(MyLevelPage, LevelPage) {
    void updateDynamicPage(GJGameLevel* level) {
        LevelPage::updateDynamicPage(level);
        if (!level) return;

        int id = level->m_levelID;
        auto it = g_custom.find(id);
        if (it == g_custom.end()) return;

        auto& data = it->second;

        if (auto nameNode = this->getChildByID("level-name")) {
            if (auto lbl = typeinfo_cast<CCLabelBMFont*>(nameNode)) {
                lbl->setString(data.name.c_str());
            }
        }

        level->m_levelName = data.name;
        level->m_stars = data.stars;
        level->m_difficulty = (GJDifficulty)data.difficulty;
    }
};

// ===================== Загрузка =====================

$on_mod(Loaded) {
    loadConfig();
    log::info("Main Levels Config loaded, {} custom levels", g_custom.size());
}
