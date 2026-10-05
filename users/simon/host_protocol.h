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
// takes the first report it reads as its answer without checking it; so for
// ~1 s after OpenRGB's protocol-version query (which opens every detection)
// replies are dropped and notifications held until the window ends.
//
// Packet layout (byte 0 = command id):
//   0xC0 GET_INFO       -> [1] protocol version, [2] feature bits, [3] indicator count,
//                          [4] max states per indicator, [5..] build date (NUL-terminated)
//   0xC1 GET_STATE      -> [1..] state block (see below)
//   0xC2 SET_GAMING     [1] 0 off, 1 rapid trigger, 2 gamepad
//   0xC3 GET_INDICATORS -> [1] count, then per indicator: wire id, state count,
//                          LED index (255 = none), state count × (r, g, b)
//   0xC4 SET_INDICATOR_COLOR [1] wire id, [2] state, [3..5] r g b, [6] persist
//   0xC5 SET_SETTING    [1] setting, [2] value, [3] persist
//                       (1 indicator brightness, 2 indicators always on, 3 blackout)
//   0xC6 RESET_INDICATOR_COLORS [1] persist
//   0xCF NOTIFY_STATE   (keyboard -> host) [1..] state block, sent when state
//                       changes on the keyboard itself (key press, Fn+P combo),
//                       never for changes a host command made
//
// Indicator wire ids: 1 macro recording, 2 leader, 3 external RGB (Hardware
// key), 4 gaming (states: rapid trigger, gamepad), 5 caps lock, 6 num lock,
// 7 fn layer (colour of the keys bound on a held FN layer; no LED of its own)
//
// State block:
//   [1] gaming state (0 off, 1 rapid, 2 gamepad, 255 unsupported)
//   [2] active Hall-effect profile (255 unsupported)
//   [3] indicator brightness  [4] indicators always on (0 = auto-hide)
//   [5] background blackout

#define HOST_PROTOCOL_VERSION 1

enum host_command_id {
    HOST_GET_INFO               = 0xC0,
    HOST_GET_STATE              = 0xC1,
    HOST_SET_GAMING             = 0xC2,
    HOST_GET_INDICATORS         = 0xC3,
    HOST_SET_INDICATOR_COLOR    = 0xC4,
    HOST_SET_SETTING            = 0xC5,
    HOST_RESET_INDICATOR_COLORS = 0xC6,
    HOST_NOTIFY_STATE           = 0xCF,
};

enum host_feature_bits {
    HOST_FEATURE_GAMING             = 1 << 0,
    HOST_FEATURE_HE_PROFILES        = 1 << 1,
    HOST_FEATURE_PERSISTENT_COLORS  = 1 << 2,
};

enum host_setting_id {
    HOST_SETTING_INDICATOR_BRIGHTNESS = 1,
    HOST_SETTING_INDICATORS_ALWAYS_ON = 2,
    HOST_SETTING_BLACKOUT             = 3,
};

#define HOST_STATE_UNSUPPORTED 255

// Returns true when the packet was a host protocol command
bool host_protocol_rx(uint8_t *data, uint8_t length);
// Call for every OpenRGB command; the protocol-version query starts the quiet window
void host_protocol_openrgb_command(uint8_t command);
// Call from the scan loop: sends pending state notifications
void host_protocol_task(void);

// Keymap hooks for gaming mode (weak defaults report "unsupported")
uint8_t host_gaming_state_user(void);
bool    host_gaming_set_user(uint8_t state);
