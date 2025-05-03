#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

class $modify(MyPauseLayer, PauseLayer) {
    struct Fields {
        CCLabelBMFont* countdownLabel = nullptr;
        float countdownTime = 0.0f;
        bool isCountingDown = false;
        int lastDisplayedTime = -1;
    };

    void playSound(std::string soundName, const bool isResume) {
        #ifdef GEODE_IS_ANDROID
        if (soundName == "file:///android_asset/sfx/counter003.ogg" && !isResume) {
            soundName = "counter003.ogg";
        } else if (soundName == "file:///android_asset/sfx/playSound_01.ogg" && isResume) {
            soundName = "playSound_01.ogg";
        } else if (!std::filesystem::exists(soundName)) return;
        #elif defined(GEODE_IS_IOS)
        if (soundName.empty() && !isResume)
            soundName = "counter003.ogg";
        } else if (soundName.empty() && isResume) {
            soundName = "playSound_01.ogg";
        } else if (!std::filesystem::exists(soundName)) return;
        #else
        if (!std::filesystem::exists(soundName)) return;
        #endif

        FMOD::Channel* channel;
		FMOD::Sound* sound;
        
        system->createSound(soundName.c_str(), FMOD_DEFAULT, nullptr, &sound);
		system->playSound(sound, nullptr, false, &channel);
		channel->setVolume(volume);
    }

    void onResume(CCObject* sender) {
        const auto fields = m_fields.self();
        if (!fields->isCountingDown) {
            int countdownSeconds = Mod::get()->getSettingValue<int64_t>("countdown-seconds");

            fields->isCountingDown = true;
            fields->countdownTime = static_cast<float>(countdownSeconds);
            fields->lastDisplayedTime = countdownSeconds + 1;

            if (fields->countdownLabel) {
                fields->countdownLabel->removeFromParentAndCleanup(true);
                fields->countdownLabel = nullptr;
            }

            this->setVisible(false);

            fields->countdownLabel = CCLabelBMFont::create(std::to_string(countdownSeconds).c_str(), "bigFont.fnt");
            fields->countdownLabel->setPosition(CCDirector::sharedDirector()->getWinSize() / 2);
            fields->countdownLabel->setScale(2.0f);
            fields->countdownLabel->setZOrder(100);
            fields->countdownLabel->setID("countdown"_spr);
            this->getParent()->addChild(fields->countdownLabel);

            this->schedule(schedule_selector(MyPauseLayer::updateCountdown), 0.1f);

            if (auto playLayer = PlayLayer::get()) {
                playLayer->setVisible(true);
                playLayer->setTouchEnabled(true);
                playLayer->setKeyboardEnabled(true);
            }
        }
    }

    void updateCountdown(float dt) {
        const auto fields = m_fields.self();
        if (!fields->isCountingDown) return;

        fields->countdownTime -= dt;

        if (fields->countdownTime <= 0.0f) {
            fields->isCountingDown = false;
            if (fields->countdownLabel) {
                fields->countdownLabel->removeFromParentAndCleanup(true);
                fields->countdownLabel = nullptr;
            }
            this->unschedule(schedule_selector(MyPauseLayer::updateCountdown));

            if (auto playLayer = PlayLayer::get()) {
                playLayer->resume();
                playLayer->setTouchEnabled(true);
                playLayer->setKeyboardEnabled(true);
                GameManager::sharedState()->setGameVariable("0028", false);
                CCDirector::sharedDirector()->getTouchDispatcher()->setDispatchEvents(true);
                this->removeFromParentAndCleanup(true);
            }

            if (Mod::get()->getSettingValue<bool>("enable-sound") && Mod::get()->getSettingValue<bool>("enable-play-sound")) {
                // original sound: "playSound_01.ogg"
                MyPauseLayer::playSound(Mod::get()->getSettingValue<std::filesystem::path>("resume-sound").string(), true);
            }
        } else {
            int displayTime = static_cast<int>(ceil(fields->countdownTime));
            fields->countdownLabel->setString(std::to_string(displayTime).c_str());

            if (Mod::get()->getSettingValue<bool>("enable-sound") && displayTime != fields->lastDisplayedTime) {
                log::info("Triggering countdown sound for: {}", displayTime);
                // original sound: "counter003.ogg"
                MyPauseLayer::playSound(Mod::get()->getSettingValue<std::filesystem::path>("counter-sound").string(), false);
                fields->lastDisplayedTime = displayTime;
            }
        }
    }

    void onExit() {
        const auto fields = m_fields.self();
        if (fields->countdownLabel) {
            fields->countdownLabel->removeFromParentAndCleanup(true);
            fields->countdownLabel = nullptr;
        }
        this->unschedule(schedule_selector(MyPauseLayer::updateCountdown));
        PauseLayer::onExit();
    }

    void customSetup() {
        PauseLayer::customSetup();
        if (const auto parent = this->getParent(); parent && parent->getChildByID("countdown"_spr)) {
            parent->removeChildByID("countdown"_spr);
        }
    }
};