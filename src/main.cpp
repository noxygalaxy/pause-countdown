#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

float countdownTime = 0.0f;
bool isCountingDown = false;
int lastDisplayedTime = -1;

void resetVariables() {
    countdownTime = 0.0f;
    isCountingDown = false;
    lastDisplayedTime = -1;
    CCScene* scene = CCScene::get();
    if (!scene) return;
    CCNode* countdownLabel = scene->getChildByIDRecursive("countdown"_spr);
    if (!countdownLabel) return;
    countdownLabel->removeMeAndCleanup();
}

class $modify(MyPauseLayer, PauseLayer) {
    struct Fields {
        CCLabelBMFont* countdownLabel = nullptr;
    };

    void playSound(std::string soundName, const bool isResume) {
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;
        if (soundName.empty()) {
            #ifdef GEODE_IS_MOBILE
            if (isResume) soundName = "playSound_01.ogg"_spr;
            else soundName = "counter003.ogg"_spr;
            #else
            return;
            #endif
        } else if (!std::filesystem::exists(soundName)) return;

        auto system = FMODAudioEngine::get()->m_system;
        FMOD::Channel* channel;
		FMOD::Sound* sound;
        
        system->createSound(soundName.c_str(), FMOD_DEFAULT, nullptr, &sound);
		system->playSound(sound, nullptr, false, &channel);
		channel->setVolume(Mod::get()->getSettingValue<int64_t>("volume") / 100.0f);
    }

    void onResume(CCObject* sender) {
        log::info("countdownTime: {}", countdownTime);
        log::info("isCountingDown: {}", isCountingDown);
        log::info("lastDisplayedTime: {}", lastDisplayedTime);
        if (!Mod::get()->getSettingValue<bool>("enabled")) return PauseLayer::onResume(sender);
		if (CCNode* nodeSender = typeinfo_cast<CCNode*>(sender); sender && nodeSender) {
            if (nodeSender->getTag() == 5032025 && nodeSender->getID() == "recursion-prevention"_spr && nodeSender->getUserObject("recursion-prevention"_spr)) {
                countdownTime = 0.0f;
                isCountingDown = false;
                lastDisplayedTime = -1;
                if (Mod::get()->getSettingValue<bool>("enable-sound") && Mod::get()->getSettingValue<bool>("enable-play-sound")) {
                    MyPauseLayer::playSound(Mod::get()->getSettingValue<std::filesystem::path>("resume-sound").string(), true);
                }
                return PauseLayer::onResume(nullptr);
            }
		}
        const auto fields = m_fields.self();
        if (!isCountingDown) {
            const int countdownSeconds = Mod::get()->getSettingValue<int64_t>("countdown-seconds");

            isCountingDown = true;
            countdownTime = static_cast<float>(countdownSeconds);
            lastDisplayedTime = countdownSeconds + 1;

            if (fields->countdownLabel) {
                fields->countdownLabel->removeMeAndCleanup();
                fields->countdownLabel = nullptr;
            }

            this->setScale(0.f);

            fields->countdownLabel = CCLabelBMFont::create(std::to_string(countdownSeconds).c_str(), "bigFont.fnt");
            fields->countdownLabel->setPosition(CCDirector::sharedDirector()->getWinSize() / 2);
            fields->countdownLabel->setScale(2.0f);
            fields->countdownLabel->setZOrder(100);
            fields->countdownLabel->setID("countdown"_spr);
            this->getParent()->addChild(fields->countdownLabel);

            this->schedule(schedule_selector(MyPauseLayer::updateCountdown), 0.1f);
        }
    }

    void updateCountdown(float dt) {
        const auto fields = m_fields.self();
        if (!isCountingDown) return;

        countdownTime -= dt;

        if (countdownTime <= 0.0f) {
            isCountingDown = false;
            if (fields->countdownLabel) {
                fields->countdownLabel->removeMeAndCleanup();
                fields->countdownLabel = nullptr;
            }
            this->unschedule(schedule_selector(MyPauseLayer::updateCountdown));

            CCNode* fakeSender = CCNode::create();
            fakeSender->setTag(5032025);
            fakeSender->setID("recursion-prevention"_spr);
            fakeSender->setUserObject("recursion-prevention"_spr, CCBool::create(true));
            PauseLayer::onResume(fakeSender);
        } else {
            int displayTime = static_cast<int>(ceil(countdownTime));
            fields->countdownLabel->setString(std::to_string(displayTime).c_str());

            if (Mod::get()->getSettingValue<bool>("enable-sound") && displayTime != lastDisplayedTime) {
                log::info("Triggering countdown sound for: {}", displayTime);
                // original sound: "counter003.ogg"
                MyPauseLayer::playSound(Mod::get()->getSettingValue<std::filesystem::path>("counter-sound").string(), false);
                lastDisplayedTime = displayTime;
            }
        }
    }

    void customSetup() {
        PauseLayer::customSetup();
        if (CCNode* label = CCScene::get()->getChildByID("countdown"_spr)) {
            label->removeMeAndCleanup();
            label = nullptr;
        }
    }

    void onEdit(cocos2d::CCObject* sender) {
        resetVariables();
        PauseLayer::onEdit(sender);
    }
    void onNormalMode(cocos2d::CCObject* sender) {
        resetVariables();
        PauseLayer::onNormalMode(sender);
    }
    void onPracticeMode(cocos2d::CCObject* sender) {
        resetVariables();
        PauseLayer::onPracticeMode(sender);
    }
    void onRestart(cocos2d::CCObject* sender) {
        resetVariables();
        PauseLayer::onRestart(sender);
    }
    void onRestartFull(cocos2d::CCObject* sender) {
        resetVariables();
        PauseLayer::onRestartFull(sender);
    }
    void onQuit(cocos2d::CCObject* sender) {
        resetVariables();
        PauseLayer::onQuit(sender);
    }
    void FLAlert_Clicked(FLAlertLayer* alert, bool buttonTwo) {
        if (Mod::get()->getSettingValue<bool>("enabled") && buttonTwo) resetVariables();
        PauseLayer::FLAlert_Clicked(alert, buttonTwo);
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void onQuit() {
        resetVariables();
        PlayLayer::onQuit();
    }
};