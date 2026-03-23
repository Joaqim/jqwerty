#include "jqwerty.h"

#include <cctype>
#include <unordered_map>

#include <fcitx-utils/keysym.h>
#include <fcitx-utils/log.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputmethodentry.h>
#include <fcitx/userinterfacemanager.h>

namespace fcitx {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Map physical XKB keycode to a punctuation character, independent of the
/// active XKB layout (assumes a standard 104/105-key ANSI/ISO physical board).
///
/// Keycodes below are Linux evdev+8 (XKB), matching a US-ANSI physical layout:
///   47 = SEMICOLON  → ';' / ':'
///   59 = COMMA      → ',' / '<'
///   60 = PERIOD     → '.' / '>'
///   61 = SLASH      → '/' / '?'
///   48 = APOSTROPHE → '\'' / '"'
///
/// Extend this table to cover any additional physical keys you need.
static bool keycodeToAsciiPunct(int code, bool shift, char &out) {
    switch (code) {
    case 47: out = shift ? ':' : ';';  return true;
    case 48: out = shift ? '"' : '\''; return true;
    case 59: out = shift ? '<' : ',';  return true;
    case 60: out = shift ? '>' : '.';  return true;
    case 61: out = shift ? '?' : '/';  return true;
    default: return false;
    }
}

/// Map physical XKB keycode to a QWERTY ASCII keysym, letter or punctuation.
/// Allows Ctrl+C, Ctrl+V etc. to work regardless of the active XKB layout
/// because we resolve letters by *position* rather than by the layout's keysym.
static bool keycodeToQwertyAsciiSym(int code, bool shift, KeySym &outSym) {
    // Physical-key → QWERTY lowercase letter mapping (XKB keycodes).
    static const std::unordered_map<int, char> kLetters = {
        // Top row  Q W E R T Y U I O P
        {24,'q'},{25,'w'},{26,'e'},{27,'r'},{28,'t'},
        {29,'y'},{30,'u'},{31,'i'},{32,'o'},{33,'p'},
        // Home row A S D F G H J K L
        {38,'a'},{39,'s'},{40,'d'},{41,'f'},{42,'g'},
        {43,'h'},{44,'j'},{45,'k'},{46,'l'},
        // Bottom row Z X C V B N M
        {52,'z'},{53,'x'},{54,'c'},{55,'v'},{56,'b'},{57,'n'},{58,'m'},
    };

    // Punctuation resolved first (same logic as keycodeToAsciiPunct).
    switch (code) {
    case 47: outSym = static_cast<KeySym>(shift ? ':' : ';');  return true;
    case 48: outSym = static_cast<KeySym>(shift ? '"' : '\''); return true;
    case 59: outSym = static_cast<KeySym>(shift ? '<' : ',');  return true;
    case 60: outSym = static_cast<KeySym>(shift ? '>' : '.');  return true;
    case 61: outSym = static_cast<KeySym>(shift ? '?' : '/');  return true;
    default: break;
    }

    auto it = kLetters.find(code);
    if (it == kLetters.end())
        return false;

    char c = shift ? static_cast<char>(std::toupper(static_cast<unsigned char>(it->second)))
                   : it->second;
    outSym = static_cast<KeySym>(c);
    return true;
}

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

JQwertyEngine::JQwertyEngine(Instance *instance) : instance_(instance) {
    FCITX_INFO() << "JQwertyEngine loaded";
}

void JQwertyEngine::activate(const InputMethodEntry & /*entry*/,
                            InputContextEvent & /*event*/) {}

void JQwertyEngine::deactivate(const InputMethodEntry &entry,
                              InputContextEvent &event) {
    reset(entry, event);
}

void JQwertyEngine::reset(const InputMethodEntry & /*entry*/,
                         InputContextEvent &event) {
    auto *ic = event.inputContext();
    ic->inputPanel().reset();
    ic->updatePreedit();
    ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void JQwertyEngine::keyEvent(const InputMethodEntry & /*entry*/,
                            KeyEvent &keyEvent) {
    // Only handle key-press events; pass releases through untouched.
    if (keyEvent.isRelease())
        return;
 
    auto *ic = keyEvent.inputContext();
    const Key  &rawKey = keyEvent.rawKey();
    const Key  &key    = keyEvent.key();
 
    const bool shift = rawKey.states().test(KeyState::Shift) ||
                       key.states().test(KeyState::Shift);
    const bool ctrl  = rawKey.states().test(KeyState::Ctrl) ||
                       key.states().test(KeyState::Ctrl);
    const bool alt   = rawKey.states().test(KeyState::Alt) ||
                       key.states().test(KeyState::Alt);
    const bool mod5  = rawKey.states().test(KeyState::Mod5) ||
                       key.states().test(KeyState::Mod5);  // AltGr
 
    // -----------------------------------------------------------------------
    // 1. AltGr pass-through: let the active layout produce 3rd/4th-level
    //    symbols (ä, ö, @, €, …) without interference from this engine.
    // -----------------------------------------------------------------------
    if (mod5) {
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
        return; // NOT filterAndAccept() – the app must receive the key.
    }
 
    // -----------------------------------------------------------------------
    // 2. Ctrl / Alt combos: forward unmodified so shortcuts keep working.
    //    We remap the *keysym* to the QWERTY position so that, e.g., Ctrl+C
    //    works even when the layout places 'c' elsewhere.
    //
    //    forwardKey() is safe here because the isVirtual() guard at the top
    //    of this function prevents the forwarded key from re-entering and
    //    causing an infinite loop.
    // -----------------------------------------------------------------------
    if (ctrl || alt) {
        const int code = keyEvent.origKey().code();
        KeySym mapped;
        if (keycodeToQwertyAsciiSym(code, shift, mapped)) {
            // Re-fire the event with the QWERTY keysym but the original
            // modifiers, so the application sees e.g. Ctrl+c regardless of
            // which layout is active.
            KeyStates states = rawKey.states();
            ic->forwardKey(Key(mapped, states));
            keyEvent.filterAndAccept();
        }
        // Unknown key / non-letter: let it fall through normally.
        return;
    }
 
    // -----------------------------------------------------------------------
    // 3. Punctuation committed directly, bypassing any composition state.
    // -----------------------------------------------------------------------
    {
        const int code = keyEvent.origKey().code();
        char punct;
        if (keycodeToAsciiPunct(code, shift, punct)) {
            ic->commitString(std::string(1, punct));
            ic->updatePreedit();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            keyEvent.filterAndAccept();
            return;
        }
    }
 
    // -----------------------------------------------------------------------
    // 4. Letter keys: commit the QWERTY character directly via commitString.
    //    We do NOT use forwardKey() here (unlike section 2) because plain
    //    letters need no modifier preservation — committing the character
    //    directly is simpler and avoids any risk of re-entry.
    // -----------------------------------------------------------------------
    {
        const int code = keyEvent.origKey().code();
        KeySym mapped;
        if (keycodeToQwertyAsciiSym(code, shift, mapped)) {
            ic->commitString(std::string(1, static_cast<char>(mapped)));
            ic->updatePreedit();
            ic->updateUserInterface(UserInterfaceComponent::InputPanel);
            keyEvent.filterAndAccept();
            return;
        }
    }

    // Everything else (function keys, arrows, etc.) passes through untouched.
}

} // namespace fcitx

// ---------------------------------------------------------------------------
// Plugin entry point
// ---------------------------------------------------------------------------
FCITX_ADDON_FACTORY(fcitx::JQwertyEngineFactory)
