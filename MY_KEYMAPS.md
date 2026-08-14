# Simon's Custom QMK Keymaps & Firmware Documentation

This document summarizes custom keyboards, keymaps, shortcuts, leader keys, indicators, and helper functionalities implemented in this repository.

---

## ⌨️ Modified Keyboards & Keymaps

| Keyboard | Keymap Directory | Keymap File | Key Features |
| :--- | :--- | :--- | :--- |
| **Keychron Q1 v2 (ANSI Encoder)** | [`keyboards/keychron/q1v2/ansi_encoder/keymaps/mine/`](keyboards/keychron/q1v2/ansi_encoder/keymaps/mine/) | [`keymap.c`](keyboards/keychron/q1v2/ansi_encoder/keymaps/mine/keymap.c) | 6 Layers (Mac/Win/Hardware), Dynamic Macros, Leader Key, Encoder Map |
| **Keychron Q5 HE (ANSI Encoder)** | [`keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/`](keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/) | [`keymap.c`](keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/keymap.c) | 8 Layers (Mac/Win/Gaming 1&2/Hardware), SignalRGB/OpenRGB, Hall Effect Profiles |
| **Shared Userspace** | [`users/simon/`](users/simon/) | [`simon.c`](users/simon/simon.c) / [`simon.h`](users/simon/simon.h) | Shared indicator system, inactivity dimming, leader sequences, external RGB sync |

---

## ⚡ Leader Key System

The Leader key allows executing sequences with a single prefix tap.

- **Leader Activators:**
  - **Left Alt Tap**: `LALT_T(KC_NO)` — Acts as standard `Left Alt` when held with another key, but triggers the **Leader Key** when tapped and released.
  - **Left GUI Tap**: `GUI_T(KC_NO)` (on Mac layout) — Acts as `GUI` when held, triggers **Leader Key** when tapped.
  - **Explicit Leader Key**: `QK_LEAD` is also assigned to the **`,` (comma)** position on the `MAC_FN` and `WIN_FN` layers.
- **Timing Configuration**:
  - `LEADER_PER_KEY_TIMING` enabled.
  - `LEADER_TIMEOUT`: `250ms` timeout per keypress.
- **LED Indicator**:
  - `INDICATOR_LEADER` lights up dynamically in **White** on the key you pressed to activate Leader mode and turns off once the sequence ends.

### Leader Key Sequences

| Sequence | Output / Action | Description |
| :--- | :--- | :--- |
| `Leader` ➔ <kbd>E</kbd> | `szymek.fogiel@gmail.com` | Personal email |
| `Leader` ➔ <kbd>E</kbd> ➔ <kbd>W</kbd> | `szymonf@google.com` | Work email |
| `Leader` ➔ <kbd>T</kbd> | `+41778150320` | Primary phone number (CH) |
| `Leader` ➔ <kbd>T</kbd> ➔ <kbd>P</kbd> | `+48732258424` | Secondary phone number (PL) |

---

## 🎬 Dynamic Macros

Record and replay keystroke macros on the fly without reflashing.

| Keycode | Macro Action | Location in `MAC_FN` / `WIN_FN` |
| :--- | :--- | :--- |
| `MR1` (`QK_DYNAMIC_MACRO_RECORD_START_1`) | Start recording Macro 1 | <kbd>A</kbd> |
| `MR2` (`QK_DYNAMIC_MACRO_RECORD_START_2`) | Start recording Macro 2 | <kbd>S</kbd> |
| `MS` (`QK_DYNAMIC_MACRO_RECORD_STOP`) | Stop recording macro | <kbd>W</kbd> |
| `MP1` (`QK_DYNAMIC_MACRO_PLAY_1`) | Playback Macro 1 | <kbd>1</kbd> |
| `MP2` (`QK_DYNAMIC_MACRO_PLAY_2`) | Playback Macro 2 | <kbd>2</kbd> |

- **Macro Recording Indicator**:
  - `INDICATOR_MACRO_REC` turns **Red** on the activator key while recording is in progress.
- **Gaming Layer Macro Controls (Q5 HE)**:
  - `GAMING` layer has `MP1` and `MP2` mapped to Numpad <kbd>1</kbd> and <kbd>2</kbd>.
  - `GAMING2` layer (accessed via Numpad <kbd>0</kbd> hold) has `MR1` and `MR2` mapped to Numpad <kbd>1</kbd> and <kbd>2</kbd>.

---

## 🔤 Custom Shortcuts & Word Navigation

| Custom Code / Combo | Shortcut Action | Purpose |
| :--- | :--- | :--- |
| `M_NW` | `Ctrl + Right` ➔ `Ctrl + Right` ➔ `Ctrl + Left` | **Next Word Jump (Windows)**: Jumps to the beginning of the next word. Mapped to <kbd>E</kbd> on `WIN_FN`. |
| `M_NM` | `Alt + Right` ➔ `Alt + Right` ➔ `Alt + Left` | **Next Word Jump (macOS)**: Jumps to the beginning of the next word. Mapped to <kbd>E</kbd> on `MAC_FN`. |
| `CW_TOGG` | Caps Word Toggle | Toggles Caps Word (mapped to <kbd>Tab</kbd> position in FN layers). |
| `TG_GMG` | Toggle Gaming Mode / Profile | Cycle between Normal ➔ Gaming Profile 1 (Red LED) ➔ Gaming Profile 2 (Green LED) ➔ Normal (Q5 HE only). |
| `IND_BR_U` | Increase Indicator Brightness | Increases indicator LED brightness in EEPROM. |
| `IND_BR_D` | Decrease Indicator Brightness | Decreases indicator LED brightness in EEPROM. |

---

## 🎛️ Dual-Role Keys & Tap-Hold Behavior

- **Spacebar Layer-Tap**:
  - `LT(MAC_FN, KC_SPC)` / `LT(WIN_FN, KC_SPC)`: Space when tapped, momentary FN layer when held.
  - Custom Tapping Term: **175ms** for ultra-responsive typing without accidental layer holds.
- **Caps Lock Modifier-Tap**:
  - Mac Base: `GUI_T(KC_ESC)` — Escape on tap, Command (`GUI`) on hold. Permissive hold enabled.
  - Windows Base: `LCTL_T(KC_ESC)` — Escape on tap, `Control` on hold. Permissive hold enabled.
  - Windows Base (Q5 HE top-left): `LCSG_T(KC_ESC)` — Escape on tap, `Ctrl+Shift+Gui` on hold.
- **HJKL Arrow Navigation**:
  - On `MAC_FN` and `WIN_FN`, <kbd>H</kbd>, <kbd>J</kbd>, <kbd>K</kbd>, <kbd>L</kbd> are mapped to <kbd>Left</kbd>, <kbd>Down</kbd>, <kbd>Up</kbd>, <kbd>Right</kbd> (Vim-style navigation).
- **Backspace & Delete**:
  - In FN layers, <kbd>D</kbd> is `KC_DEL` and <kbd>F</kbd> is `KC_BSPC`.

---

## 🎚️ Rotary Encoder Functions

| Layer | Counter-Clockwise (CCW) | Clockwise (CW) | Description |
| :--- | :--- | :--- | :--- |
| **`MAC_BASE`** | `KC_VOLD` | `KC_VOLU` | Volume Down / Up |
| **`MAC_FN`** | `MS_WHLU` | `MS_WHLR` (Q1) / `MS_WHLD` (Q5) | Mouse Wheel Scrolling |
| **`WIN_BASE`** | `KC_VOLD` | `KC_VOLU` | Volume Down / Up |
| **`WIN_FN`** | `MS_WHLU` | `MS_WHLD` | Mouse Wheel Scrolling |
| **`GAMING`** | `KC_VOLD` | `KC_VOLU` | Volume Down / Up (Q5 HE) |
| **`HARDWARE`** | `UG_VALD` | `UG_VALU` | RGB Matrix Brightness Down / Up |
| **`HARDWARE2`** | `IND_BR_D` | `IND_BR_U` | Indicator LED Brightness Down / Up |

---

## 💡 Lighting, Indicators & Inactivity Dimming

- **Inactivity Timeout (5 min)**:
  - After 5 minutes (300,000 ms) of idle time, RGB automatically dims to minimum (`val=1`) to preserve LEDs.
  - Wakes up immediately upon any key press or incoming USB/HID activity.
- **FN Layer Key Masking**:
  - Activating `MAC_FN`, `WIN_FN`, or `HARDWARE` automatically highlights active keys in solid white and blacks out unmapped keys for clear visual feedback.
- **External RGB (SignalRGB & OpenRGB)**:
  - `UG_SRGB` toggles external RGB synchronization.
  - External RGB indicator (LED index 36 on Q5 HE):
    - **White**: External RGB active and streaming.
    - **Green**: External RGB idle / timed out.
    - **Red**: External RGB disabled.
  - When SignalRGB is active, QMK RGB adjustment keys intercept and forward standard SignalRGB shortcuts (`Ctrl+Alt+Shift+Gui` + `+`/`-`/`Q`/`A`/`Z`).

---

## 🛠️ Build Commands Quick Reference

To compile the firmware for your keyboards:

```bash
# Build Keychron Q1 v2 ANSI Encoder
qmk compile -kb keychron/q1v2/ansi_encoder -km mine

# Build Keychron Q5 HE ANSI Encoder
qmk compile -kb keychron/q5_he/ansi_encoder -km mine
```

To flash directly (with bootloader):
```bash
qmk flash -kb keychron/q1v2/ansi_encoder -km mine
qmk flash -kb keychron/q5_he/ansi_encoder -km mine
```
