#include <Geode/Geode.hpp>
#include <Geode/modify/LevelTools.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Popup.hpp>
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
        g_custom[1] = {"NA NA BANA MADNESS", 0, 0};
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

// ===================== UI =====================

class MLCWindow : public geode::Popup {
protected:
    TextInput* m_idInput = nullptr;
    TextInput* m_nameInput = nullptr;
    TextInput* m_starsInput = nullptr;
    TextInput* m_diffInput = nullptr;

    bool init(float width, float height) {
        if (!Popup::init(width, height))
            return false;

        this->setTitle("Main Levels Config");

        auto winSize = m_mainLayer->getContentSize();

        m_idInput = TextInput::create(80.f, "ID");
        m_idInput->setPosition({winSize.width / 2 - 120.f, winSize.height - 70.f});
        m_idInput->setID("mlc-id");
        m_mainLayer->addChild(m_idInput);

        m_nameInput = TextInput::create(180.f, "Name");
        m_nameInput->setPosition({winSize.width / 2 + 30.f, winSize.height - 70.f});
        m_nameInput->setID("mlc-name");
        m_mainLayer->addChild(m_nameInput);

        m_starsInput = TextInput::create(100.f, "Stars");
        m_starsInput->setPosition({winSize.width / 2 - 70.f, winSize.height - 120.f});
        m_starsInput->setID("mlc-stars");
        m_mainLayer->addChild(m_starsInput);

        m_diffInput = TextInput::create(100.f, "Diff 0-5");
        m_diffInput->setPosition({winSize.width / 2 + 70.f, winSize.height - 120.f});
        m_diffInput->setID("mlc-diff");
        m_mainLayer->addChild(m_diffInput);

        auto saveBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Save"),
            this,
            menu_selector(MLCWindow::onSave)
        );
        auto menu = CCMenu::create();
        menu->addChild(saveBtn);
        menu->setPosition({winSize.width / 2, winSize.height - 175.f});
        m_mainLayer->addChild(menu);

        return true;
    }

    void onSave(CCObject*) {
        if (!m_idInput || !m_nameInput || !m_starsInput || !m_diffInput) return;

        int id = 0;
        try { id = std::stoi(m_idInput->getString()); } catch (...) { return; }

        CustomLevel lvl;
        lvl.name = m_nameInput->getString();
        try { lvl.stars = std::stoi(m_starsInput->getString()); } catch (...) { lvl.stars = 0; }
        try { lvl.difficulty = std::stoi(m_diffInput->getString()); } catch (...) { lvl.difficulty = 0; }

        g_custom[id] = lvl;
        saveConfig();

        log::info("Saved ID {}: {}", id, lvl.name);
    }

public:
    static MLCWindow* create() {
        auto ret = new MLCWindow();
        if (ret && ret->init(420.f, 280.f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// ===================== Кнопка в меню =====================

class $modify(MyLevelSelectLayer, LevelSelectLayer) {
    bool init(int page) {
        if (!LevelSelectLayer::init(page)) return false;

        auto btn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("MLC"),
            this,
            menu_selector(MyLevelSelectLayer::onOpenMLC)
        );
        btn->setID("mlc-button");

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
        } else {
            auto newMenu = CCMenu::create();
            newMenu->addChild(btn);
            newMenu->setPosition({50, 50});
            this->addChild(newMenu);
        }
        return true;
    }

    void onOpenMLC(CCObject*) {
        MLCWindow::create()->show();
    }
};

// ===================== LevelTools =====================

class $modify(MyLevelTools, LevelTools) {
    static gd::unordered_set<int> getLevelList() {
        auto list = LevelTools::getLevelList();
        for (auto& [id, lvl] : g_custom) {
            if (id > 1000) list.insert(id);
        }
        return list;
    }

    static GJGameLevel* getLevel(int id, bool noString) {
        auto it = g_custom.find(id);

        if (it != g_custom.end() && id > 1000) {
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

            log::info("Created custom level ID {}: {}", id, it->second.name);
            return level;
        }

        return LevelTools::getLevel(id, noString);
    }
};

// ===================== LevelPage =====================

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
