#pragma once

#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/event.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>

namespace fcitx {

class JQwertyEngine : public InputMethodEngineV2 {
public:
    explicit JQwertyEngine(Instance *instance);
    ~JQwertyEngine() override = default;

    // InputMethodEngineV2 interface
    void keyEvent(const InputMethodEntry &entry, KeyEvent &keyEvent) override;
    void activate(const InputMethodEntry &entry,
                  InputContextEvent &event) override;
    void deactivate(const InputMethodEntry &entry,
                    InputContextEvent &event) override;
    void reset(const InputMethodEntry &entry,
               InputContextEvent &event) override;

    Instance *instance() { return instance_; }

private:
    Instance *instance_;
};

class JQwertyEngineFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new JQwertyEngine(manager->instance());
    }
};

} // namespace fcitx
