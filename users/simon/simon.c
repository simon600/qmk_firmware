#include "simon.h"
#include "keychron_common.h"
#include "print.h"

// --- STATE STORAGE ---
// Clean separation: Immediate control flags vs. Deferred rendering actions

// IMMEDIATE CONTROL STATE (read/write anytime, affects control flow)
static bool    rgb_adjusted_in_fn = false; // Whether RGB was adjusted while in FN layer
static uint8_t active_fn_layer    = 0;     // Which FN layer is active (0 if none)

// Inactivity dimming state (5-minute timeout)
static uint32_t last_activity_time = 0;     // Last keyboard activity timestamp
static bool     is_dimmed          = false; // Whether brightness has been dimmed
static uint8_t  saved_brightness   = 0;     // Original brightness before dimming

#ifdef SIGNALRGB_ENABLE
// SignalRGB control state - immediate flags that affect SignalRGB calculation
static signalrgb_state_t srgb_state = {0};
#endif

// --- COMPILE-TIME INDICATOR REGISTRY ---
// Initialize with defaults: all LED indices to 255 (disabled), inactive, black color
// Keyboard-specific mappings are applied via KEYBOARD_LED_MAP macro
indicator_t indicator_library[INDICATOR_COUNT] = {[0 ... INDICATOR_COUNT - 1] = {.led_index = 255, .active = false, .color = {0, 0, 0}},
#ifdef KEYBOARD_LED_MAP
                                                  KEYBOARD_LED_MAP
#endif
};

// LED mask for FN layer rendering
static bool mod_led_mask[RGB_MATRIX_LED_COUNT];

// --- STATE MANAGEMENT FUNCTIONS ---

#ifdef SIGNALRGB_ENABLE
// Calculate whether SignalRGB should process LED updates (ALWAYS COMPUTED, NEVER CACHED)
// This is derived from immediate control state and should be called fresh each time
static bool calculate_signalrgb_should_process(void) {
    if (!srgb_state.user_enabled) return false;
    if (srgb_state.timed_out) return false;
    return true;
}
#endif

// Register activity and restore brightness if dimmed
static void register_activity(void) {
    last_activity_time = timer_read32();

    // Restore brightness only if we actually saved a valid brightness value
    if (is_dimmed && saved_brightness > 0) {
        rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), saved_brightness);
        is_dimmed = false;
    }
}

// Update the LED mask based on the active FN layer
// This is called whenever the FN layer changes
static void update_mod_led_mask(uint8_t fn_layer) {
    // Clear the mask
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        mod_led_mask[i] = false;
    }

    // If no FN layer is active, nothing to mask
    if (fn_layer == 0) {
        return;
    }

    // Iterate through all key positions and mark LEDs that have non-transparent keycodes
    if (fn_layer != 0) {
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                uint8_t  led_index = g_led_config.matrix_co[row][col];
                uint16_t keycode   = keymap_key_to_keycode(active_fn_layer, (keypos_t){col, row});
                if (led_index != NO_LED && keycode != KC_TRNS) {
                    mod_led_mask[led_index] = true;
                }
            }
        }
    }
}

// --- PUBLIC API FUNCTIONS ---

// State access functions
indicator_t *get_indicators(void) {
    return indicator_library;
}

uint8_t get_active_fn_layer(void) {
    return active_fn_layer;
}

// --- QMK CALLBACK IMPLEMENTATIONS ---

void keyboard_post_init_shared(void) {
    // Explicitly initialize state variables (BSS may not be zeroed properly)
    active_fn_layer    = 0;
    rgb_adjusted_in_fn = false;

    // Initialize inactivity dimming
    last_activity_time = timer_read32();
    is_dimmed          = false;
    saved_brightness   = 0;

#ifdef SIGNALRGB_ENABLE
    // Initialize SignalRGB control state
    srgb_state.user_enabled  = true;
    srgb_state.timed_out     = false;
    srgb_state.last_activity = timer_read32();

    // Enable SignalRGB mode (required for RGB matrix to render SignalRGB colors)
    signalrgb_mode_enable();
#endif
}

void matrix_scan_shared(void) {
    // Check for inactivity timeout
    uint32_t elapsed = timer_elapsed32(last_activity_time);
    if (!is_dimmed && elapsed > INACTIVITY_TIMEOUT_MS) {
        // Save current brightness and dim to minimum (val=16, just above off)
        saved_brightness = rgb_matrix_get_val();
        if (saved_brightness > 1) { // Only dim if brightness is above minimum
            rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), 16);
            is_dimmed = true;
        }
    }

#ifdef SIGNALRGB_ENABLE
    // Check for SignalRGB timeout (immediate state update)
    // Only check timeout if we've received data before (last_activity != 0)
    if (!srgb_state.timed_out && srgb_state.last_activity != 0 && timer_elapsed32(srgb_state.last_activity) > 300) {
        srgb_state.timed_out = true;
        signalrgb_mode_disable();
    }

    bool should_process = calculate_signalrgb_should_process();
#    ifdef SIGNALRGB_ENABLE
    indicator_library[INDICATOR_SIGNALRGB].active = srgb_state.user_enabled && !should_process;
#    endif
#endif
}

bool process_record_shared(uint16_t keycode, keyrecord_t *record) {
    if (!process_caps_word(keycode, record)) {
        return false;
    }

    // Track activity for inactivity dimming
    if (record->event.pressed) {
        register_activity();
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
                rgb_adjusted_in_fn = true;
                break;
        }
    }

    switch (keycode) {
        case LT(1, KC_NO):
        case LT(3, KC_NO):
            if (record->tap.count > 0) {
                if (!record->event.pressed) { // Trigger on release for better accuracy
                    leader_start();
                }
                return false; // Don't send KC_NO
            }
            break;
        case UG_SRGB:
            if (record->event.pressed) {
#ifdef SIGNALRGB_ENABLE
                srgb_state.user_enabled = !srgb_state.user_enabled;

                if (srgb_state.user_enabled) {
                    // When enabling, reset timeout tracking and enable RGB matrix mode
                    srgb_state.last_activity = timer_read32();
                    srgb_state.timed_out     = false;
                    signalrgb_mode_enable(); // Enable RGB matrix SignalRGB mode
                } else {
                    signalrgb_mode_disable();
                }
#endif
            }
            return false;

        case UG_ANIM1:
            if (record->event.pressed) {
                rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
                rgb_matrix_sethsv(156, 191, 255);
            }
            return false;

        case UG_ANIM2:
        case UG_ANIM3:
            return false;

        case M_NW:
            if (record->event.pressed) {
                tap_code16(C(KC_RGHT));
                tap_code16(C(KC_RGHT));
                tap_code16(C(KC_LEFT));
            }
            return false;

        case M_NM:
            if (record->event.pressed) {
                tap_code16(A(KC_RGHT));
                tap_code16(A(KC_RGHT));
                tap_code16(A(KC_LEFT));
            }
            return false;

#ifdef SIGNALRGB_ENABLE
        case UG_VALU:
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(S(C(A(G(KC_EQL)))));
                }
                return false;
            }
            // Let QMK handle it when SignalRGB is disabled
            return true;

        case UG_VALD:
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(S(C(A(G(KC_MINS)))));
                }
                return false;
            }
            // Let QMK handle it when SignalRGB is disabled
            return true;

        case UG_NEXT:
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(S(C(A(G(KC_Q)))));
                }
                return false;
            }
            // Let QMK handle it when SignalRGB is disabled
            return true;

        case UG_PREV:
            // Check real-time state (not cached) so first press after unblocking works
            if (calculate_signalrgb_should_process() && !srgb_state.timed_out) {
                if (record->event.pressed) {
                    tap_code16(S(C(A(G(KC_A)))));
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

layer_state_t layer_state_set_shared(layer_state_t state) {
    // Note: This function should be called from the keymap's layer_state_set_user
    // The keymap needs to determine which FN layer is active and pass that info
    // This is a limitation since layer enums are keyboard-specific

    // This function primarily updates the LED mask when layer state changes
    // The actual layer tracking should be done in the keymap

    return state;
}

bool rgb_matrix_indicators_advanced_shared(uint8_t led_min, uint8_t led_max) {
    if (is_dimmed) {
        return true;
    }
#ifdef SIGNALRGB_ENABLE
    // Apply SignalRGB colors first if processing (base layer)
    if (calculate_signalrgb_should_process()) {
        for (uint8_t i = led_min; i < led_max; i++) {
            rgb_led_t color = signalrgb_get_color(i);
            rgb_matrix_set_color(i, color.r, color.g, color.b);
        }
    }
#endif

    // Show FN layer mask if FN layer is active (override layer)
    if (active_fn_layer != 0) {
        for (uint8_t i = led_min; i < led_max; i++) {
            if (i < RGB_MATRIX_LED_COUNT) {
                if (mod_led_mask[i]) {
                    rgb_matrix_set_color(i, 255, 255, 255);
                } else if (!rgb_adjusted_in_fn) {
                    // Only black out if RGB hasn't been adjusted
                    rgb_matrix_set_color(i, 0, 0, 0);
                }
                // Otherwise, let the RGB matrix show (either QMK effects or SignalRGB)
            }
        }
    }

    // Render active indicators (top layer, always visible)
    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        if (indicator_library[i].active && indicator_library[i].led_index != 255) {
            rgb_matrix_set_color(indicator_library[i].led_index, indicator_library[i].color.r, indicator_library[i].color.g, indicator_library[i].color.b);
        }
    }

    return true;
}

bool get_permissive_hold_shared(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case LCTL_T(KC_ESC):
        case GUI_T(KC_ESC):
            return true;
        default:
            return false;
    }
}

void leader_start_shared(void) {
    indicator_library[INDICATOR_LEADER].active = true;
}

void leader_end_shared(void) {
    indicator_library[INDICATOR_LEADER].active = false;

    if (leader_sequence_one_key(KC_E)) {
        SEND_STRING("szymek.fogiel@gmail.com");
    }
}

#if defined(VIA_ENABLE) && defined(SIGNALRGB_ENABLE)
extern bool kc_raw_hid_rx(uint8_t src, uint8_t *data, uint8_t length);
extern bool srgb_raw_hid_rx(uint8_t *data, uint8_t length);

bool via_command_shared(uint8_t src, uint8_t *data, uint8_t length) {
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

    // Track USB activity for inactivity dimming
    register_activity();

    srgb_state.last_activity = timer_read32();

    // Clear timeout flag when receiving data
    if (srgb_state.timed_out) {
        srgb_state.timed_out = false;
    }

    // Process SignalRGB HID messages if user hasn't disabled it
    if (calculate_signalrgb_should_process() && srgb_raw_hid_rx(data, length)) {
        return true;
    }

    return false;
}
#endif

// Helper function for keymaps to set the active FN layer
// This should be called from the keymap's layer_state_set_user
void set_active_fn_layer(uint8_t layer) {
    bool was_fn_active = (active_fn_layer != 0);

    active_fn_layer = layer;

    // Reset RGB adjustment flag when entering FN layer
    if (!was_fn_active && active_fn_layer) {
        rgb_adjusted_in_fn = false;
    }

    update_mod_led_mask(active_fn_layer);
}

bool dynamic_macro_record_start_shared(int8_t direction) {
    if (direction == 1) {
        indicator_library[INDICATOR_MACRO_REC_1].active = true;
        indicator_library[INDICATOR_MACRO_REC_2].active = false;
    } else if (direction == -1) {
        indicator_library[INDICATOR_MACRO_REC_1].active = false;
        indicator_library[INDICATOR_MACRO_REC_2].active = true;
    }
    return true;
}

bool dynamic_macro_record_end_shared(int8_t direction) {
    indicator_library[INDICATOR_MACRO_REC_1].active = false;
    indicator_library[INDICATOR_MACRO_REC_2].active = false;
    return true;
}
