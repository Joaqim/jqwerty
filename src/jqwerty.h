#pragma once
#include <fcitx/addoninstance.h>
#include <fcitx/addonfactory.h>
#include <fcitx/event.h>
#include <fcitx/instance.h>            // fcitx::Instance, watchEvent
#include <fcitx-utils/signals.h>       // EventConnection

namespace fcitx { class InputContext; }

class JQwertyAddon : public fcitx::AddonInstance {
public:
    explicit JQwertyAddon(fcitx::Instance *instance);
    ~JQwertyAddon() override = default;

    using KeyFilterConnection = std::unique_ptr<fcitx::HandlerTableEntry<std::function<void(fcitx::Event&)>>>;

private:
    fcitx::Instance *instance_;
    KeyFilterConnection keyFilterConn_;

    bool filterKey(fcitx::InputContext *ic, fcitx::KeyEvent &event);
    bool isNonQwertyLayout(fcitx::InputContext *ic) const;
};

class JQwertyAddonFactory : public fcitx::AddonFactory {
public:
    fcitx::AddonInstance *create(fcitx::AddonManager *manager) override;
};