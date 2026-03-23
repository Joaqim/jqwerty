#include "jqwerty.h"
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <unordered_map>


// ── physical keycode → QWERTY keysym (unchanged logic, correct order) ────────

static bool keycodeToQwertyAsciiSym(int code, bool shift,
                                     fcitx::KeySym &outSym) {
    static const std::unordered_map<int, char> kLetters = {
        {24,'q'},{25,'w'},{26,'e'},{27,'r'},{28,'t'},
        {29,'y'},{30,'u'},{31,'i'},{32,'o'},{33,'p'},
        {38,'a'},{39,'s'},{40,'d'},{41,'f'},{42,'g'},
        {43,'h'},{44,'j'},{45,'k'},{46,'l'},
        {52,'z'},{53,'x'},{54,'c'},{55,'v'},{56,'b'},{57,'n'},{58,'m'},
    };

    // Letters first — critical for dvp where AD03 (code 26) maps to 'period'
    auto it = kLetters.find(code);
    if (it != kLetters.end()) {
        char c = shift ? static_cast<char>(
                             std::toupper(static_cast<unsigned char>(it->second)))
                       : it->second;
        outSym = static_cast<fcitx::KeySym>(c);
        return true;
    }

    // Punctuation by physical position
    switch (code) {
    case 47: outSym = static_cast<fcitx::KeySym>(shift ? ':' : ';');  return true;
    case 48: outSym = static_cast<fcitx::KeySym>(shift ? '"' : '\''); return true;
    case 59: outSym = static_cast<fcitx::KeySym>(shift ? '<' : ',');  return true;
    case 60: outSym = static_cast<fcitx::KeySym>(shift ? '>' : '.');  return true;
    case 61: outSym = static_cast<fcitx::KeySym>(shift ? '?' : '/');  return true;
    default: return false;
    }
}

// ── layout detection ──────────────────────────────────────────────────────────
// Ask fcitx which input method is active for this context.
// If it is the built-in "keyboard-us" with no variant (or variant "basic"),
// we are already on positional QWERTY — no remapping needed.

bool JQwertyAddon::isNonQwertyLayout(fcitx::InputContext *ic) const {
    const auto *entry = instance_->inputMethodEntry(ic);
    if (!entry) return false;                    // no active IM → safe to skip

    // The keyboard IM name is "keyboard-<layout>-<variant>" or "keyboard-<layout>"
    // e.g. "keyboard-us-dvp", "keyboard-us-dvorak", "keyboard-us"
    const std::string &name = entry->uniqueName();
    if (name == "keyboard-us" || name == "keyboard-us-basic")
        return false;                             // plain QWERTY, nothing to do

    // Any other layout: remap
    return name.rfind("keyboard-", 0) == 0;
}

// ── event filter ─────────────────────────────────────────────────────────────

bool JQwertyAddon::filterKey(fcitx::InputContext *ic,
                                     fcitx::KeyEvent &event) {
    if (event.isRelease()) return false;

    const fcitx::Key &key = event.key();
    const auto states = key.states();

    FCITX_INFO() << "IM JQwerty"
                << " received event: " << key.toString();

    const bool hasCtrl  = states.test(fcitx::KeyState::Ctrl);
    const bool hasAlt   = states.test(fcitx::KeyState::Alt);
    const bool hasMeta  = states.test(fcitx::KeyState::Super);
    if (!hasCtrl && !hasAlt && !hasMeta) return false;

    // AltGr (Mod5) without Ctrl/Meta → let the layout produce ä, €, etc.
    if (states.test(fcitx::KeyState::Mod5) && !hasCtrl && !hasMeta)
        return false;

    if (!isNonQwertyLayout(ic)) return false;

    const int  code  = key.code();   // physical keycode — never key.sym()
    const bool shift = states.test(fcitx::KeyState::Shift);

    fcitx::KeySym qwertySym = FcitxKey_VoidSymbol;
    if (!keycodeToQwertyAsciiSym(code, shift, qwertySym))
        return false;

    // Idempotency guard: if layout already agrees with QWERTY for this key,
    // don't re-dispatch — avoids a no-op forward and any re-entry risk.
    if (static_cast<uint32_t>(qwertySym) ==
        static_cast<uint32_t>(key.sym()))
        return false;

        FCITX_INFO() << "IM JQwerty"
                << " Will attempt to convert key: " << key.toString() << " to QWERTY";
                
    ic->forwardKey(fcitx::Key(qwertySym, states), event.isRelease());
    event.filterAndAccept();
    return true;
}

// ── constructor ───────────────────────────────────────────────────────────────

JQwertyAddon::JQwertyAddon(fcitx::Instance *instance)
    : instance_(instance) {
    keyFilterConn_ = instance_->watchEvent(
        fcitx::EventType::InputContextKeyEvent,
        fcitx::EventWatcherPhase::PreInputMethod,
        [this](fcitx::Event &rawEvent) {
            auto &kev = static_cast<fcitx::KeyEvent &>(rawEvent);
            filterKey(kev.inputContext(), kev);
        });
}

// ── factory ───────────────────────────────────────────────────────────────────

fcitx::AddonInstance *
JQwertyAddonFactory::create(fcitx::AddonManager *manager) {
    return new JQwertyAddon(manager->instance());
}

FCITX_ADDON_FACTORY_V2(jqwerty, JQwertyAddonFactory)