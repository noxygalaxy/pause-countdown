#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>

using namespace geode::prelude;

class $modify(MyPauseLayer, PauseLayer) {
    struct Fields {
        CCLabelBMFont* countdownLabel = nullptr;
        float countdownTime = 0.0f;
        bool isCountingDown = false;
        int lastDisplayedTime = -1;
        std::map<std::string, bool> m_existingSounds;
    };

    void playSound(const std::string soundName, float pitch = 1.0f) {
        auto fmod = FMODAudioEngine::sharedEngine();
        fmod->m_channelGroup2->setPaused(false);
        fmod->m_backgroundMusicChannel->setPaused(false);
        fmod->m_globalChannel->setPaused(false);
        fmod->m_system->update();
        fmod->playEffect(soundName.c_str(), 1, pitch, 1);
    }

    void onResume(CCObject* sender) {
        if (!m_fields->isCountingDown) {
            int countdownSeconds = Mod::get()->getSettingValue<int64_t>("countdown-seconds");

            m_fields->isCountingDown = true;
            m_fields->countdownTime = static_cast<float>(countdownSeconds);
            m_fields->lastDisplayedTime = countdownSeconds + 1;

            if (m_fields->countdownLabel) {
                m_fields->countdownLabel->removeFromParentAndCleanup(true);
                m_fields->countdownLabel = nullptr;
            }

            this->setVisible(false);

            m_fields->countdownLabel = CCLabelBMFont::create(std::to_string(countdownSeconds).c_str(), "bigFont.fnt");
            m_fields->countdownLabel->setPosition(CCDirector::sharedDirector()->getWinSize() / 2);
            m_fields->countdownLabel->setScale(2.0f);
            m_fields->countdownLabel->setZOrder(100);
            this->getParent()->addChild(m_fields->countdownLabel);

            this->schedule(schedule_selector(MyPauseLayer::updateCountdown), 0.1f);

            if (auto playLayer = GameManager::sharedState()->getPlayLayer()) {
                playLayer->setVisible(true);
                playLayer->setTouchEnabled(true);
                playLayer->setKeyboardEnabled(true);
            }
        }
    }

    void updateCountdown(float dt) {
        if (!m_fields->isCountingDown) return;

        m_fields->countdownTime -= dt;

        if (m_fields->countdownTime <= 0.0f) {
            m_fields->isCountingDown = false;
            if (m_fields->countdownLabel) {
                m_fields->countdownLabel->removeFromParentAndCleanup(true);
                m_fields->countdownLabel = nullptr;
            }
            this->unschedule(schedule_selector(MyPauseLayer::updateCountdown));

            if (auto playLayer = GameManager::sharedState()->getPlayLayer()) {
                playLayer->resume();
                playLayer->setTouchEnabled(true);
                playLayer->setKeyboardEnabled(true);
                GameManager::sharedState()->setGameVariable("0028", false);
                CCDirector::sharedDirector()->getTouchDispatcher()->setDispatchEvents(true);
                playLayer->setVisible(true);
                playLayer->setZOrder(0);
                this->removeFromParentAndCleanup(true);
            }
            bool enableSound = Mod::get()->getSettingValue<bool>("enable-sound");
            if (enableSound) {
                auto fmod = FMODAudioEngine::sharedEngine();
                fmod->m_channelGroup2->setPaused(false);
                fmod->m_backgroundMusicChannel->setPaused(false);
                fmod->m_globalChannel->setPaused(false);
                fmod->m_system->update();
                playSound("playSound_01.ogg");
            }
        } else {
            int displayTime = static_cast<int>(ceil(m_fields->countdownTime));
            m_fields->countdownLabel->setString(std::to_string(displayTime).c_str());

            bool enableSound = Mod::get()->getSettingValue<bool>("enable-sound");
            if (enableSound && displayTime != m_fields->lastDisplayedTime) {
                log::info("Triggering countdown sound for: {}", displayTime);
                playSound("counter003.ogg");
                m_fields->lastDisplayedTime = displayTime;
            }
        }
    }

    void onExit() {
        if (m_fields->countdownLabel) {
            m_fields->countdownLabel->removeFromParentAndCleanup(true);
            m_fields->countdownLabel = nullptr;
        }
        this->unschedule(schedule_selector(MyPauseLayer::updateCountdown));
        PauseLayer::onExit();
    }
};