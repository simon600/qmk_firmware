#pragma once

#include "quantum.h"

// --- Host protocol (raw HID, 0xC0–0xCF) ---
//
// Lets a host service (kbd-daemon) drive keyboard state that isn't lighting:
// gaming mode, settings and indicator colours. It shares the raw HID interface
// with OpenRGB (0x01–0x09), SignalRGB (0x21–0x28) and Keychron (0xA0–0xAB).
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
//   0xC0 GET_INFO       -> [1] protocol version, [2] feature bits, [3] indicator count,
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
//                       4 layer dim: other keys while an FN layer shows)
//   0xC6 RESET_INDICATOR_COLORS [1] persist   (back to the firmware defaults)
//   0xC7 RELOAD_INDICATOR_COLORS               (back to the saved colours and layer
//                       dim: drops unsaved host overrides, e.g. an OpenRGB profile's)
//   0xC8, 0xC9          retired (protocol 3-4 workspace hints; openrgb-daemon
//                       draws them now from NOTIFY_MODS)
//   0xCC PING           heartbeat; any command counts. While the host was heard
//                       from in the last 6 s (USB), the RGB keys in direct mode
//                       go to it as NOTIFY_RGB_KEY instead of acting locally
//   0xCD NOTIFY_RGB_KEY (keyboard -> host) [1] 1 next profile, 2 previous
//                       profile, 3 brightness up, 4 brightness down, 5 lights
//                       on/off. Sent instead
//                       of acting on the key while a host streams direct mode
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

#define HOST_PROTOCOL_VERSION 7

enum host_command_id {
    HOST_GET_INFO               = 0xC0,
    HOST_GET_STATE              = 0xC1,
    HOST_SET_GAMING             = 0xC2,
    HOST_GET_INDICATORS         = 0xC3,
    HOST_SET_INDICATOR_COLOR    = 0xC4,
    HOST_SET_SETTING            = 0xC5,
    HOST_RESET_INDICATOR_COLORS = 0xC6,
    HOST_RELOAD_INDICATOR_COLORS = 0xC7,
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
};

enum host_setting_id {
    HOST_SETTING_INDICATOR_BRIGHTNESS = 1,
    HOST_SETTING_INDICATORS_ALWAYS_ON = 2,
    HOST_SETTING_BLACKOUT             = 3,
    HOST_SETTING_LAYER_DIM            = 4,
};

#define HOST_STATE_UNSUPPORTED 255

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

// Returns true when the packet was a host protocol command
bool host_protocol_rx(uint8_t *data, uint8_t length);
// Call from the scan loop: sends pending state notifications
void host_protocol_task(void);
// A host on USB pinged recently: it takes the RGB keys in direct mode
bool host_listening(void);
// Hands an RGB key (HOST_RGB_KEY_*) to the host
void host_notify_rgb_key(uint8_t key);

// Keymap hooks for gaming mode (weak defaults report "unsupported")
uint8_t host_gaming_state_user(void);
bool    host_gaming_set_user(uint8_t state);
// Keymap hook: which base layer is active (HOST_BASE_LAYER_*)
uint8_t host_base_layer_user(void);
