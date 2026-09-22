#include <Geode/Geode.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <fstream>

using namespace geode::prelude;
using namespace matjson;

struct CustomLevel {
    std::string name = "";
    int stars = 0;
    int difficulty = 0;
};

static std::unordered_map<int, CustomLevel> g_custom;
static std::string g_configPath;

// ===================== JSON =====================

static void saveConfig() {
    matjson::Value root = matjson::Value::object();
    for (auto& [id, lvl] : g_custom) {
        matjson::Value obj = matjson::Value::object();
        obj["name"] = lvl.name;
        obj["stars"] = lvl.stars;
        obj["difficulty"] = lvl.difficulty;
        root[std::to_string(id)] = obj;
    }
    std::ofstream out(g_configPath);
    out << root.dump();
    out.close();
    log::info("Config saved to {}", g_configPath);
}

static void loadConfig() {
    g_configPath = (Mod::get()->getSaveDir() / "config.json").string();

    if (!std::filesystem::exists(g_configPath)) {
        // дефолт: три уровня
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
    if (!res.isOk()) {
        log::error("Failed to parse config");
        return;
    }

    auto root = res.unwrap();
    if (!root.isObject()) return;

    for (auto& [key, val] : root) {
        int id = std::stoi(key);
        CustomLevel lvl;
        lvl.name = val["name"].asString().unwrapOr("");
        lvl.stars = val["stars"].asInt().unwrapOr(0);
        lvl.difficulty = val["difficulty"].asInt().unwrapOr(0);
        g_custom[id] = lvl;
    }
    log::info("Loaded {} custom levels", g_custom.size());
}

// ===================== UI =====================

class MLCWindow : public geode::Popup<> {
protected:
    int m_selectedID = 1;
    TextInput* m_nameInput = nullptr;
    TextInput* m_starsInput = nullptr;
    TextInput* m_diffInput = nullptr;
    CCLabelBMFont* m_statusLabel = nullptr;

    bool setup() override {
        this->setTitle("Main Levels Config");

        auto winSize = m_mainLayer->getContentSize();

        // Поле ID
        auto idLabel = CCLabelBMFont::create("Level ID:", "bigFont.fnt");
        idLabel->setScale(0.4f);
        idLabel->setPosition({60, winSize.height - 50});
        m_mainLayer->addChild(idLabel);

        m_nameInput = TextInput::create(200, "Name");
        m_nameInput->setPosition({winSize.width / 2, winSize.height - 90});
        m_nameInput->setID("mlc-name");
        m_mainLayer->addChild(m_nameInput);

        m_starsInput = TextInput::create(100, "Stars");
        m_starsInput->setPosition({winSize.width / 2 - 60, winSize.height - 130});
        m_starsInput->setID("mlc-stars");
        m_mainLayer->addChild(m_starsInput);

        m_diffInput = TextInput::create(100, "Diff 0-5");
        m_diffInput->setPosition({winSize.width / 2 + 60, winSize.height - 130});
        m_diffInput->setID("mlc-diff");
        m_mainLayer->addChild(m_diffInput);

        // Кнопка Save
        auto saveBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Save"),
            this,
            menu_selector(MLCWindow::onSave)
        );
        auto menu = CCMenu::create();
        menu->addChild(saveBtn);
        menu->setPosition({winSize.width / 2, winSize.height - 180});
        m_mainLayer->addChild(menu);

        // Статус
        m_statusLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_statusLabel->setScale(0.3f);
        m_statusLabel->setPosition({winSize.width / 2, 30});
        m_mainLayer->addChild(m_statusLabel);

        return true;
    }

    void onSave(CCObject*) {
        int id = m_selectedID;
        CustomLevel lvl;
        lvl.name = m_nameInput->getString();
        lvl.stars = std::stoi(m_starsInput->getString());
        lvl.difficulty = std::stoi(m_diffInput->getString());

        g_custom[id] = lvl;
        saveConfig();

        if (m_statusLabel) {
            m_statusLabel->setString(("Saved ID " + std::to_string(id)).c_str());
        }
    }

public:
    static MLCWindow* create() {
        auto ret = new MLCWindow();
        if (ret && ret->init(400, 280)) {
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

// ===================== Применение к уровням =====================

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
    log::info("Main Levels List Change loaded");
}
