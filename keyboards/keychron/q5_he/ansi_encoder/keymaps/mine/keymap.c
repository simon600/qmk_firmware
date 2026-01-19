#include QMK_KEYBOARD_H
#include "keychron_common.h"
#ifdef SIGNALRGB_ENABLE
#    include "signalrgb.h"
#endif

// --- CONSTANTS & DEFINITIONS ---

#define GAMING_IND_IDX 0
#define SRGB_IND_IDX 1
#define INDICATOR_COUNT 2

#define TIMEOUT_MS 300000 // 5 minutes

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

// --- DATA STRUCTURES ---

typedef struct {
    uint8_t r, g, b;
} color_t;

typedef struct {
    uint8_t   index;
    bool      active;
    color_t   color;
} indicator_t;

typedef struct {
    // User Intent / Persistent Settings
    bool user_srgb_enabled;      // Toggled by UG_SRGB

    // System Counters
    uint32_t last_activity_time; // For timeout tracking

    // Derived/Transient State
    bool timeout_active;         // True if inactivity timer expired
    bool is_fn_layer_active;     // True if any FN layer is on
    uint8_t current_fn_layer;    // The actual layer index (or 0)
    bool rgb_adjusted_in_fn;     // True if user messed with RGB settings inside FN layer

    // Active State (What is currently happening)
    bool srgb_active;            // Is SignalRGB currently driving the board?

    // Indicators
    indicator_t indicators[INDICATOR_COUNT];

} keyboard_state_t;

// --- GLOBAL STATE ---

static keyboard_state_t state = {
    .user_srgb_enabled = true,
    .last_activity_time = 0,
    .timeout_active = false,
    .is_fn_layer_active = false,
    .current_fn_layer = 0,
    .rgb_adjusted_in_fn = false,
    .srgb_active = false,
    .indicators = {
        {.index = 14, .active = false, .color = {255, 0, 0}},   // GAMING
        {.index = 36, .active = false, .color = {255, 255, 255}} // SRGB
    }
};

static bool mod_led_mask[256]; // 0 = SignalRGB controlled, 1 = QMK controlled (masked)

// --- KEYMAP ---

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
#endif

// clang-format on

// --- LOGIC ---

// Updates mod_led_mask based on current state (doesn't sync it)
void calculate_mask(bool *mask_out) {
    // Clear mask
    for (uint16_t i = 0; i < 256; i++) {
        mask_out[i] = false;
    }

    // 1. If FN layer active, mask keys that are NOT transparent on that layer
    if (state.is_fn_layer_active && state.current_fn_layer != 0) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                uint16_t keycode = keymap_key_to_keycode(state.current_fn_layer, (keypos_t){col, row});

                if (keycode != KC_TRNS) {
                    uint8_t led_index = g_led_config.matrix_co[row][col];
                    if (led_index != NO_LED) {
                        mask_out[led_index] = true;
                    }
                }
            }
        }
    }

    // 2. Indicators always masked when active
    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        if (state.indicators[i].active) {
            uint8_t led_index = state.indicators[i].index;
            if (led_index != NO_LED) {
                mask_out[led_index] = true;
            }
        }
    }
}

// Determines if SignalRGB SHOULD be processing right now
bool should_process_srgb(void) {
#ifndef SIGNALRGB_ENABLE
    return false;
#endif
    // If not enabled by user, off.
    if (!state.user_srgb_enabled) return false;

    // If timeout, off.
    if (state.timeout_active) return false;

    // If in FN layer and user has NOT adjusted RGB manually inside that layer:
    // Original logic: "if (is_fn_layer_active && !rgb_adjusted_in_fn) return false;"
    // This implies SignalRGB is disabled during FN layer usage to show internal lighting,
    // UNLESS the user touched RGB controls, in which case we assume they want to see what they did?
    // Or maybe it meant: "If I'm in FN layer, I want my key colors (masked). But if I explicitly change RGB, maybe I want to see it?"
    // Let's stick to the original logic:
    if (state.is_fn_layer_active && !state.rgb_adjusted_in_fn) {
        return false;
    }

    return true;
}

// Main State Update Loop (called from matrix_scan_user)
void update_rgb_state_atomic(void) {
    // 1. Update Timeout State
    if (!state.timeout_active && timer_elapsed32(state.last_activity_time) > TIMEOUT_MS) {
        state.timeout_active = true;
    }

    // 2. Update Indicators Logic
    // SRGB Indicator: Active if User Enabled it, but it's currently suspended (e.g. by timeout or layer)
    // "active_timeout" in original code seemed to trigger the indicator logic:
    // "indicators[SRGB_IND_IDX].active = true" was in the "else" block of "force initial state".

    // Let's replicate the "SRGB Indicator is ON when SRGB is TIMED OUT or SUSPENDED but Enabled" logic.
    bool target_srgb_active = should_process_srgb();

    // If User Enabled SRGB, but it is NOT processing (e.g. Timeout), show indicator?
    // Original: "indicators[SRGB_IND_IDX].active = srgb.keyboard_enabled;" when "srgb.was_active && !should_process" (Disable transition)
    // This implies if we disable SignalRGB (due to timeout/fn), we turn ON the indicator if the keyboard is still "enabled" logically.
    // If the user explicitly disabled it (UG_SRGB), indicator is OFF.

    if (state.user_srgb_enabled && !target_srgb_active) {
        state.indicators[SRGB_IND_IDX].active = true;
    } else {
        state.indicators[SRGB_IND_IDX].active = false;
    }

    // Gaming Indicator is updated in layer_state_set_user usually, but we can double check here or just trust the state.
    // (It's already in state).

    // 3. Calculate Target Mask
    bool new_mask[256];
    calculate_mask(new_mask);

    // 4. Check for Mask Differences
    bool mask_changed = false;
    for (int i = 0; i < 256; i++) {
        if (mod_led_mask[i] != new_mask[i]) {
            mask_changed = true;
            break;
        }
    }

    // 5. Apply Changes Atomically (Visual consistency)

    // CASE A: SignalRGB State Change (Enable -> Disable)
    if (state.srgb_active && !target_srgb_active) {
#ifdef SIGNALRGB_ENABLE
        signalrgb_mode_disable();
#endif
        state.srgb_active = false;

        // Update mask locally (for QMK internal rendering if needed)
        for(int i=0; i<256; i++) mod_led_mask[i] = new_mask[i];

        // If we disabled SignalRGB, we might want to sync mask? Not strictly necessary if disabled,
        // but good for consistency if it re-enables.
        // But more importantly, QMK RGB Matrix needs to know which keys to light up (masked keys).
    }
    // CASE B: SignalRGB State Change (Disable -> Enable)
    else if (!state.srgb_active && target_srgb_active) {
        // Update mask FIRST so we don't flash unmasked keys
        if (mask_changed) {
            for(int i=0; i<256; i++) mod_led_mask[i] = new_mask[i];
#ifdef SIGNALRGB_ENABLE
            signalrgb_sync_mask(mod_led_mask);
#endif
            mask_changed = false; // Handled
        } else {
            // Even if not changed, ensure it's synced before enabling?
            // Usually not needed if we track it well, but let's be safe?
            // No, only sync if changed or init.
        }

#ifdef SIGNALRGB_ENABLE
        signalrgb_mode_enable();
#endif
        state.srgb_active = true;
    }
    // CASE C: No State Change, but Mask Change
    else if (mask_changed) {
        for(int i=0; i<256; i++) mod_led_mask[i] = new_mask[i];
#ifdef SIGNALRGB_ENABLE
        // Only sync to host if SignalRGB is active or we want host to know about it for later
        signalrgb_sync_mask(mod_led_mask);
#endif
    }
}

// --- HOOKS ---

layer_state_t layer_state_set_user(layer_state_t state_arg) {
    bool win_fn_active  = layer_state_cmp(state_arg, WIN_FN);
    bool mac_fn_active  = layer_state_cmp(state_arg, MAC_FN);
    bool hrdw_fn_active = layer_state_cmp(state_arg, HARDWARE);
    bool gaming_active  = layer_state_cmp(state_arg, GAMING);

    bool was_fn_layer_active = state.is_fn_layer_active;

    // Determine current FN layer
    uint8_t fn_layer = 0;
    if (win_fn_active) fn_layer = WIN_FN;
    else if (mac_fn_active) fn_layer = MAC_FN;
    else if (hrdw_fn_active) fn_layer = HARDWARE;

    // Update State
    state.current_fn_layer = fn_layer;
    state.is_fn_layer_active = (fn_layer != 0);

    // Reset RGB adjustment flag when entering FN layer
    if (!was_fn_layer_active && state.is_fn_layer_active) {
        state.rgb_adjusted_in_fn = false;
    }

    // Update Gaming Indicator
    state.indicators[GAMING_IND_IDX].active = gaming_active;

    // Reset activity timer on layer change (user interaction)
    state.last_activity_time = timer_read32();
    if (state.timeout_active) state.timeout_active = false;

    // Note: We do NOT call update_mod_led_mask here.
    // We let matrix_scan_user handle the atomic update of mask + srgb state.

    return state_arg;
}

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // If SignalRGB is active, QMK typically doesn't render unless we mask.
    // If SignalRGB is disabled, QMK renders everything.

    // Render Masked Keys (Functional Layer Highlighting)
    if (state.is_fn_layer_active) {
        for (uint8_t i = led_min; i < led_max; i++) {
            if (mod_led_mask[i]) {
                rgb_matrix_set_color(i, 255, 255, 255);
            } else if (!state.rgb_adjusted_in_fn && !state.srgb_active) {
                // If we are in FN layer, and SignalRGB is OFF (expected), and we haven't adjusted RGB:
                // Turn off unmasked keys? (Black out non-functional keys)
                rgb_matrix_set_color(i, 0, 0, 0);
            }
        }
    }

    // Render Indicators
    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        if (state.indicators[i].active) {
            rgb_matrix_set_color(state.indicators[i].index,
                                 state.indicators[i].color.r,
                                 state.indicators[i].color.g,
                                 state.indicators[i].color.b);
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

    state.last_activity_time = timer_read32();
    state.user_srgb_enabled = true;
    state.timeout_active = false;

    // Force initial update logic
#ifdef SIGNALRGB_ENABLE
    signalrgb_mode_enable(); // Default to enabled?
    state.srgb_active = true;
#endif
}

void matrix_scan_user(void) {
    update_rgb_state_atomic();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Reset Activity Timer on any keypress
    if (record->event.pressed) {
        state.last_activity_time = timer_read32();
        if (state.timeout_active) {
            state.timeout_active = false;
            // update_rgb_state_atomic will handle re-enabling in the next scan
        }
    }

    if (!process_caps_word(keycode, record)) {
        return false;
    }

    // Track RGB Adjustments in FN layer
    if (state.is_fn_layer_active && record->event.pressed) {
        switch (keycode) {
            case UG_TOGG: case UG_NEXT: case UG_PREV: case UG_VALU: case UG_VALD:
            case UG_HUEU: case UG_HUED: case UG_SATU: case UG_SATD: case UG_SPDU:
            case UG_SPDD: case UG_SRGB: case UG_ANIM1: case UG_ANIM2: case UG_ANIM3:
                state.rgb_adjusted_in_fn = true;
                break;
        }
    }

    switch (keycode) {
        case UG_SRGB:
            if (record->event.pressed) {
                state.user_srgb_enabled = !state.user_srgb_enabled;
                // update_rgb_state_atomic will handle the transition
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
        // SignalRGB Passthrough keys
        case UG_VALU:
            if (state.srgb_active && !state.timeout_active) {
                if (record->event.pressed) tap_code16(C(A(G(KC_EQL))));
                return false;
            }
            break;
        case UG_VALD:
            if (state.srgb_active && !state.timeout_active) {
                if (record->event.pressed) tap_code16(C(A(G(KC_MINS))));
                return false;
            }
            break;
        case UG_NEXT:
            if (state.srgb_active && !state.timeout_active) {
                if (record->event.pressed) tap_code16(C(A(G(KC_Q))));
                return false;
            }
            break;
        case UG_PREV:
            if (state.srgb_active && !state.timeout_active) {
                if (record->event.pressed) tap_code16(C(A(G(KC_A))));
                return false;
            }
            break;
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
    // Intercept SignalRGB commands to update activity/timeout state
    switch (data[0]) {
        case GET_QMK_VERSION:
        case GET_PROTOCOL_VERSION:
        case GET_UNIQUE_IDENTIFIER:
        case STREAM_RGB_DATA:
        case SET_SIGNALRGB_MODE_ENABLE:
        case SET_SIGNALRGB_MODE_DISABLE:
        case GET_TOTAL_LEDS:
        case GET_FIRMWARE_TYPE:
             // Reset timeout if SignalRGB is talking to us
             state.last_activity_time = timer_read32();
             if (state.timeout_active) state.timeout_active = false;
             break;
    }

    // Only allow SignalRGB raw HID if we actually want it enabled.
    // If user disabled it (UG_SRGB), block messages so app doesn't force re-enable.
    if (state.user_srgb_enabled && srgb_raw_hid_rx(data, length)) {
        return true;
    }
    return false;
}
#endif
