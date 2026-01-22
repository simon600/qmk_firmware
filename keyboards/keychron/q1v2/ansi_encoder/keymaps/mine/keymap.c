/* Copyright 2023 ~ 2025 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include "simon.h"
#include "print.h"

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
    HARDWARE,
};

#define FN_MAC MO(MAC_FN)
#define FN_WIN MO(WIN_FN)
#define FN_HRD MO(HARDWARE)
#define MR1 QK_DYNAMIC_MACRO_RECORD_START_1
#define MR2 QK_DYNAMIC_MACRO_RECORD_START_2
#define MP1 QK_DYNAMIC_MACRO_PLAY_1
#define MP2 QK_DYNAMIC_MACRO_PLAY_2
#define MS QK_DYNAMIC_MACRO_RECORD_STOP

// clang-format off
// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_ansi_82(
        KC_ESC,         KC_BRID,    KC_BRIU,  KC_MCTL,    KC_LPAD,    UG_VALD,    UG_VALU,    KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,  KC_VOLD,  KC_VOLU,  KC_DEL,             KC_F13,
        KC_GRV,         KC_1,       KC_2,     KC_3,       KC_4,       KC_5,       KC_6,       KC_7,     KC_8,     KC_9,     KC_0,     KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,
        KC_TAB,         KC_Q,       KC_W,     KC_E,       KC_R,       KC_T,       KC_Y,       KC_U,     KC_I,     KC_O,     KC_P,     KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,
        GUI_T(KC_ESC),  KC_A,       KC_S,     KC_D,       KC_F,       KC_G,       KC_H,       KC_J,     KC_K,     KC_L,     KC_SCLN,  KC_QUOT,            KC_ENT,             KC_HOME,
        KC_LSFT,                    KC_Z,     KC_X,       KC_C,       KC_V,       KC_B,       KC_N,     KC_M,     KC_COMM,  KC_DOT,   KC_SLSH,            KC_RSFT,  KC_UP,
        KC_LCTL,        KC_LOPTN,   KC_LCMMD,                                     LT(MAC_FN, KC_SPC),                       KC_RCMMD, FN_MAC,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [MAC_FN] = LAYOUT_ansi_82(
        _______,        KC_F1,      KC_F2,    KC_F3,      KC_F4,      KC_F5,      KC_F6,      KC_F7,    KC_F8,    KC_F9,    KC_F10,     KC_F11,   KC_F12,   _______,            _______,
        _______,        MP1,        MP2,      _______,    G(KC_RGHT), _______,    _______,    _______,  _______,  _______,  G(KC_LEFT), _______,  _______,  _______,            _______,
        _______,        MS,         G(KC_RGHT),M_ENDM,    _______,    _______,    _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,
        CW_TOGG,        MR1,        MR2,      KC_DEL,     _______,    _______,    KC_LEFT,    KC_DOWN,  KC_UP,    KC_RGHT,  _______,    _______,            _______,            KC_END,
        _______,                    _______,  _______,    _______,    _______,    G(KC_LEFT), _______,  _______,  QK_LEAD,  _______,    _______,            _______,  _______,
        _______,        _______,    _______,                                      _______,                                  _______,    _______,  _______,  _______,  _______,  _______),

    [WIN_BASE] = LAYOUT_ansi_82(
        KC_ESC,         KC_F1,      KC_F2,    KC_F3,      KC_F4,      KC_F5,      KC_F6,      KC_F7,    KC_F8,    KC_F9,    KC_F10,     KC_F11,   KC_F12,   KC_DEL,             KC_MUTE,
        KC_GRV,         KC_1,       KC_2,     KC_3,       KC_4,       KC_5,       KC_6,       KC_7,     KC_8,     KC_9,     KC_0,       KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,
        KC_TAB,         KC_Q,       KC_W,     KC_E,       KC_R,       KC_T,       KC_Y,       KC_U,     KC_I,     KC_O,     KC_P,       KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,
        LCTL_T(KC_ESC), KC_A,       KC_S,     KC_D,       KC_F,       KC_G,       KC_H,       KC_J,     KC_K,     KC_L,     KC_SCLN,    KC_QUOT,            KC_ENT,             KC_HOME,
        KC_LSFT,                    KC_Z,     KC_X,       KC_C,       KC_V,       KC_B,       KC_N,     KC_M,     KC_COMM,  KC_DOT,     KC_SLSH,            KC_RSFT,  KC_UP,
        KC_LCTL,        KC_LWIN,    KC_LALT,                                      LT(WIN_FN, KC_SPC),                       KC_RALT,    FN_WIN,   KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT),

    [WIN_FN] = LAYOUT_ansi_82(
        _______,        KC_BRID,    KC_BRIU,  KC_TASK,    KC_FILE,    UG_VALD,    UG_VALU,    KC_MPRV,  KC_MPLY,  KC_MNXT,  KC_MUTE,    KC_VOLD,  KC_VOLU,  _______,            _______,
        _______,        MP1,        MP2,      _______,    KC_END,     _______,    _______,    _______,  _______,  _______,  KC_HOME,    _______,  _______,  KC_DEL,             _______,
        _______,        MS,         C(KC_RGHT),M_ENDW,    _______,    _______,    _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,
        CW_TOGG,        MR1,        MR2,      KC_DEL,     _______,    _______,    KC_LEFT,    KC_DOWN,  KC_UP,    KC_RGHT,  _______,    _______,            _______,            KC_END,
        _______,                    _______,  _______,    _______,    _______,    C(KC_LEFT), _______,  _______,  QK_LEAD,  _______,    _______,            _______,  _______,
        _______,        _______,    _______,                                    _______,                                    _______,    _______,  _______,  _______,  _______,  _______),

    [HARDWARE] = LAYOUT_ansi_82(
        _______,        _______,    _______,  _______,    _______,    _______,    _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            UG_TOGG,
        _______,        _______,    _______,  _______,    _______,    _______,    _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,
        _______,        UG_NEXT,    UG_VALU,  UG_HUEU,    UG_SATU,    UG_SPDU,    _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,
        _______,        UG_PREV,    UG_VALD,  UG_HUED,    UG_SATD,    UG_SPDD,    _______,    _______,  _______,  _______,  _______,    _______,            _______,            _______,
        _______,                    UG_ANIM1, _______,    _______,    _______,    _______,    _______,  _______,  _______,  _______,    _______,            _______,  _______,
        _______,        _______,    _______,                                      _______,                                  _______,    _______,  _______,  _______,  _______,  _______)
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [MAC_BASE] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MAC_FN]   = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
    [WIN_BASE] = { ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [WIN_FN]   = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
    [HARDWARE] = { ENCODER_CCW_CW(UG_VALD, UG_VALU)}
};
#endif // ENCODER_MAP_ENABLE

// --- QMK CALLBACK WRAPPERS ---

void keyboard_post_init_user(void) {
    keyboard_post_init_shared();
}

void matrix_scan_user(void) {
    matrix_scan_shared();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_shared(keycode, record)) {
        return false;
    }
    return true;
}

layer_state_t layer_state_set_user(layer_state_t state) {
    bool win_fn_active = layer_state_cmp(state, WIN_FN);
    bool mac_fn_active = layer_state_cmp(state, MAC_FN);

    // Determine current FN layer
    uint8_t fn_layer = 0;
    if (win_fn_active)
        fn_layer = WIN_FN;
    else if (mac_fn_active)
        fn_layer = MAC_FN;

    // Update shared state
    set_active_fn_layer(fn_layer);

    return layer_state_set_shared(state);
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    return rgb_matrix_indicators_advanced_shared(led_min, led_max);
}

bool get_permissive_hold(uint16_t keycode, keyrecord_t *record) {
    return get_permissive_hold_shared(keycode, record);
}

void leader_end_user(void) {
    leader_end_shared();
}

bool dynamic_macro_record_start_user(int8_t direction) {
    return dynamic_macro_record_start_shared(direction);
}

bool dynamic_macro_record_end_user(int8_t direction) {
    return dynamic_macro_record_end_shared(direction);
}

