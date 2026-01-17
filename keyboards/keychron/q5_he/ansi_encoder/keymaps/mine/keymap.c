#include QMK_KEYBOARD_H
#include "keychron_common.h"
#ifdef SIGNALRGB_ENABLE
#   include "signalrgb.h"
#endif

// --- STATE STORAGE ---
static bool rgb_adjusted_in_fn = false; // Tracks if an RGB key was pressed while in Fn layer
static bool is_fn_layer_active = false; // Tracks if we are currently in either Fn layer
static bool is_gaming_layer_active = false; // Tracks if we are currently in GAMING layer
#ifdef SIGNALRGB_ENABLE
static uint32_t last_srgb_activity = 0;
static bool srgb_active_timeout = false;
static bool srgb_was_enabled_before_fn = false;
static bool keyboard_srgb_enabled = true;  // Track if keyboard wants SignalRGB enabled
static bool process_srgb = true;
static bool was_srgb_active = true;
#endif

enum custom_keycodes {
    UG_SRGB = SAFE_RANGE,
    UG_ANIM1,
    UG_ANIM2,
    UG_ANIM3,
    M_ENDW,
    M_ENDM,
};

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
    GAMING,
    HARDWARE,
};

#define FN_MAC MO(MAC_FN)
#define FN_WIN MO(WIN_FN)
#define FN_HRD MO(HARDWARE)
#define TG_GMG TG(GAMING)

// --- CONFIGURATION ---
enum {
    TD_SLSH_BLSH = 0
};

#define GAMING_MODE_IDX 14

tap_dance_action_t tap_dance_actions[] = {
    [TD_SLSH_BLSH] = ACTION_TAP_DANCE_DOUBLE(KC_SLSH, KC_BSLS)
};

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_ansi_101(
        KC_ESC,                          KC_BRID,    KC_BRIU,  KC_MCTRL,   KC_LNPAD, UG_VALD,    UG_VALU,  _______,  _______,  KC_MPLY,    KC_MNXT,  KC_MPRV,  _______,            KC_DEL,   KC_F13,   KC_F14 ,  FN_HRD,   KC_MUTE,
        KC_GRV,                KC_1,     KC_2,       KC_3,     KC_4,       KC_5,     KC_6,       KC_7,     KC_8,     KC_9,     KC_0,       KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,                KC_Q,     KC_W,       KC_E,     KC_R,       KC_T,     KC_Y,       KC_U,     KC_I,     KC_O,     KC_P,       KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        GUI_T(KC_ESC),         KC_A,     KC_S,       KC_D,     KC_F,       KC_G,     KC_H,       KC_J,     KC_K,     KC_L,     KC_SCLN,    KC_QUOT,            KC_ENT,             KC_HOME,  KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,                         KC_Z,       KC_X,     KC_C,       KC_V,     KC_B,       KC_N,     KC_M,     KC_COMM,  KC_DOT,     KC_SLSH,            KC_RSFT,  KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,               KC_LOPTN, KC_LCMMD,                                   LT(MAC_FN, KC_SPC),                       KC_RCMMD,   FN_MAC,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,            KC_P0,    KC_PDOT,  KC_PENT),

    [MAC_FN] = LAYOUT_ansi_101(
        _______,                         KC_F1,      KC_F2,    KC_F3,      KC_F4,    KC_F5,      KC_F6,    KC_F7,    KC_F8,    KC_F9,      KC_F10,   KC_F11,   KC_F12,             _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  G(KC_RGHT), _______,  _______,    _______,  _______,  _______,  G(KC_LEFT), _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  G(KC_RGHT), M_ENDM,   _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        CW_TOGG,               _______,  _______,    KC_DEL,   _______,    _______,  KC_LEFT,    KC_DOWN,  KC_UP,    KC_RGHT,  _______,    _______,            _______,            KC_END,   _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  G(KC_LEFT), _______,  _______,  QK_LEAD,  _______,    _______,            _______,  _______,            _______,  _______,  _______,
        _______,               _______,  _______,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [WIN_BASE] = LAYOUT_ansi_101(
        LCSG_T(KC_ESC),                  KC_F1,      KC_F2,    KC_F3,      KC_F4,    KC_F5,      KC_F6,    KC_F7,    KC_F8,    KC_F9,      KC_F10,   KC_F11,   KC_F12,             KC_DEL,   _______,  _______,  FN_HRD,   KC_MUTE,
        KC_GRV,                KC_1,     KC_2,       KC_3,     KC_4,       KC_5,     KC_6,       KC_7,     KC_8,     KC_9,     KC_0,       KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,                KC_Q,     KC_W,       KC_E,     KC_R,       KC_T,     KC_Y,       KC_U,     KC_I,     KC_O,     KC_P,       KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        LCTL_T(KC_ESC),        KC_A,     KC_S,       KC_D,     KC_F,       KC_G,     KC_H,       KC_J,     KC_K,     KC_L,     KC_SCLN,    KC_QUOT,            KC_ENT,             KC_HOME,  KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,                         KC_Z,       KC_X,     KC_C,       KC_V,     KC_B,       KC_N,     KC_M,     KC_COMM,  KC_DOT,     KC_SLSH,            KC_RSFT,  KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,               KC_LWIN,  KC_LALT,                                    LT(WIN_FN, KC_SPC),                       KC_RALT,    FN_WIN,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,            KC_P0,    KC_PDOT,  KC_PENT),

    [WIN_FN] = LAYOUT_ansi_101(
        _______,                         KC_BRID,    KC_BRIU,  KC_TASK,    KC_FILE,  UG_VALD,    UG_VALU,  _______,  _______,  KC_MPLY,    KC_MNXT,  KC_MPRV,  _______,            _______,  TG_GMG,   _______,  _______,  _______,
        _______,               _______,  _______,    _______,  KC_END,     _______,  _______,    _______,  _______,  _______,  KC_HOME,    _______,  _______,  KC_DEL,             _______,  _______,  _______,  _______,  _______,
        _______,               _______,  C(KC_RGHT), M_ENDW,   _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        CW_TOGG,               _______,  _______,    KC_DEL,   _______,    _______,  KC_LEFT,    KC_DOWN,  KC_UP,    KC_RGHT,  _______,    _______,            _______,            KC_END,   _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  C(KC_LEFT), _______,  _______,  QK_LEAD,  _______,    _______,            _______,  _______,            _______,  _______,  _______,
        _______,               _______,  _______,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [GAMING] = LAYOUT_ansi_101(
        KC_ESC,                          _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,            _______,  _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,  _______,            _______,  _______,  _______,
        _______,               _______,  _______,                                    KC_SPC,                                   _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [HARDWARE] = LAYOUT_ansi_101(
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  UG_TOGG,
        _______,               BT_HST1,  BT_HST2,    BT_HST3,  P2P4G,      _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        UG_SRGB,               UG_NEXT,  UG_VALU,    UG_HUEU,  UG_SATU,    UG_SPDU,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        _______,               UG_PREV,  UG_VALD,    UG_HUED,  UG_SATD,    UG_SPDD,  _______,    _______,  _______,  _______,  _______,    _______,            _______,            _______,  _______,  _______,  _______,  _______,
        _______,                         UG_ANIM1,   _______,  _______,    _______,  BAT_LVL,    _______,  _______,  _______,  _______,    _______,            _______,  _______,            _______,  _______,  _______,
        _______,               _______,  _______,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),
};

// clang-format on
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [MAC_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MAC_FN]   = {ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
    [WIN_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [WIN_FN]   = {ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
    [GAMING]   = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [HARDWARE] = {ENCODER_CCW_CW(UG_VALD, UG_VALU)},
};
#endif // ENCODER_MAP_ENABLE

static bool mod_led_mask[256];      // Lookup table for fast O(1) checks in render loop

void update_mod_led_mask(uint8_t fn_layer) {
    // Clear mask
    for (uint16_t i = 0; i < 256; i++) {
        mod_led_mask[i] = false;
    }

    // If FN layer is active, mask keys with non-transparent keycodes
    if (is_fn_layer_active) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                uint16_t keycode = keymap_key_to_keycode(fn_layer, (keypos_t){col, row});

                if (keycode != KC_TRNS) {
                    uint8_t led_index = g_led_config.matrix_co[row][col];
                    if (led_index != NO_LED) {
                        mod_led_mask[led_index] = true;
                    }
                }
            }
        }
    }

    // If gaming layer is active, mask the gaming mode indicator
    if (is_gaming_layer_active) {
        mod_led_mask[GAMING_MODE_IDX] = true;
    }

    // Sync with SignalRGB
#ifdef SIGNALRGB_ENABLE
    signalrgb_sync_mask(mod_led_mask);
#endif
}

layer_state_t layer_state_set_user(layer_state_t state) {
    bool win_fn_active = (state & (1UL << WIN_FN));
    bool mac_fn_active = (state & (1UL << MAC_FN));
    bool hrdw_fn_active = (state & (1UL << HARDWARE));
    bool gaming_active = (state & (1UL << GAMING));

    bool was_gaming_active = is_gaming_layer_active;
    is_gaming_layer_active = gaming_active;

    bool was_fn_layer_active = is_fn_layer_active;
    uint8_t fn_layer = 0;

    if (win_fn_active) {
        is_fn_layer_active = true;
        fn_layer = WIN_FN;
    } else if (mac_fn_active) {
        is_fn_layer_active = true;
        fn_layer = MAC_FN;
    } else if (hrdw_fn_active) {
        is_fn_layer_active = true;
        fn_layer = HARDWARE;
    } else {
        is_fn_layer_active = false;
        rgb_adjusted_in_fn = false;
    }

    // Entering FN layer
    if (!was_fn_layer_active && is_fn_layer_active) {
        rgb_adjusted_in_fn = false;
    }

    // Exiting FN layer
    if (was_fn_layer_active && !is_fn_layer_active) {
        // State restoration handled by SignalRGB module via HID
    }

    // Update mask whenever gaming or FN layer state changes
    if (was_gaming_active != is_gaming_layer_active || 
        was_fn_layer_active != is_fn_layer_active) {
        update_mod_led_mask(fn_layer);
    }

    return state;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (is_gaming_layer_active) {
        rgb_matrix_set_color(GAMING_MODE_IDX, 255, 255, 255);
    }

    if (is_fn_layer_active) {
        for (uint8_t i = led_min; i < led_max; i++) {
            if (mod_led_mask[i]) {
                rgb_matrix_set_color(i, 255, 255, 255);
            } else if (!rgb_adjusted_in_fn && !process_srgb) {
                 rgb_matrix_set_color(i, 0, 0, 0);
            }
        }
    }
    return true;
}

bool get_permissive_hold(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LCTL_T(KC_ESC):
        case GUI_T(KC_ESC):
            return true;
        default:
            return false;
    }
}

void keyboard_post_init_user(void) {
    rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
    rgb_matrix_sethsv(156, 191, 255);
#ifdef SIGNALRGB_ENABLE
    last_srgb_activity = timer_read32();
#endif
}

#ifdef SIGNALRGB_ENABLE
void matrix_scan_user(void) {
    if (!srgb_active_timeout && process_srgb) {
        if (timer_elapsed32(last_srgb_activity) > 2000) {
            srgb_active_timeout = true;
        }
    }
    if (is_fn_layer_active && !rgb_adjusted_in_fn) {
        process_srgb = false;
    } else {
        process_srgb = keyboard_srgb_enabled;
    }
    bool is_srgb_active = process_srgb && !srgb_active_timeout;
    if (was_srgb_active && !is_srgb_active) {
        signalrgb_mode_disable();
    } else if (!was_srgb_active && is_srgb_active) {
        signalrgb_mode_enable();
    }
    if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_SIGNALRGB && !srgb_active_timeout) {
        process_srgb = false;
    }
    was_srgb_active = process_srgb && !srgb_active_timeout;
}
#endif

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_caps_word(keycode, record)) { return false; }

    if (is_fn_layer_active && record->event.pressed) {
        switch (keycode) {
            case UG_TOGG:
            case UG_NEXT:
            case UG_PREV:
            case UG_VALU:
            case UG_VALD:
            case UG_HUEU:
            case UG_HUED:
            case UG_SATU:
            case UG_SATD:
            case UG_SPDU:
            case UG_SPDD:
            case UG_SRGB:
            case UG_ANIM1:
            case UG_ANIM2:
            case UG_ANIM3:
                if (!rgb_adjusted_in_fn) {
                    rgb_adjusted_in_fn = true;
                }
                break;
        }
    }

    switch (keycode) {
        case UG_SRGB:
            if (record->event.pressed) {
#ifdef SIGNALRGB_ENABLE
                keyboard_srgb_enabled = !keyboard_srgb_enabled;
                
                if (keyboard_srgb_enabled) {
                    // When enabling, reset timeout tracking
                    last_srgb_activity = timer_read32();
                    srgb_active_timeout = false;
                    srgb_was_enabled_before_fn = true;
                }
#endif
            }
            return false;
        
        case UG_ANIM1:
            if (record->event.pressed) {
                 rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
            }
            return false;
            
        case UG_ANIM2:
        case UG_ANIM3:
            return false;
        case M_ENDW:
            if (record->event.pressed) {
                tap_code16(C(KC_RGHT));
                tap_code16(KC_RGHT);
            }
            return false;
        case M_ENDM:
            if (record->event.pressed) {
                tap_code16(G(KC_RGHT));
                tap_code16(KC_LEFT);
            }
            return false;
#ifdef SIGNALRGB_ENABLE
        case UG_VALU:
            if (keyboard_srgb_enabled && !srgb_active_timeout) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_EQL))));
                }
                return false;
            }
            break;
        case UG_VALD:
            if (keyboard_srgb_enabled && !srgb_active_timeout) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_MINS))));
                }
                return false;
            }
            break;
        case UG_NEXT:
            if (keyboard_srgb_enabled && !srgb_active_timeout) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_Q))));
                }
                return false;
            }
            break;
        case UG_PREV:
            if (keyboard_srgb_enabled && !srgb_active_timeout) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_A))));
                }
                return false;
            }
            break;


#endif
    }
    return true;
}

void leader_start_user(void) {
}

void leader_end_user(void) {
    if (leader_sequence_one_key(KC_E)) {
        SEND_STRING("szymek.fogiel@gmail.com");
    }
}

#if defined(VIA_ENABLE) && defined(SIGNALRGB_ENABLE)
extern bool kc_raw_hid_rx(uint8_t src, uint8_t *data, uint8_t length);
extern bool srgb_raw_hid_rx(uint8_t *data, uint8_t length);

bool via_command_kb(uint8_t src, uint8_t *data, uint8_t length) {
    // Don't process SignalRGB HID messages if keyboard has disabled SignalRGB
    // This prevents SignalRGB app from re-enabling when user toggled it off
    if (!process_srgb) {
        return kc_raw_hid_rx(src, data, length);
    }
    
#ifdef SIGNALRGB_ENABLE
    if (srgb_raw_hid_rx(data, length)) {
        last_srgb_activity = timer_read32();
        // Clear timeout flag when receiving data
        if (srgb_active_timeout) {
            srgb_active_timeout = false;
        }
        return true;
    }
#endif
    return kc_raw_hid_rx(src, data, length);
}
#endif
