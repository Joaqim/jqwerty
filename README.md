# fcitx5-jqwerty

A minimal fcitx5 input method engine that forces **QWERTY key positions** for
letters and punctuation regardless of the active XKB layout.

This is useful when you type in a non-standard Latin layout (e.g. Dvorak,
Colemak, or a locale-specific layout like Finnish/Swedish) but still want:

- `Ctrl+C`, `Ctrl+V`, `Ctrl+Z` etc. to work by **physical position**, not by
  the character the key currently produces.
- Punctuation (`. , ; : ' "`) committed as US-ASCII regardless of layout.
- `AltGr` combinations (ä, ö, @, €, …) to pass through untouched so your
  layout's 3rd/4th-level symbols still work.

---

## Requirements

| Dependency | Minimum version | Package (Debian/Ubuntu) |
|---|---|---|
| fcitx5-core | 5.0 | `libfcitx5-core-dev` |
| fcitx5-utils | 5.0 | `libfcitx5utils-dev` |
| extra-cmake-modules | 5.80 | `extra-cmake-modules` |
| CMake | 3.16 | `cmake` |
| C++17 compiler | — | `g++` / `clang++` |

```
# Debian / Ubuntu
sudo apt install libfcitx5-core-dev libfcitx5utils-dev extra-cmake-modules \
                 cmake g++
```

---

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### Install system-wide

```bash
sudo cmake --install build
```

### Install to a local prefix (no sudo)

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build -j$(nproc)
cmake --install build
```

With `~/.local` as prefix the files land at:

```
~/.local/lib/fcitx5/fcitx5-jqwerty.so
~/.local/share/fcitx5/addon/jqwerty.conf
~/.local/share/fcitx5/inputmethod/jqwerty.conf
```

Make sure `~/.local/lib` is on your library path if you go this route, or
set `FCITX_ADDON_DIRS` accordingly.

---

## Activate

1. Restart fcitx5 (or `fcitx5-remote -r`).
2. Open **Fcitx5 Configuration** → **Input Method**.
3. Add **JQwerty (QWERTY shim)** to your input method list.
4. Move it to the top (or switch to it as your active IM).

---

## How it works

The engine intercepts every key-press event and classifies it into one of four
paths before letting the normal fcitx5 pipeline continue:

```
Key press
   │
   ├─ AltGr held? ──────────────────────────────► pass through (layout 3rd/4th level)
   │
   ├─ Ctrl / Alt held? ──► remap keysym to QWERTY ► forward (shortcuts work by position)
   │
   ├─ Punctuation keycode? ─► commit ASCII char ──► filterAndAccept
   │
   └─ Letter keycode? ──────► remap keysym to QWERTY ► forward
```

The keycode→QWERTY tables in `src/jqwerty.cpp` use **Linux evdev+8 XKB
keycodes** for a standard ANSI physical keyboard. If your hardware has a
different physical layout (e.g. ISO with an extra key at keycode 94), add
entries to the tables as needed.

---

## Extending the punctuation table

Edit the `keycodeToAsciiPunct` and `keycodeToQwertyAsciiSym` functions in
`src/jqwerty.cpp`. Each `case` is a physical XKB keycode. To find the keycode
for an arbitrary key, run:

```bash
xev | grep keycode
```

and press the key you want to map.

---

## Uninstall

```bash
sudo cmake --build build --target uninstall
```

(Only works if you installed system-wide with the default prefix.)
