#pragma once

#include "quantum.h"

// --- Host lighting over 2.4 GHz (host protocol 10, 0xCA LIGHTING) ---
//
// The dongle can't carry frames (32-byte reports, one at a time), so the host
// uploads its lighting once and the keyboard draws it: a base (the direct-mode
// buffer) and openrgb-daemon's overlay layers, in its order:
//   - submap:    shown / hidden by the host (Hyprland submap), with its keys
//   - modifiers: the keyboard's own held modifiers pick a slot (keys bound
//                with exactly that combo); with SUPER (not CTRL) the number
//                keys show the workspaces, 11-20 with ALT
//   - urgent:    the number keys of urgent workspaces breathe
// Highlights light their keys and dim the rest, fading in and out.
//
// It lives in RAM: sleep keeps it, a power cycle doesn't (the heartbeat's [2]
// says whether it's loaded, so the host can send it again). Leaving 2.4 GHz,
// or the link coming back with no host speaking up within 15 s, gives the keys
// back to the keyboard's own saved effect.
//
// 0xCA LIGHTING [1] op, no replies:
//   1 RELEASE     the keyboard's own effect again; everything is dropped
//   2 BASE_FILL   [2..4] r g b: every LED, and the keys show the host's lighting
//   3 BASE_LEDS   [2] first LED, [3] count (<= 9), [4..] r g b per LED: same
//   4 LOOK        [2] submap dim, [3] submap fade (4 ms units), [4] modifier
//                 dim, [5] modifier fade, [6..8] workspace occupied, [9..11]
//                 workspace active, [12..14] urgent, [15] urgent period (20 ms
//                 units), [16] flags (1 workspaces, 2 urgent), [17..26] LED of
//                 number keys 1..9, 0 (255 = none). Dims: 255 = untouched
//   5 SLOTS_CLEAR the modifier slots
//   6 SLOT        [2] modmask (Hyprland bits, 1 shift 4 ctrl 8 alt 64 super),
//                 [3..5] r g b, [6..] LED bitmask (LED n = byte n / 8, bit n % 8)
//   7 SUBMAP      [2] 1 show, 0 hide, [3..5] r g b, [6..] LED bitmask
//   8 WORKSPACES  [2..4] active, [5..7] occupied, [8..10] urgent: 24-bit
//                 little-endian masks, bit n = workspace n + 1
// Slots describe the open submap's binds (top level when none): the host sends
// them again when a submap opens or closes.

enum overlay_op {
    OVERLAY_RELEASE     = 1,
    OVERLAY_BASE_FILL   = 2,
    OVERLAY_BASE_LEDS   = 3,
    OVERLAY_LOOK        = 4,
    OVERLAY_SLOTS_CLEAR = 5,
    OVERLAY_SLOT        = 6,
    OVERLAY_SUBMAP      = 7,
    OVERLAY_WORKSPACES  = 8,
};

#ifdef LK_WIRELESS_ENABLE
// A LIGHTING packet from the 2.4 GHz host
void overlay_rx(uint8_t *data, uint8_t length);
// The host's lighting is loaded (it may be off the keys: a local effect change)
bool overlay_loaded(void);
// Back to the keyboard's own effect
void overlay_release(void);
// Base plus overlays for one LED, as the direct-mode effect shows the base
// (unscaled); call overlay_frame() first in each render pass
void overlay_frame(void);
rgb_led_t overlay_color(uint8_t led);
#endif
