#include QMK_KEYBOARD_H
#include "keychron_common.h"
#ifdef SIGNALRGB_ENABLE
#    include "signalrgb.h"
#endif

#define GAMING_IND_IDX 0
#define SRGB_IND_IDX 1

// --- STATE STORAGE ---
// Clean separation: Immediate control flags vs. Deferred rendering actions

// IMMEDIATE CONTROL STATE (read/write anytime, affects control flow)
static bool    rgb_adjusted_in_fn  = false; // Whether RGB was adjusted while in FN layer
static uint8_t active_fn_layer     = 0;     // Which FN layer is active (0 if none)
static bool    gaming_layer_active = false; // Whether gaming layer is active

#ifdef SIGNALRGB_ENABLE
// SignalRGB control state - immediate flags that affect SignalRGB calculation
typedef struct {
    uint32_t last_activity;    // Last HID activity timestamp
    bool     timed_out;        // Whether SignalRGB has timed out
    bool     user_enabled;     // Whether user has enabled SignalRGB via toggle
    bool     fn_layer_blocked; // Whether FN layer is blocking SignalRGB
} signalrgb_state_t;

// DEFERRED RENDERING ACTIONS (queued, applied atomically in matrix_scan_user)
// This prevents visual flickers by updating LEDs, masks, and indicators in one frame
typedef struct {
    bool has_pending;              // Whether there are pending rendering actions
    bool recalc_mask;              // Recalculate LED mask for current FN layer
    bool signalrgb_was_processing; // SignalRGB state before change (for enable/disable)
    bool signalrgb_will_process;   // SignalRGB state after change (for enable/disable)
} pending_rendering_t;

static signalrgb_state_t   srgb_state = {0};
static pending_rendering_t pending    = {0};
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
// Calculate whether SignalRGB should process LED updates (ALWAYS COMPUTED, NEVER CACHED)
// This is derived from immediate control state and should be called fresh each time
static bool calculate_signalrgb_should_process(void) {
    if (!srgb_state.user_enabled) return false;
    if (srgb_state.timed_out) return false;
    if (srgb_state.fn_layer_blocked) return false;
    return true;
}

// Commit rendering actions to be applied atomically in next matrix_scan
// Call this after updating immediate state flags
static void commit_rendering_update(bool need_mask_recalc) {
    pending.signalrgb_was_processing = pending.signalrgb_will_process; // Previous becomes current
    pending.signalrgb_will_process   = calculate_signalrgb_should_process();
    pending.recalc_mask              = need_mask_recalc;
    pending.has_pending              = true;
}
#endif

void update_mod_led_mask(uint8_t fn_layer) {
    // Clear mask
    for (uint16_t i = 0; i < 256; i++) {
        mod_led_mask[i] = false;
    }

    // If FN layer is active, mask keys with non-transparent keycodes
    if (active_fn_layer != 0) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                uint8_t  led_index = g_led_config.matrix_co[row][col];
                uint16_t keycode   = keymap_key_to_keycode(fn_layer, (keypos_t){col, row});
                if (led_index != NO_LED && keycode != KC_TRNS) {
                    mod_led_mask[led_index] = true;
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

    bool was_fn_active = (active_fn_layer != 0);

    // Determine current FN layer
    uint8_t fn_layer = 0;
    if (win_fn_active)
        fn_layer = WIN_FN;
    else if (mac_fn_active)
        fn_layer = MAC_FN;
    else if (hrdw_fn_active)
        fn_layer = HARDWARE;

    bool is_fn_active = (fn_layer != 0);

    // Update IMMEDIATE state (affects control flow)
    active_fn_layer     = fn_layer;
    gaming_layer_active = gaming_active;

    // Reset RGB adjustment flag when entering FN layer
    if (!was_fn_active && is_fn_active) {
        rgb_adjusted_in_fn = false;
    }

#ifdef SIGNALRGB_ENABLE
    // Update SignalRGB blocking based on FN layer
    srgb_state.fn_layer_blocked = (is_fn_active && !rgb_adjusted_in_fn);

    // Commit rendering update (need mask recalc due to layer change)
    commit_rendering_update(true);
#else
    // Without SignalRGB, just update the mask directly
    update_mod_led_mask(fn_layer);
    indicators[GAMING_IND_IDX].active = gaming_layer_active;
#endif

    return state;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // Show FN layer mask if FN layer is active
    if (active_fn_layer != 0) {
        for (uint8_t i = led_min; i < led_max; i++) {
            if (mod_led_mask[i]) {
                rgb_matrix_set_color(i, 255, 255, 255);
            } else if (!rgb_adjusted_in_fn && !calculate_signalrgb_should_process()) {
                // Only black out if RGB hasn't been adjusted AND SignalRGB isn't processing
                rgb_matrix_set_color(i, 0, 0, 0);
            }
            // Otherwise, let the RGB matrix show (either QMK effects or SignalRGB)
        }
    }

    // Render active indicators
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
    rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
    rgb_matrix_sethsv_noeeprom(156, 191, 255);

#ifdef SIGNALRGB_ENABLE
    // Initialize SignalRGB control state
    srgb_state.last_activity    = timer_read32();
    srgb_state.user_enabled     = true;
    srgb_state.timed_out        = false;
    srgb_state.fn_layer_blocked = false;

    // Enable SignalRGB and set initial indicator state
    signalrgb_mode_enable();
    indicators[SRGB_IND_IDX].active = false;

    // Initialize pending with current state for next update
    pending.signalrgb_will_process = true;
#endif

    // Initialize gaming indicator
    indicators[GAMING_IND_IDX].active = false;
}

void matrix_scan_user(void) {
#ifdef SIGNALRGB_ENABLE
    // Check for SignalRGB timeout (immediate state update)
    if (!srgb_state.timed_out && timer_elapsed32(srgb_state.last_activity) > 300) {
        srgb_state.timed_out = true;
        commit_rendering_update(false);
    }

    // Apply all pending rendering actions atomically (prevents flickers)
    if (pending.has_pending) {
        // 1. Update SignalRGB mode if state changed
        if (pending.signalrgb_was_processing != pending.signalrgb_will_process) {
            if (pending.signalrgb_will_process) {
                signalrgb_mode_enable();
            } else {
                signalrgb_mode_disable();
            }
        }

        // 2. Recalculate LED mask if needed
        if (pending.recalc_mask) {
            update_mod_led_mask(active_fn_layer);
        }

        // 3. Update indicators based on IMMEDIATE state (always fresh)
        indicators[GAMING_IND_IDX].active = gaming_layer_active;

        bool should_process             = calculate_signalrgb_should_process();
        indicators[SRGB_IND_IDX].active = srgb_state.user_enabled && !should_process;

        // Update mask for SRGB indicator if needed
        if (pending.recalc_mask || (indicators[SRGB_IND_IDX].active != mod_led_mask[indicators[SRGB_IND_IDX].index])) {
            mod_led_mask[indicators[SRGB_IND_IDX].index] = indicators[SRGB_IND_IDX].active;
            signalrgb_sync_mask(mod_led_mask);
        }

        // 4. Clear pending flag
        pending.has_pending = false;
    }
#else
    // Without SignalRGB, just update gaming indicator directly
    indicators[GAMING_IND_IDX].active = gaming_layer_active;
#endif
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!process_caps_word(keycode, record)) {
        return false;
    }

    // Track RGB adjustments in FN layer (IMMEDIATE state update)
    if (active_fn_layer != 0 && record->event.pressed) {
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
                    // Mark that RGB was adjusted in FN layer (IMMEDIATE)
                    rgb_adjusted_in_fn = true;
#ifdef SIGNALRGB_ENABLE
                    // Unblock SignalRGB and commit rendering update
                    srgb_state.fn_layer_blocked = false;
                    commit_rendering_update(false);
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

                commit_rendering_update(false);
#endif
            }
            return false;

        case UG_ANIM1:
            if (record->event.pressed) {
                rgb_matrix_mode_noeeprom(RGB_MATRIX_SOLID_COLOR);
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

    // Clear timeout flag when receiving data and commit rendering update
    if (srgb_state.timed_out) {
        srgb_state.timed_out = false;
        commit_rendering_update(false);
    }

    // Don't process SignalRGB HID messages if keyboard has disabled SignalRGB
    // This prevents SignalRGB app from re-enabling when user toggled it off
    if (calculate_signalrgb_should_process() && srgb_raw_hid_rx(data, length)) {
        return true;
    }
    return false;
}
#endif
