#include QMK_KEYBOARD_H
#include "keychron_common.h"
#include "simon.h"
#include "print.h"
#include "profile.h"

enum layers {
    MAC_BASE,
    MAC_FN,
    WIN_BASE,
    WIN_FN,
    GAMING,
    GAMING2,
    HARDWARE,
    HARDWARE2,
};

#define FN_MAC MO(MAC_FN)
#define FN_WIN MO(WIN_FN)
#define FN_LMAC LT(MAC_FN, KC_NO)
#define FN_LWIN LT(WIN_FN, KC_NO)
#define FN_HRD MO(HARDWARE)
#define FN_HRD2 MO(HARDWARE2)
#define TG_GMG TG(GAMING)
#define FN_GMG2 MO(GAMING2)
#define MR1 QK_DYNAMIC_MACRO_RECORD_START_1
#define MR2 QK_DYNAMIC_MACRO_RECORD_START_2
#define MP1 QK_DYNAMIC_MACRO_PLAY_1
#define MP2 QK_DYNAMIC_MACRO_PLAY_2
#define MS QK_DYNAMIC_MACRO_RECORD_STOP

bool gaming_mode_enabled = false;

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [MAC_BASE] = LAYOUT_ansi_101(
        KC_ESC,                          KC_BRID,    KC_BRIU,  KC_MCTRL,   KC_LNPAD, UG_VALD,    UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,    _______,  _______,  _______,            KC_DEL,   KC_F13,   KC_F14 ,  FN_HRD,   KC_MUTE,
        KC_GRV,                KC_1,     KC_2,       KC_3,     KC_4,       KC_5,     KC_6,       KC_7,     KC_8,     KC_9,     KC_0,       KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,                KC_Q,     KC_W,       KC_E,     KC_R,       KC_T,     KC_Y,       KC_U,     KC_I,     KC_O,     KC_P,       KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        GUI_T(KC_ESC),         KC_A,     KC_S,       KC_D,     KC_F,       KC_G,     KC_H,       KC_J,     KC_K,     KC_L,     KC_SCLN,    KC_QUOT,            KC_ENT,             KC_HOME,  KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,                         KC_Z,       KC_X,     KC_C,       KC_V,     KC_B,       KC_N,     KC_M,     KC_COMM,  KC_DOT,     KC_SLSH,            KC_RSFT,  KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,               KC_LOPTN, KC_LCMMD,                                   LT(MAC_FN, KC_SPC),                       KC_RALT,    FN_LMAC,  KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,            KC_P0,    KC_PDOT,  KC_PENT),

    [MAC_FN] = LAYOUT_ansi_101(
        _______,                         KC_F1,      KC_F2,    KC_F3,      KC_F4,    KC_F5,      KC_F6,    KC_F7,    KC_F8,    KC_F9,      KC_F10,   KC_F11,   KC_F12,             _______,  _______,  _______,  _______,  _______,
        _______,               MP1,      MP2,        _______,  G(KC_RGHT), _______,  _______,    _______,  _______,  _______,  G(KC_LEFT), _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               MS,       M_NM,   A(KC_RIGHT),   _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        CW_TOGG,               MR1,      MR2,        KC_DEL,   _______,    _______,  KC_LEFT,    KC_DOWN,  KC_UP,    KC_RGHT,  _______,    _______,            _______,            KC_END,   _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  A(KC_LEFT), _______,  _______,  QK_LEAD,  _______,    _______,            _______,  _______,            _______,  _______,  _______,
        _______,               _______,  _______,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [WIN_BASE] = LAYOUT_ansi_101(
        LCSG_T(KC_ESC),                  KC_F1,      KC_F2,    KC_F3,      KC_F4,    KC_F5,      KC_F6,    KC_F7,    KC_F8,    KC_F9,      KC_F10,   KC_F11,   KC_F12,             KC_DEL,   TG_GMG,   _______,  FN_HRD,   KC_MUTE,
        KC_GRV,                KC_1,     KC_2,       KC_3,     KC_4,       KC_5,     KC_6,       KC_7,     KC_8,     KC_9,     KC_0,       KC_MINS,  KC_EQL,   KC_BSPC,            KC_PGUP,  KC_NUM,   KC_PSLS,  KC_PAST,  KC_PMNS,
        KC_TAB,                KC_Q,     KC_W,       KC_E,     KC_R,       KC_T,     KC_Y,       KC_U,     KC_I,     KC_O,     KC_P,       KC_LBRC,  KC_RBRC,  KC_BSLS,            KC_PGDN,  KC_P7,    KC_P8,    KC_P9,
        LCTL_T(KC_ESC),        KC_A,     KC_S,       KC_D,     KC_F,       KC_G,     KC_H,       KC_J,     KC_K,     KC_L,     KC_SCLN,    KC_QUOT,            KC_ENT,             KC_HOME,  KC_P4,    KC_P5,    KC_P6,    KC_PPLS,
        KC_LSFT,                         KC_Z,       KC_X,     KC_C,       KC_V,     KC_B,       KC_N,     KC_M,     KC_COMM,  KC_DOT,     KC_SLSH,            KC_RSFT,  KC_UP,              KC_P1,    KC_P2,    KC_P3,
        KC_LCTL,               KC_LWIN,  KC_LALT,                                    LT(WIN_FN, KC_SPC),                       KC_RALT,    FN_LWIN,  KC_RCTL,  KC_LEFT,  KC_DOWN,  KC_RGHT,            KC_P0,    KC_PDOT,  KC_PENT),

    [WIN_FN] = LAYOUT_ansi_101(
        _______,                         KC_BRID,    KC_BRIU,  KC_TASK,    KC_FILE,  UG_VALD,    UG_VALU,  KC_MPRV,  KC_MPLY,  KC_MNXT,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               MP1,      MP2,        _______,  KC_END,     _______,  _______,    _______,  _______,  _______,  KC_HOME,    _______,  _______,  KC_DEL,             _______,  _______,  _______,  _______,  _______,
        _______,               MS,       M_NW,    C(KC_RGHT),  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        CW_TOGG,               MR1,      MR2,        KC_DEL,   _______,    _______,  KC_LEFT,    KC_DOWN,  KC_UP,    KC_RGHT,  _______,    _______,            _______,            KC_END,   _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  C(KC_LEFT), _______,  _______,  QK_LEAD,  _______,    _______,            _______,  _______,            _______,  _______,      _______,
        _______,               _______,  _______,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [GAMING] = LAYOUT_ansi_101(
        KC_ESC,                          _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        KC_RALT,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,            _______,  _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,  _______,            MP1,      MP2,      _______,
        _______,               _______,  _______,                                    KC_SPC,                                   _______,    _______,  _______,  _______,  _______,  _______,            FN_GMG2,  _______,  _______),

    [GAMING2] = LAYOUT_ansi_101(
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,            _______,  _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,  _______,            MR1,      MR2,      _______,
        _______,               _______,  _______,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [HARDWARE] = LAYOUT_ansi_101(
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  UG_TOGG,
        _______,               BT_HST1,  BT_HST2,    BT_HST3,  P2P4G,      _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        UG_SRGB,               UG_NEXT,  UG_VALU,    UG_HUEU,  UG_SATU,    UG_SPDU,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        _______,               UG_PREV,  UG_VALD,    UG_HUED,  UG_SATD,    UG_SPDD,  _______,    _______,  _______,  _______,  _______,    _______,            _______,            _______,  _______,  _______,  _______,  _______,
        _______,                         UG_ANIM1,   _______,  _______,    _______,  BAT_LVL,    _______,  _______,  _______,  _______,    _______,            _______,  _______,            _______,  _______,  _______,
        _______,               _______,  FN_HRD2,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),

    [HARDWARE2] = LAYOUT_ansi_101(
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,  _______,  _______,            _______,  _______,  _______,  _______,
        _______,               _______,  _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,            _______,  _______,  _______,  _______,  _______,
        _______,                         _______,    _______,  _______,    _______,  _______,    _______,  _______,  _______,  _______,    _______,            _______,  _______,            _______,  _______,  _______,
        _______,               _______,  _______,                                    _______,                                  _______,    _______,  _______,  _______,  _______,  _______,            _______,  _______,  _______),
};

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [MAC_BASE]  = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MAC_FN]    = {ENCODER_CCW_CW(MS_WHLU, MS_WHLD)},
    [WIN_BASE]  = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [WIN_FN]    = {ENCODER_CCW_CW(MS_WHLU, MS_WHLD)},
    [GAMING]    = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [GAMING2]   = {ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
    [HARDWARE]  = {ENCODER_CCW_CW(UG_VALD, UG_VALU)},
    [HARDWARE2] = {ENCODER_CCW_CW(IND_BR_D, IND_BR_U)},
};
#endif // ENCODER_MAP_ENABLE
// clang-format on

// --- KEYBOARD-SPECIFIC INDICATOR CONFIGURATION ---

// --- QMK CALLBACK WRAPPERS ---

void keyboard_post_init_user(void) {
    keyboard_post_init_shared();
}

void matrix_scan_user(void) {
    matrix_scan_shared();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    return process_record_shared(keycode, record);
}

layer_state_t layer_state_set_user(layer_state_t state) {
    bool win_fn_active  = layer_state_cmp(state, WIN_FN);
    bool mac_fn_active  = layer_state_cmp(state, MAC_FN);
    bool hrdw_fn_active = layer_state_cmp(state, HARDWARE) || layer_state_cmp(state, HARDWARE2);
    bool gaming_active  = layer_state_cmp(state, GAMING);

    if (!gaming_mode_enabled && gaming_active) {
        profile_select(1, false);
    } else if (gaming_mode_enabled && !gaming_active) {
        profile_select(0, false);
    }
    gaming_mode_enabled = gaming_active;

    uint8_t fn_layer = 0;
    if (win_fn_active)
        fn_layer = WIN_FN;
    else if (mac_fn_active)
        fn_layer = MAC_FN;
    else if (hrdw_fn_active)
        fn_layer = HARDWARE;
    else if (layer_state_cmp(state, GAMING2))
        fn_layer = GAMING2;

    // Update shared state
    set_active_fn_layer(fn_layer);

    // Update gaming indicator using compile-time registry
    indicator_t *indicators             = get_indicators();
    indicators[INDICATOR_GAMING].active = gaming_active;

    return layer_state_set_shared(state);
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    return rgb_matrix_indicators_advanced_shared(led_min, led_max);
}

bool get_permissive_hold(uint16_t keycode, keyrecord_t *record) {
    return get_permissive_hold_shared(keycode, record);
}

void leader_start_user(void) {
    leader_start_shared();
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

#if defined(VIA_ENABLE)
bool via_command_user(uint8_t src, uint8_t *data, uint8_t length) {
    return via_command_shared(src, data, length);
}
#endif
