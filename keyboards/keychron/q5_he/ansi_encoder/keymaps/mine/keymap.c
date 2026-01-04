#include QMK_KEYBOARD_H
#include "keychron_common.h"

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
};

#define FN_MAC MO(MAC_FN)
#define FN_WIN MO(WIN_FN)

// --- CONFIGURATION ---
#define MOD_LAYER 3

// --- STATE STORAGE ---
// --- STATE STORAGE ---
static bool mod_led_mask[256];      // Lookup table for fast O(1) checks in render loop
static bool is_mod_layer_active = false; // Tracks if we are currently in the layer

// Storage for restoring the previous RGB state
static uint8_t saved_rgb_mode;
static HSV saved_rgb_hsv;

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_ansi_101(
        KC_ESC,             KC_BRID,  KC_BRIU,  KC_MCTRL, KC_LNPAD, UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,            KC_DEL,   KC_F13,   KC_F14 ,  KC_F15,   KC_MUTE,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,             KC_HOME,  KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,  KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,  KC_LOPTN, KC_LCMMD,                               KC_SPC,                                 KC_RCMMD, FN_MAC,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,            KC_P0,    KC_PDOT,  KC_PENT),

    [MAC_FN] = LAYOUT_ansi_101(
        _______,            KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,             _______,  _______,  _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,
        _______,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  _______,  _______,  _______,  _______,  _______,  _______,            _______,            KC_END,   _______,  _______,  _______,  _______,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,            _______,  _______,            _______,  _______,  _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [WIN_BASE] = LAYOUT_ansi_101(
        KC_ESC,             KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,    KC_F9,    KC_F10,   KC_F11,   KC_F12,             KC_DEL,   _______,  _______,  _______,  KC_MUTE,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,     KC_T,     KC_Y,     KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        KC_CAPS,  KC_A,     KC_S,     KC_D,     KC_F,     KC_G,     KC_H,     KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,             KC_HOME,  KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,            KC_Z,     KC_X,     KC_C,     KC_V,     KC_B,     KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,  KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,  KC_LWIN,  KC_LALT,                                KC_SPC,                                 KC_RALT,  FN_WIN,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,            KC_P0,    KC_PDOT,  KC_PENT),

    [WIN_FN] = LAYOUT_ansi_101(
        _______,            KC_BRID,  KC_BRIU,  KC_TASK,  KC_FILE,  UG_VALD,  UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,            _______,  _______,  _______,  _______,  UG_TOGG,
        _______,  BT_HST1,  BT_HST2,  BT_HST3,  P2P4G,    _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        UG_TOGG,  UG_NEXT,  UG_VALU,  UG_HUEU,  UG_SATU,  UG_SPDU,  _______,  _______,  _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______,  _______,
        _______,  UG_PREV,  UG_VALD,  UG_HUED,  UG_SATD,  UG_SPDD,  _______,  _______,  _______,  _______,  _______,  _______,            _______,            KC_END,   _______,  _______,  _______,  _______,
        _______,            _______,  _______,  _______,  _______,  BAT_LVL,  _______,  _______,  _______,  _______,  _______,            _______,  _______,            _______,  _______,  _______,
        _______,  _______,  _______,                                _______,                                _______,  _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),
};

// clang-format on
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [MAC_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MAC_FN]   = {ENCODER_CCW_CW(UG_VALD, UG_VALU)},
    [WIN_BASE] = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [WIN_FN]   = {ENCODER_CCW_CW(UG_VALD, UG_VALU)},
};
#endif // ENCODER_MAP_ENABLE

void scan_mod_layer_keys(void) {
    // Clear mask
    for (uint16_t i = 0; i < 256; i++) {
        mod_led_mask[i] = false;
    }

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            // Check keys specifically on the MOD_LAYER
            uint16_t keycode = keymap_key_to_keycode(MOD_LAYER, (keypos_t){col, row});
            
            if (keycode != KC_TRNS) {
                uint8_t led_index = g_led_config.matrix_co[row][col];
                if (led_index != NO_LED) {
                    mod_led_mask[led_index] = true;
                }
            }
        }
    }
}

layer_state_t layer_state_set_user(layer_state_t state) {
    uint8_t highest_layer = get_highest_layer(state);
    
    // CASE 1: ENTERING the Mod Layer
    if (highest_layer == MOD_LAYER && !is_mod_layer_active) {
        // A. Save current state
        saved_rgb_mode = rgb_matrix_get_mode();
        saved_rgb_hsv = rgb_matrix_get_hsv();
        
        // B. Run the scan once
        scan_mod_layer_keys();

        // FORCE MODE to Solid Color (Stops the animation!)
        rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
        
        // C. Black out the board (to make the mod keys pop)
        // Note: rgb_matrix_indicators_advanced_user will handle the coloring
        
        // D. Mark flag as active
        is_mod_layer_active = true;
    } 
    
    // CASE 2: LEAVING the Mod Layer
    else if (highest_layer != MOD_LAYER && is_mod_layer_active) {
        // A. Restore previous mode
        rgb_matrix_mode_noeeprom(saved_rgb_mode);
        
        // B. Restore previous color
        rgb_matrix_sethsv_noeeprom(saved_rgb_hsv.h, saved_rgb_hsv.s, saved_rgb_hsv.v);
        
        // C. Mark flag as inactive
        is_mod_layer_active = false;
    }
    
    return state;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (is_mod_layer_active) {
        for (uint8_t i = led_min; i < led_max; i++) {
            if (mod_led_mask[i]) {
                rgb_matrix_set_color(i, 255, 255, 255);
            } else {
                rgb_matrix_set_color(i, 0, 0, 0);
            }
        }
        return false;
    }
    return true;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_F24: // This is our hijacked "Magic" key
            if (record->event.pressed) {
                // Force the RGB Matrix to Solid Color mode
                rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
            }
            return false; // Don't send F24 to the computer
    }
    return true;
}