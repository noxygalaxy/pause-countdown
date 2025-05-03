#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

#define ACTUALLY_RESUME_THE_GAME\
    if (Mod::get()->getSettingValue<bool>("enable-sound") && Mod::get()->getSettingValue<bool>("enable-play-sound")) {\
        MyPauseLayer::playSound(Mod::get()->getSettingValue<std::filesystem::path>("resume-sound").string(), true);\
    }\
    return PauseLayer::onResume(sender);

class $modify(MyPauseLayer, PauseLayer) {
    struct Fields {
        CCLabelBMFont* countdownLabel = nullptr;
        float countdownTime = 0.0f;
        bool isCountingDown = false;
        int lastDisplayedTime = -1;
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
        if (!Mod::get()->getSettingValue<bool>("enabled")) return PauseLayer::onResume(sender);
		if (CCNode* nodeSender = typeinfo_cast<CCNode*>(sender); sender && nodeSender) {
            if (nodeSender->getTag() == 5032025) ACTUALLY_RESUME_THE_GAME
			if (nodeSender->getID() == "recursion-prevention"_spr) ACTUALLY_RESUME_THE_GAME
			if (nodeSender->getUserObject("recursion-prevention"_spr)) ACTUALLY_RESUME_THE_GAME
		}
        const auto fields = m_fields.self();
        if (!fields->isCountingDown) {
            int countdownSeconds = Mod::get()->getSettingValue<int64_t>("countdown-seconds");

            fields->isCountingDown = true;
            fields->countdownTime = static_cast<float>(countdownSeconds);
            fields->lastDisplayedTime = countdownSeconds + 1;

            if (fields->countdownLabel) {
                fields->countdownLabel->removeMeAndCleanup();
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
        }
    }

    void updateCountdown(float dt) {
        const auto fields = m_fields.self();
        if (!fields->isCountingDown) return;

        fields->countdownTime -= dt;

        if (fields->countdownTime <= 0.0f) {
            fields->isCountingDown = false;
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

    void customSetup() {
        PauseLayer::customSetup();
        if (const auto parent = this->getParent(); parent && parent->getChildByID("countdown"_spr)) {
            parent->removeChildByID("countdown"_spr);
        }
    }
};

class $modify(MyPlayLayer, PlayLayer) {
    void onQuit() {
        if (const auto countdownLabel = CCScene::get()->getChildByIDRecursive("countdown"_spr)) countdownLabel->removeMeAndCleanup();
        PlayLayer::onQuit();
    }
};