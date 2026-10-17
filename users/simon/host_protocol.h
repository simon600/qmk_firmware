#pragma once

#include "quantum.h"

// --- Host protocol (raw HID, 0xC0–0xCF) ---
//
// Lets a host service (kbd-daemon) drive keyboard state that isn't lighting:
// gaming mode, settings and indicator colours. It shares the raw HID interface
// with OpenRGB (0x01–0x09), SignalRGB (0x21–0x28) and Keychron (0xA0–0xAB).
//
// With RGB_RAW_ENABLE (protocol 8, HOST_FEATURE_OWN_INTERFACE) it and the
// OpenRGB packets move to a second raw HID interface of their own: 64-byte
// reports, usage page 0xFF61 / usage 0x61, USB only. The first interface
// (usage page 0xFF60, 32 bytes) keeps VIA, Keychron and SignalRGB and no
// longer answers 0x01–0x09 or 0xC0–0xCF. What follows about other programs
// reading the same reports applies to the shared interface only.
//
// Over 2.4 GHz (protocol 9, HOST_FEATURE_WIRELESS) the host protocol, and only
// it, also runs on the Keychron Link dongle's raw HID interface (usage page
// 0xFF60, 32-byte reports, shared with VIA; OpenRGB's ids collide with VIA's
// there). Every packet fits in 32 bytes; one request at a time (the link drops
// reports sent back to back). Bluetooth is left alone.
// The keyboard there says nothing until a host has spoken, or once (a single
// heartbeat, then 6 s for an answer) when the link comes up: power on, back to
// 2.4 GHz, reconnect. Then it runs the link:
//   - while in use it sends a heartbeat (PING [1] 0) 2 s after the host's last
//     packet or its own last heartbeat, right away
//     when it's used again or the link comes back. The host answers each with
//     any command (PING will do) and sends only between a heartbeat and the
//     idle notice
//   - when the backlight times out (no keys), PING [1] 1, idle, once, and again
//     after a host that speaks while it's idle is done (250 ms of quiet: reports
//     sent back to back get lost). The host then keeps
//     quiet until the next heartbeat: any packet would wake a sleeping keyboard
//     and keep it awake for another 30 minutes
//   - 6 s after a heartbeat without an answer it takes the host to be gone,
//     and stays quiet until one speaks again
//   - the heartbeat's [2] is 1 while the host's lighting (0xCA, protocol 10)
//     is on the keys: 0 after a power cycle, a handback or a local effect
//     change, so the host sends it again
//   - its [3] [4] are the battery (protocol 11, as GET_STATE [9] [10])
// NOTIFY_STATE goes to such a host too, and NOTIFY_RGB_KEY while its lighting
// is loaded. NOTIFY_MODS and NOTIFY_HANDBACK stay on USB: the keyboard draws
// the modifier highlight itself (overlay.h).
//
// Every reply starts with the command id it answers. Writes never reply, so
// they can't land in another host program's read queue. Linux hands every
// input report to every process that has the hidraw node open, and OpenRGB
// takes the first report it reads as its answer without checking it; so the
// keyboard says nothing unprompted until a host speaks to it: notifications
// only go out while a host is listening (any command, e.g. PING, in the last
// 6 s, over USB). A machine with only OpenRGB never sees a host-protocol
// report. One that runs a host program must keep the OpenRGB server away from
// the keyboard (kbd-daemon drives its lighting itself over 0x01–0x09).
//
// Packet layout (byte 0 = command id):
//   0xC0 GET_INFO       -> [1] protocol version, [2] feature bits (HOST_FEATURE_*), [3] indicator count,
//                          [4] max states per indicator, [5..] build date (NUL-terminated)
//   0xC1 GET_STATE      -> [1..] state block (see below)
//   0xC2 SET_GAMING     [1] 0 off, 1 rapid trigger, 2 gamepad
//   0xC3 GET_INDICATORS [1] first indicator -> [1] total count, [2] first,
//                          [3] entries in this reply, then per indicator: wire id,
//                          state count, LED index (255 = none), state count × (r, g, b).
//                          Ask again from first + entries until all are read.
//   0xC4 SET_INDICATOR_COLOR [1] wire id, [2] state, [3..5] r g b, [6] persist
//   0xC5 SET_SETTING    [1] setting, [2] value, [3] persist
//                       (1 indicator brightness, 2 indicators always on, 3 blackout,
//                       4 layer dim: other keys while an FN layer shows,
//                       5 wireless backlight timeout in 10 s units (protocol 12;
//                       always saved, Keychron's own setting: the Launcher's too),
//                       6 the keyboard's own brightness for the link typing goes
//                       over (protocol 12; always saved; Fn + the brightness keys
//                       change it on the keyboard))
//   0xC6 RESET_INDICATOR_COLORS [1] persist   (back to the firmware defaults)
//   0xC7 RELOAD_INDICATOR_COLORS               (back to the saved colours and layer
//                       dim: drops unsaved host overrides, e.g. an OpenRGB profile's)
//   0xC8, 0xC9          retired (protocol 3-4 workspace hints; openrgb-daemon
//                       draws them now from NOTIFY_MODS)
//   0xCA LIGHTING       [1] op, ...: the host's lighting over 2.4 GHz, drawn by
//                       the keyboard (overlay.h). No replies; ignored on USB
//   0xCC PING           heartbeat; any command counts. While the host was heard
//                       from in the last 6 s (USB), the RGB keys in direct mode
//                       go to it as NOTIFY_RGB_KEY instead of acting locally.
//                       Keyboard -> host over 2.4 GHz: [1] 0 in use, 1 idle,
//                       [2] 1 the host's lighting is loaded (above)
//   0xCB NOTIFY_HANDBACK (keyboard -> host) no arguments. The keyboard shows its
//                       own saved effect: it just started (a reboot the host
//                       can't see, e.g. USB power cut in S3 and the device
//                       kept across resume), or USB came back (wake, KVM switch)
//                       and no host listened within 15 s. Sent once, when a host
//                       next speaks, so it can set its lighting again
//   0xCD NOTIFY_RGB_KEY (keyboard -> host) [1] 1 next profile, 2 previous
//                       profile, 3 brightness up, 4 brightness down, 5 lights
//                       on/off. Sent instead
//                       of acting on the key while a host streams direct mode
//                       (USB) or its lighting is loaded (2.4 GHz). The brightness
//                       keys where the keymap says so (brightness_keys_are_
//                       keyboard_user: Fn + them) stay on the keyboard: its own
//                       brightness, which direct mode is drawn at
//   0xCE NOTIFY_MODS    (keyboard -> host) [1] held modifiers as an X11 /
//                       Hyprland modmask (1 shift, 4 ctrl, 8 alt, 64 super; left
//                       and right alike). Sent once they've been held for 200 ms,
//                       right away when they change while reported, 0 on release.
//                       While a host listens (USB)
//   0xCF NOTIFY_STATE   (keyboard -> host) [1..] state block, sent when state
//                       changes on the keyboard itself (key press, Fn+P combo),
//                       never for changes a host command made
//
// Indicator wire ids: 1 macro recording, 2 leader, 3 external RGB (Hardware
// key), 4 gaming (states: rapid trigger, gamepad), 5 caps lock, 6 num lock,
// 7 fn layer (colour of the keys bound on a held FN layer; no LED of its own),
// 10 caps word (Tab key). 8 and 9 (workspace, urgent) are retired, not reused
//
// State block:
//   [1] gaming state (0 off, 1 rapid, 2 gamepad, 255 unsupported)
//   [2] active Hall-effect profile (255 unsupported)
//   [3] indicator brightness  [4] indicators always on (0 = auto-hide)
//   [5] background blackout
//   [6] base layer (0 unknown, 1 Mac, 2 Linux/Windows)  [7] layer dim (0-255)
//   GET_STATE only (protocol 10): [8] what typing goes over (HOST_TRANSPORT_*).
//   The mode switch moves it with the cable still in, and USB stays enumerated:
//   a host on USB learns it's on 2.4 GHz from this
//   (protocol 11) [9] battery percent (255 unknown: no battery, or not measured
//   yet; the module measures it only while typing goes over the air, so on USB
//   it's the last reading), [10] power (HOST_POWER_*)
//   (protocol 12) [11] wireless backlight timeout in 10 s units (0: none),
//   [12] the keyboard's own brightness for this link (setting 6).
//   NOTIFY_STATE carries [8..12] too, and goes out when [12] changes

#define HOST_PROTOCOL_VERSION 12

enum host_command_id {
    HOST_GET_INFO               = 0xC0,
    HOST_GET_STATE              = 0xC1,
    HOST_SET_GAMING             = 0xC2,
    HOST_GET_INDICATORS         = 0xC3,
    HOST_SET_INDICATOR_COLOR    = 0xC4,
    HOST_SET_SETTING            = 0xC5,
    HOST_RESET_INDICATOR_COLORS = 0xC6,
    HOST_RELOAD_INDICATOR_COLORS = 0xC7,
    HOST_LIGHTING               = 0xCA,
    HOST_NOTIFY_HANDBACK        = 0xCB,
    HOST_PING                   = 0xCC,
    HOST_NOTIFY_RGB_KEY         = 0xCD,
    HOST_NOTIFY_MODS            = 0xCE,
    HOST_NOTIFY_STATE           = 0xCF,
};

enum host_feature_bits {
    HOST_FEATURE_GAMING             = 1 << 0,
    HOST_FEATURE_HE_PROFILES        = 1 << 1,
    HOST_FEATURE_PERSISTENT_COLORS  = 1 << 2,
    HOST_FEATURE_MODS_NOTIFY        = 1 << 3,
    HOST_FEATURE_RGB_KEYS           = 1 << 4,
    // Host protocol and OpenRGB packets on their own raw HID interface
    HOST_FEATURE_OWN_INTERFACE      = 1 << 5,
    // The host protocol over the 2.4 GHz dongle (protocol 9)
    HOST_FEATURE_WIRELESS           = 1 << 6,
    // Lighting drawn by the keyboard over 2.4 GHz (0xCA, protocol 10)
    HOST_FEATURE_LIGHTING           = 1 << 7,
};

// Where a host command came from, and its reply goes
enum host_link {
    HOST_LINK_USB,
    HOST_LINK_WIRELESS,
};

// PING from the keyboard over 2.4 GHz, [1]
enum host_heartbeat {
    HOST_HEARTBEAT_ACTIVE = 0,
    HOST_HEARTBEAT_IDLE   = 1,
};

enum host_setting_id {
    HOST_SETTING_INDICATOR_BRIGHTNESS = 1,
    HOST_SETTING_INDICATORS_ALWAYS_ON = 2,
    HOST_SETTING_BLACKOUT             = 3,
    HOST_SETTING_LAYER_DIM            = 4,
    HOST_SETTING_WIRELESS_BACKLIGHT   = 5,
    HOST_SETTING_KEYBOARD_BRIGHTNESS  = 6,
};

#define HOST_STATE_UNSUPPORTED 255

// GET_STATE [10], heartbeat [4]
enum host_power {
    HOST_POWER_BATTERY  = 0,
    HOST_POWER_CHARGING = 1,
    // Cable in, battery full
    HOST_POWER_FULL = 2,
};

#define HOST_BATTERY_UNKNOWN 255

// GET_STATE [8]
enum host_transport {
    HOST_TRANSPORT_USB       = 0,
    HOST_TRANSPORT_2_4GHZ    = 1,
    HOST_TRANSPORT_BLUETOOTH = 2,
};

// NOTIFY_MODS bits (X11 / Hyprland modmask)
enum host_mod_bits {
    HOST_MOD_SHIFT = 1 << 0,
    HOST_MOD_CTRL  = 1 << 2,
    HOST_MOD_ALT   = 1 << 3,
    HOST_MOD_SUPER = 1 << 6,
};

// NOTIFY_RGB_KEY actions
enum host_rgb_key {
    HOST_RGB_KEY_PROFILE_NEXT    = 1,
    HOST_RGB_KEY_PROFILE_PREV    = 2,
    HOST_RGB_KEY_BRIGHTNESS_UP   = 3,
    HOST_RGB_KEY_BRIGHTNESS_DOWN = 4,
    HOST_RGB_KEY_TOGGLE          = 5,
};

enum host_base_layer {
    HOST_BASE_LAYER_UNKNOWN = 0,
    HOST_BASE_LAYER_MAC     = 1,
    HOST_BASE_LAYER_LINUX   = 2,
};

// Returns true when the packet was a host protocol command. Packets from the
// 2.4 GHz link that aren't (or come over Bluetooth) are left to VIA.
bool host_protocol_rx(uint8_t link, uint8_t *data, uint8_t length);
// Call from the scan loop: sends pending state notifications
void host_protocol_task(void);
// A host on USB pinged recently: it takes the RGB keys in direct mode (the
// lighting's host; the 2.4 GHz one doesn't count)
bool host_listening(void);
// A host takes the RGB keys while its lighting is on the keys: the USB one
// listening, or the 2.4 GHz one with its lighting loaded
bool host_takes_rgb_keys(void);
// Hands an RGB key (HOST_RGB_KEY_*) to that host
void host_notify_rgb_key(uint8_t key);
// The modifiers held now (HOST_MOD_*)
uint8_t host_mods(void);

// Keymap hooks for gaming mode (weak defaults report "unsupported")
uint8_t host_gaming_state_user(void);
bool    host_gaming_set_user(uint8_t state);
// Keymap hook: which base layer is active (HOST_BASE_LAYER_*)
uint8_t host_base_layer_user(void);
// Keymap hook: the brightness keys pressed now change the keyboard's own
// brightness rather than going to the host's whole lighting (3 / 4); default:
// the whole lighting
bool brightness_keys_are_keyboard_user(void);
// User hook: USB came back (wake, KVM switch) and no host listened within 15 s;
// lighting a host left on the keys should go back to the keyboard's own.
// Returns true if there was any (the next host gets NOTIFY_HANDBACK).
bool host_handback_user(void);
