# Simon's Custom QMK Keymaps & Firmware Documentation

This document summarizes custom keyboards, keymaps, shortcuts, leader keys, indicators, and helper functionalities implemented in this repository.

---

## ⌨️ Modified Keyboards & Keymaps

| Keyboard | Keymap Directory | Keymap File | Key Features |
| :--- | :--- | :--- | :--- |
| **Keychron Q1 v2 (ANSI Encoder)** | [`keyboards/keychron/q1v2/ansi_encoder/keymaps/mine/`](keyboards/keychron/q1v2/ansi_encoder/keymaps/mine/) | [`keymap.c`](keyboards/keychron/q1v2/ansi_encoder/keymaps/mine/keymap.c) | 6 Layers (Mac/Win/Hardware), Dynamic Macros, Leader Key, Encoder Map |
| **Keychron Q5 HE (ANSI Encoder)** | [`keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/`](keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/) | [`keymap.c`](keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/keymap.c) ([HE Profiles README](keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/README.md)) | 8 Layers (Mac/Win/Gaming 1&2/Hardware), SignalRGB/OpenRGB, Hardcoded Hall Effect Profiles |
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
  - `INDICATOR_LEADER` lights up dynamically in **Peach** on the key you pressed to activate Leader mode and turns off once the sequence ends.

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
  - `INDICATOR_MACRO_REC` turns **Red** (Catppuccin) on the activator key while recording is in progress.
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
| `TG_GMG` | Toggle Gaming Mode / Profile | Cycle between Normal ➔ Gaming Profile 1 (Maroon LED) ➔ Gaming Profile 2 (Green LED) ➔ Normal (Q5 HE only). |
| `IND_BR_U` | Increase Indicator Brightness | Increases indicator LED brightness in EEPROM. |
| `IND_BR_D` | Decrease Indicator Brightness | Decreases indicator LED brightness in EEPROM. |
| `IND_MODE` | Toggle Indicator Mode | Switches indicators between **auto-hide** (default) and **always on**; saved in EEPROM. Mapped to <kbd>I</kbd> on `HARDWARE`. |

---

## 🎛️ Dual-Role Keys & Tap-Hold Behavior

- **Spacebar Layer-Tap**:
  - `LT(MAC_FN, KC_SPC)` / `LT(WIN_FN, KC_SPC)`: Space when tapped, momentary FN layer when held.
  - Custom Tapping Term: **175ms** for ultra-responsive typing without accidental layer holds.
- **Caps Lock Modifier-Tap**:
  - Mac Base: `GUI_T(KC_ESC)` — Escape on tap, Command (`GUI`) on hold. Permissive hold enabled.
  - Windows Base: `LCTL_T(KC_ESC)` — Escape on tap, `Control` on hold. Permissive hold enabled.
  - Q5 HE: plain `KC_ESC` top-left; `KC_HYPR` (`Ctrl+Shift+Alt+Gui`) on the key left of the Hardware key (Mac and Windows base layers).
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
  - Activating `MAC_FN`, `WIN_FN`, or `HARDWARE` automatically highlights active keys in the `fn-layer` indicator colour (white by default; OpenRGB profiles set teal via `[keyboard.indicators]`; `kbd-ctl color fn-layer 0 '#rrggbb' --save`) and dims unmapped keys to the layer dim (25% by default; `kbd-ctl set layer-dim 0.25 --save`, OpenRGB profiles' `[keyboard] dim`) of their regular colour (black in effects whose colour can't be computed), the same ratio as the OpenRGB submap highlight.
- **Indicator Colors**: Catppuccin Macchiato hues with boosted saturation (pastels wash out on LEDs), defined once as `IND_COLOR_*` in `users/simon/simon.h`. They are the defaults of a per-indicator, per-state colour table; on the Q5 HE the table is host-editable over the host protocol (below) and stored in the user EEPROM datablock.
- **Indicators & Auto-Hide**:
  - Caps Lock (on the Caps Lock key) and Num Lock (Q5 HE only), both **Peach**, indicators replace Keychron's built-in ones.
  - In auto-hide mode (default), status indicators (Caps Lock, Num Lock, Gaming) show for **2 s** and then fade into the key's regular color over **500 ms**. They come back on startup/wake, on any layer change (holding Space/Fn/Hardware key), and whenever an indicator turns on, off or changes color. While a layer key is held they stay visible; the 2 s window starts on release.
  - The fade blends into the regular color in Solid Color and OpenRGB/SignalRGB direct modes; in other effects the indicator just switches off.
  - Leader and Macro Recording indicators are never hidden while active.
  - `IND_MODE` (<kbd>Hardware</kbd> + <kbd>I</kbd>) toggles between auto-hide and always on.
- **Caps Word**: the Tab key lights peach while Caps Word is on (`caps-word` indicator, auto-hides like caps lock).
- **Workspace hints (Linux layer, with `kbd-daemon`)**:
  - Hold **SUPER** for 200 ms: number keys of Hyprland workspaces 1–10 with windows light (`workspace` indicator: occupied white, active peach); **SUPER + ALT** shows 11–20. Every other key dims to the layer dim (25% by default).
  - A workspace with a window demanding attention breathes its number key in the `urgent` colour (red), even while dimmed or with the lights off; a new one wakes a dimmed keyboard.
  - The data comes from `kbd-daemon` and is dropped when USB drops (KVM switch, replug) or the daemon stops.
- **External RGB (SignalRGB & OpenRGB)**:
  - External control is automatic: OpenRGB/SignalRGB take over the LEDs when they send data; SignalRGB falls back to the local effect after 500 ms of silence.
  - External RGB indicator (Q5 HE, on the Hardware key): lights **Lavender** while the Hardware layer is held and a host is driving the LEDs (direct mode).
  - When SignalRGB is active, QMK RGB adjustment keys intercept and forward standard SignalRGB shortcuts (`Ctrl+Alt+Shift+Gui` + `+`/`-`/`Q`/`A`/`Z`).

---

## 🎮 Hall Effect (HE) Profiles & Switching (Q5 HE)

See full details in **[`keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/README.md`](keyboards/keychron/q5_he/ansi_encoder/keymaps/mine/README.md)**.

### Profile Switching Shortcuts
1. **`TG_GMG` Key (F14 position next to Delete on `WIN_BASE`):**
   - **Press 1**: Gaming Profile 1 (Rapid Trigger WASD, 1.0mm actuation, Maroon LED).
   - **Press 2**: Gamepad Profile 2 (Xbox controller / analog axis bindings, Green LED).
   - **Press 3**: Turn off Gaming mode & restore Profile 0 (Default typing).
2. **Keychron Hardware Combo (`Fn + P` held, then press):**
   - <kbd>X</kbd> ➔ Profile 0 (Default / Typing)
   - <kbd>C</kbd> ➔ Profile 1 (Gaming / Rapid Trigger)
   - <kbd>V</kbd> ➔ Profile 2 (Gamepad / Controller)

### Hardcoded Profiles Summary
* **Profile 0 (Default)**: Normal $2.0\text{ mm}$ actuation typing mode.
* **Profile 1 (Gaming)**: $1.0\text{ mm}$ actuation globally, **Rapid Trigger** on WASD ($1.0\text{ mm}$ actuation, $0.4\text{ mm}$ press/release sensitivity).
* **Profile 2 (Gamepad)**: $1.0\text{ mm}$ actuation globally with full Xbox controller stick / button analog matrix mappings.

---

## 🔌 Host Protocol (Q5 HE, `kbd-daemon`)

[`users/simon/host_protocol.c`](users/simon/host_protocol.c) / [`host_protocol.h`](users/simon/host_protocol.h) — raw HID commands `0xC0–0xCF` for a host service (`kbd-daemon`): gaming mode, settings and indicator colours. Built wherever `OPENRGB_ENABLE = yes` (defines `HOST_PROTOCOL_ENABLE`). The packet layout is documented in the header.

| ID | Command | Reply |
| :--- | :--- | :--- |
| `0xC0` | `GET_INFO` — protocol version, feature bits, indicator count, build date | yes |
| `0xC1` | `GET_STATE` — gaming state, active HE profile, indicator brightness, indicators always on, blackout, base layer (Mac / Linux) | yes |
| `0xC2` | `SET_GAMING` — 0 off, 1 rapid trigger, 2 gamepad (same path as `TG_GMG`) | no |
| `0xC3` | `GET_INDICATORS` — paged from a first index: per indicator wire id, state count, LED index, colours | yes |
| `0xC4` | `SET_INDICATOR_COLOR` — wire id, state, RGB, persist | no |
| `0xC5` | `SET_SETTING` — 1 indicator brightness, 2 indicators always on (`IND_MODE`), 3 blackout, 4 layer dim; persist | no |
| `0xC6` | `RESET_INDICATOR_COLORS` — back to the `IND_COLOR_*` defaults; persist | no |
| `0xC7` | `RELOAD_INDICATOR_COLORS` — back to the saved colours and layer dim (drops unsaved overrides) | no |
| `0xC8` | `SET_WORKSPACES` — occupied / urgent / active 20-bit masks (workspaces 1–20) | no |
| `0xC9` | `CLEAR_WORKSPACES` | no |
| `0xCF` | `NOTIFY_STATE` (keyboard → host) — state changed on the keyboard itself | — |

- **Sharing the interface with OpenRGB**: Linux delivers every input report to every process with the hidraw node open, and OpenRGB takes the first report it reads as its answer. So writes never reply, and for 1 s after OpenRGB's protocol-version query (`0x01`, which opens every detection) host replies are dropped and notifications held.
- **Notifications** only cover changes made on the keyboard (`TG_GMG`, `Fn + P`, `IND_MODE`, indicator brightness keys), coalesced to one per 100 ms; changes a host command made are not echoed.
- **Indicator wire ids** (stable across keyboards): `1` macro recording, `2` leader, `3` external RGB, `4` gaming (rapid / gamepad), `5` caps lock, `6` num lock, `7` fn layer (colour of the keys bound on a held FN layer), `8` workspace (occupied / active), `9` urgent, `10` caps word. 7–9 have no LED of their own.
- **Unsaved overrides** (OpenRGB profiles' `[keyboard]` colours and dim, via `kbd-daemon`) are dropped when USB drops to unconfigured (KVM switch to another computer): the keyboard shows its saved colours there. `kbd-daemon` re-applies them when the keyboard comes back. A host colour edit wakes auto-hidden indicators so the change is visible.
- **EEPROM**: host-protocol keyboards keep `user_config` and the colour table in a 64-byte user datablock (`EECONFIG_USER_DATA_SIZE`). The first boot after switching to it starts from defaults once. Not for VIA keyboards (Q1 v2): the datablock would shift the dynamic keymap.

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
