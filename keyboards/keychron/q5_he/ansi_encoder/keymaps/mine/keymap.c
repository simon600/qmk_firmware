#include QMK_KEYBOARD_H
#include "keychron_common.h"
#ifdef SIGNALRGB_ENABLE
#    include "signalrgb.h"
#endif

#define GAMING_IND_IDX 0
#define SRGB_IND_IDX 1

// --- STATE STORAGE ---

// RGB rendering state - tracks what should be displayed
typedef struct {
    bool    signalrgb_should_process; // Whether SignalRGB should handle LED updates
    bool    show_fn_layer_mask;       // Whether to show FN layer highlights
    uint8_t active_fn_layer;          // Which FN layer is active (0 if none)
    bool    gaming_indicator_on;      // Gaming layer indicator state
    bool    srgb_indicator_on;        // SignalRGB indicator state
    bool    rgb_adjusted_in_fn;       // Whether RGB was adjusted while in FN layer
} rgb_state_t;

#ifdef SIGNALRGB_ENABLE
// SignalRGB control state - tracks SignalRGB-specific conditions
typedef struct {
    uint32_t last_activity;    // Last HID activity timestamp
    bool     timed_out;        // Whether SignalRGB has timed out
    bool     user_enabled;     // Whether user has enabled SignalRGB via toggle
    bool     fn_layer_blocked; // Whether FN layer is blocking SignalRGB
} signalrgb_state_t;

// Pending state changes - applied atomically in matrix_scan_user
typedef struct {
    bool        has_pending;      // Whether there are pending changes
    rgb_state_t next_rgb;         // Next RGB state to apply
    bool        recalc_mask;      // Whether to recalculate LED mask
    bool        update_signalrgb; // Whether to call signalrgb_mode_enable/disable
} pending_state_t;

static rgb_state_t       current_rgb = {0};
static signalrgb_state_t srgb_state  = {0};
static pending_state_t   pending     = {0};
#else
static rgb_state_t current_rgb = {0};
#endif

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;
typedef struct {
    uint8_t   index;
    bool      active;
    rgb_led_t color;
} indicator_state_t;

#define INDICATOR_COUNT 2
static indicator_state_t indicators[INDICATOR_COUNT] = {
    {.index = 14, .active = false, .color = {255, 0, 0}},
    {.index = 36, .active = false, .color = {255, 255, 255}},
};

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
enum { TD_SLSH_BLSH = 0 };

tap_dance_action_t tap_dance_actions[] = {[TD_SLSH_BLSH] = ACTION_TAP_DANCE_DOUBLE(KC_SLSH, KC_BSLS)};

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

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [MAC_BASE]  = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [MAC_FN]    = {ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
    [WIN_BASE]  = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [WIN_FN]    = {ENCODER_CCW_CW(KC_TRNS, KC_TRNS)},
    [GAMING]    = {ENCODER_CCW_CW(KC_VOLD, KC_VOLU)},
    [HARDWARE]  = {ENCODER_CCW_CW(UG_VALD, UG_VALU)},
};
#endif // ENCODER_MAP_ENABLE
// clang-format on

static bool mod_led_mask[256]; // Lookup table for fast O(1) checks in render loop

// --- STATE MANAGEMENT FUNCTIONS ---

#ifdef SIGNALRGB_ENABLE
// Calculate whether SignalRGB should process LED updates
static bool calculate_signalrgb_should_process(void) {
    if (!srgb_state.user_enabled) return false;
    if (srgb_state.timed_out) return false;
    if (srgb_state.fn_layer_blocked) return false;
    return true;
}

// Calculate the next RGB state based on current conditions
static void calculate_next_rgb_state(rgb_state_t *next) {
    next->signalrgb_should_process = calculate_signalrgb_should_process();
    next->show_fn_layer_mask       = (current_rgb.active_fn_layer != 0);
    next->active_fn_layer          = current_rgb.active_fn_layer;
    next->gaming_indicator_on      = current_rgb.gaming_indicator_on;
    // NOTE: rgb_adjusted_in_fn is NOT copied here - it's set immediately in process_record_user
    // and doesn't need to go through the pending mechanism
    next->srgb_indicator_on = srgb_state.user_enabled && !next->signalrgb_should_process;
}

// Commit a state change to be applied atomically on next matrix scan
static void commit_state_change(bool need_mask_update, bool need_srgb_update) {
    calculate_next_rgb_state(&pending.next_rgb);
    pending.recalc_mask      = need_mask_update;
    pending.update_signalrgb = need_srgb_update;
    pending.has_pending      = true;
}
#endif

void update_mod_led_mask(uint8_t fn_layer) {
    // Clear mask
    for (uint16_t i = 0; i < 256; i++) {
        mod_led_mask[i] = false;
    }

    // If FN layer is active, mask keys with non-transparent keycodes
    if (current_rgb.active_fn_layer != 0) {
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

    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        if (indicators[i].active) {
            uint8_t led_index = indicators[i].index;
            if (led_index != NO_LED) {
                mod_led_mask[led_index] = true;
            }
        }
    }

    // Sync with SignalRGB
#ifdef SIGNALRGB_ENABLE
    signalrgb_sync_mask(mod_led_mask);
#endif
}

layer_state_t layer_state_set_user(layer_state_t state) {
    bool win_fn_active  = layer_state_cmp(state, WIN_FN);
    bool mac_fn_active  = layer_state_cmp(state, MAC_FN);
    bool hrdw_fn_active = layer_state_cmp(state, HARDWARE);
    bool gaming_active  = layer_state_cmp(state, GAMING);

    bool was_fn_active = (current_rgb.active_fn_layer != 0);

    // Determine current FN layer
    uint8_t fn_layer = 0;
    if (win_fn_active)
        fn_layer = WIN_FN;
    else if (mac_fn_active)
        fn_layer = MAC_FN;
    else if (hrdw_fn_active)
        fn_layer = HARDWARE;

    bool is_fn_active = (fn_layer != 0);

    // Update state tracking
    current_rgb.active_fn_layer     = fn_layer;
    current_rgb.gaming_indicator_on = gaming_active;

    // Reset RGB adjustment flag when entering FN layer
    if (!was_fn_active && is_fn_active) {
        current_rgb.rgb_adjusted_in_fn = false;
    }

#ifdef SIGNALRGB_ENABLE
    // Update SignalRGB blocking based on FN layer
    bool was_blocked            = srgb_state.fn_layer_blocked;
    srgb_state.fn_layer_blocked = (is_fn_active && !current_rgb.rgb_adjusted_in_fn);

    // Commit changes (need mask update due to layer change)
    bool need_srgb_update = (was_blocked != srgb_state.fn_layer_blocked);
    commit_state_change(true, need_srgb_update);
#else
    // Without SignalRGB, just update the mask
    update_mod_led_mask(fn_layer);
    indicators[GAMING_IND_IDX].active = current_rgb.gaming_indicator_on;
#endif

    return state;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    if (current_rgb.show_fn_layer_mask) {
        for (uint8_t i = led_min; i < led_max; i++) {
            if (mod_led_mask[i]) {
                rgb_matrix_set_color(i, 255, 255, 255);
            } else if (!current_rgb.rgb_adjusted_in_fn && !current_rgb.signalrgb_should_process) {
                // Only black out if RGB hasn't been adjusted AND SignalRGB isn't processing
                rgb_matrix_set_color(i, 0, 0, 0);
            }
            // Otherwise, let the RGB matrix show (either QMK effects or SignalRGB)
        }
    }

    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        if (indicators[i].active) {
            rgb_matrix_set_color(indicators[i].index, indicators[i].color.r, indicators[i].color.g, indicators[i].color.b);
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
    // Initialize SignalRGB state
    srgb_state.last_activity    = timer_read32();
    srgb_state.user_enabled     = true;
    srgb_state.timed_out        = false;
    srgb_state.fn_layer_blocked = false;

    // Initialize RGB rendering state
    current_rgb.signalrgb_should_process = true;
    current_rgb.show_fn_layer_mask       = false;
    current_rgb.active_fn_layer          = 0;
    current_rgb.gaming_indicator_on      = false;
    current_rgb.srgb_indicator_on        = false;
    current_rgb.rgb_adjusted_in_fn       = false;

    // Apply initial state
    signalrgb_mode_enable();
    indicators[SRGB_IND_IDX].active = false;
#else
    // Initialize RGB rendering state (non-SignalRGB)
    current_rgb.show_fn_layer_mask  = false;
    current_rgb.active_fn_layer     = 0;
    current_rgb.gaming_indicator_on = false;
    current_rgb.rgb_adjusted_in_fn  = false;
#endif

    indicators[GAMING_IND_IDX].active = false;
}

void matrix_scan_user(void) {
#ifdef SIGNALRGB_ENABLE
    // Check for SignalRGB timeout
    if (!srgb_state.timed_out && timer_elapsed32(srgb_state.last_activity) > 300) {
        srgb_state.timed_out = true;
        commit_state_change(false, true);
    }

    // Apply all pending changes atomically
    if (pending.has_pending) {
        // 1. Update SignalRGB mode if needed
        if (pending.update_signalrgb) {
            bool was_processing = current_rgb.signalrgb_should_process;
            bool will_process   = pending.next_rgb.signalrgb_should_process;

            if (was_processing && !will_process) {
                signalrgb_mode_disable();
            } else if (!was_processing && will_process) {
                signalrgb_mode_enable();
            }
        }

        // 2. Update LED mask if needed (before changing current state)
        if (pending.recalc_mask) {
            update_mod_led_mask(pending.next_rgb.active_fn_layer);
        }

        // 3. Update current state (preserving rgb_adjusted_in_fn which is managed immediately)
        bool preserve_rgb_adjusted     = current_rgb.rgb_adjusted_in_fn;
        current_rgb                    = pending.next_rgb;
        current_rgb.rgb_adjusted_in_fn = preserve_rgb_adjusted;

        // 4. Update indicators based on new current state
        indicators[GAMING_IND_IDX].active = current_rgb.gaming_indicator_on;
        indicators[SRGB_IND_IDX].active   = current_rgb.srgb_indicator_on;

        // Update mask for SRGB indicator if it changed
        if (pending.recalc_mask || (indicators[SRGB_IND_IDX].active != mod_led_mask[indicators[SRGB_IND_IDX].index])) {
            mod_led_mask[indicators[SRGB_IND_IDX].index] = indicators[SRGB_IND_IDX].active;
            signalrgb_sync_mask(mod_led_mask);
        }

        // 5. Clear pending
        pending.has_pending = false;
    }
#else
    // Without SignalRGB, just update gaming indicator directly
    // (indicator state is updated in layer_state_set_user)
    indicators[GAMING_IND_IDX].active = current_rgb.gaming_indicator_on;
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_caps_word(keycode, record)) {
        return false;
    }

    if (current_rgb.active_fn_layer != 0 && record->event.pressed) {
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
                if (!current_rgb.rgb_adjusted_in_fn) {
                    // Mark that RGB was adjusted in FN layer
                    current_rgb.rgb_adjusted_in_fn = true;
#ifdef SIGNALRGB_ENABLE
                    // Unblock SignalRGB and commit the state change
                    srgb_state.fn_layer_blocked = false;
                    commit_state_change(false, true);
#endif
                }
                break;
        }
    }

    switch (keycode) {
        case UG_SRGB:
            if (record->event.pressed) {
#ifdef SIGNALRGB_ENABLE
                srgb_state.user_enabled = !srgb_state.user_enabled;

                if (srgb_state.user_enabled) {
                    // When enabling, reset timeout tracking
                    srgb_state.last_activity = timer_read32();
                    srgb_state.timed_out     = false;
                }

                commit_state_change(false, true);
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
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_EQL))));
                }
                return false;
            }
            // Let QMK handle it when SignalRGB is disabled
            return true;
        case UG_VALD:
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_MINS))));
                }
                return false;
            }
            // Let QMK handle it when SignalRGB is disabled
            return true;
        case UG_NEXT:
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_Q))));
                }
                return false;
            }
            // Let QMK handle it when SignalRGB is disabled
            return true;
        case UG_PREV:
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(C(A(G(KC_A))));
                }
                return false;
            }
            // Let QMK handle it when SignalRGB is disabled
            return true;
        case UG_TOGG:
            // RGB toggle should always use QMK handling, not SignalRGB
            // This prevents flickers when toggling RGB on/off
            return true;

#endif
    }
    return true;
}

void leader_start_user(void) {}

void leader_end_user(void) {
    if (leader_sequence_one_key(KC_E)) {
        SEND_STRING("szymek.fogiel@gmail.com");
    }
}

#if defined(VIA_ENABLE) && defined(SIGNALRGB_ENABLE)
extern bool kc_raw_hid_rx(uint8_t src, uint8_t *data, uint8_t length);
extern bool srgb_raw_hid_rx(uint8_t *data, uint8_t length);

bool via_command_user(uint8_t src, uint8_t *data, uint8_t length) {
    switch (data[0]) {
        case GET_QMK_VERSION:
        case GET_PROTOCOL_VERSION:
        case GET_UNIQUE_IDENTIFIER:
        case STREAM_RGB_DATA:
        case SET_SIGNALRGB_MODE_ENABLE:
        case SET_SIGNALRGB_MODE_DISABLE:
        case GET_TOTAL_LEDS:
        case GET_FIRMWARE_TYPE:
            break;
        default:
            return false;
    }

    srgb_state.last_activity = timer_read32();

    // Clear timeout flag when receiving data and commit state change
    if (srgb_state.timed_out) {
        srgb_state.timed_out = false;
        commit_state_change(false, true);
    }

    // Don't process SignalRGB HID messages if keyboard has disabled SignalRGB
    // This prevents SignalRGB app from re-enabling when user toggled it off
    if (calculate_signalrgb_should_process() && srgb_raw_hid_rx(data, length)) {
        return true;
    }
    return false;
}
#endif
