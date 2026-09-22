#include <Geode/Geode.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Popup.hpp>
#include <matjson.hpp>
#include <fstream>

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

// ===================== UI =====================

class MLCWindow : public geode::Popup {
protected:
    bool setup() override {
        this->setTitle("Main Levels Config");

        auto winSize = m_mainLayer->getContentSize();

        // Поля ввода
        auto nameInput = TextInput::create(220.f, "Name");
        nameInput->setPosition({winSize.width / 2, winSize.height - 80.f});
        nameInput->setID("mlc-name");
        m_mainLayer->addChild(nameInput);

        auto starsInput = TextInput::create(100.f, "Stars");
        starsInput->setPosition({winSize.width / 2 - 70.f, winSize.height - 120.f});
        starsInput->setID("mlc-stars");
        m_mainLayer->addChild(starsInput);

        auto diffInput = TextInput::create(100.f, "Diff 0-5");
        diffInput->setPosition({winSize.width / 2 + 70.f, winSize.height - 120.f});
        diffInput->setID("mlc-diff");
        m_mainLayer->addChild(diffInput);

        // Кнопка Save
        auto saveBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Save"),
            this,
            menu_selector(MLCWindow::onSave)
        );
        auto menu = CCMenu::create();
        menu->addChild(saveBtn);
        menu->setPosition({winSize.width / 2, winSize.height - 170.f});
        m_mainLayer->addChild(menu);

        return true;
    }

    void onSave(CCObject*) {
        // Заглушка для теста
        auto input = typeinfo_cast<TextInput*>(m_mainLayer->getChildByID("mlc-name"));
        if (input) {
            log::info("Input text: {}", input->getString());
        }
    }

public:
    static MLCWindow* create() {
        auto ret = new MLCWindow();
        if (ret && ret->init(400.f, 280.f)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

// ===================== Кнопка =====================

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

// ===================== Применение =====================

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

$on_mod(Loaded) {
    loadConfig();
    log::info("Main Levels List Change loaded");
}
