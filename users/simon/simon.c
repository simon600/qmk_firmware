#include "simon.h"
#ifdef OPENRGB_ENABLE
#    include "openrgb.h"
#endif
#ifdef HOST_PROTOCOL_ENABLE
#    include "host_protocol.h"
#endif
#include "keychron_common.h"
#include "print.h"
#ifdef LK_WIRELESS_ENABLE
#    include "transport.h"
#endif
#ifdef KEYCHRON_RGB_ENABLE
#    include "keychron_rgb_type.h"
#endif

// --- COMBO DEFINITIONS ---
/*const uint16_t PROGMEM op_combo[]   = {KC_O, KC_P, COMBO_END};
const uint16_t PROGMEM io_combo[]   = {KC_I, KC_O, COMBO_END};
combo_t                key_combos[] = {
    [OP_BSPC] = COMBO(op_combo, KC_BSPC),
    [IO_DEL]  = COMBO(io_combo, KC_DEL),
};*/

// --- STATE STORAGE ---
// Clean separation: Immediate control flags vs. Deferred rendering actions

// IMMEDIATE CONTROL STATE (read/write anytime, affects control flow)
static bool    rgb_adjusted_in_fn = false; // Whether RGB was adjusted while in FN layer
static uint8_t active_fn_layer    = 0;     // Which FN layer is active (0 if none)

// Inactivity dimming state (5-minute timeout)
static dimming_state_t dimming_state;
static bool            suspended = false; // Suspend state tracker

static user_config_t user_config;

// Auto-hide: indicators are fully visible for INDICATOR_SHOW_MS after this
// timestamp, then fade out over INDICATOR_FADE_MS
static uint32_t indicators_shown_at;

#define MIN_SAFE_BRIGHTNESS 50
#define MIN_INDICATOR_BRIGHTNESS 16
// Default brightness of the other keys while an FN layer is shown: 64/255 =
// 25%, as openrgb-daemon's `dim = 0.25`.
// The live value (layer_dim) is a host setting; OpenRGB profiles override it.
#define LAYER_DIM_DEFAULT 64
#ifndef RGB_MATRIX_VAL_STEP
#    define RGB_MATRIX_VAL_STEP 8
#endif

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
// External RGB control state - immediate flags that affect external RGB calculation
static ext_rgb_state_t ext_rgb_state = {0};
#endif

// --- COMPILE-TIME INDICATOR REGISTRY ---
// Initialize with defaults: all LED indices to 255 (disabled), inactive, state 0
// Keyboard-specific mappings are applied via KEYBOARD_LED_MAP macro
indicator_t indicator_library[INDICATOR_COUNT] = {[0 ... INDICATOR_COUNT - 1] = {.led_index = 255, .active = false, .state = 0},
                                                  [INDICATOR_MACRO_REC]       = {.led_index = 255, .active = false, .always_on = true},
                                                  [INDICATOR_LEADER]          = {.led_index = 255, .active = false, .always_on = true},
#ifdef CAPS_LOCK_INDEX
                                                  [INDICATOR_CAPS_LOCK] = {.led_index = CAPS_LOCK_INDEX, .active = false},
#endif
#ifdef NUM_LOCK_INDEX
                                                  [INDICATOR_NUM_LOCK] = {.led_index = NUM_LOCK_INDEX, .active = false},
#endif
#ifdef KEYBOARD_LED_MAP
                                                  KEYBOARD_LED_MAP
#endif
};

// Default colour per indicator state; keyboards add theirs via KEYBOARD_INDICATOR_COLORS.
// The live table is host-editable and, on host-protocol keyboards, persisted.
static const rgb_led_t indicator_default_colors[INDICATOR_COUNT][INDICATOR_MAX_STATES] = {
    [INDICATOR_MACRO_REC] = {IND_COLOR_RED},
    [INDICATOR_LEADER]    = {IND_COLOR_PEACH},
    [INDICATOR_CAPS_LOCK] = {IND_COLOR_PEACH},
    [INDICATOR_NUM_LOCK]  = {IND_COLOR_PEACH},
    [INDICATOR_CAPS_WORD] = {IND_COLOR_PEACH},
    // Colour-only entry (no LED of its own): the keys bound on a held FN
    // layer. OpenRGB profiles can override it while active ([keyboard_indicators]).
    [INDICATOR_FN_LAYER]  = {{255, 255, 255}},
#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
    [INDICATOR_SIGNALRGB] = {IND_COLOR_LAVENDER},
#endif
#ifdef KEYBOARD_INDICATOR_COLORS
    KEYBOARD_INDICATOR_COLORS
#endif
};

#define X(id, wire, states) [id] = wire,
static const uint8_t indicator_wire_ids[INDICATOR_COUNT] = {SHARED_INDICATOR_IDS KEYBOARD_INDICATOR_IDS};
#undef X
#define X(id, wire, states) [id] = states,
static const uint8_t indicator_state_counts[INDICATOR_COUNT] = {SHARED_INDICATOR_IDS KEYBOARD_INDICATOR_IDS};
#undef X

static rgb_led_t indicator_colors[INDICATOR_COUNT][INDICATOR_MAX_STATES];
static uint8_t   layer_dim = LAYER_DIM_DEFAULT;

#if (EECONFIG_USER_DATA_SIZE) > 0
_Static_assert(sizeof(user_data_t) <= (EECONFIG_USER_DATA_SIZE), "user_data_t does not fit EECONFIG_USER_DATA_SIZE");
#endif

static rgb_led_t indicator_current_color(uint8_t id) {
    return indicator_colors[id][indicator_library[id].state];
}

// LED mask for FN layer rendering
static bool mod_led_mask[RGB_MATRIX_LED_COUNT];
// Keys of that mask that do nothing under a host's lighting (see inert_under_host)
static bool host_inert_mask[RGB_MATRIX_LED_COUNT];

// --- STATE MANAGEMENT FUNCTIONS ---

#ifdef SIGNALRGB_ENABLE
// True while SignalRGB is actively streaming, in which case the RGB keys are
// forwarded to the host app as hotkeys instead of being handled locally.
static bool signalrgb_is_streaming(void) {
    return !ext_rgb_state.timed_out && ext_rgb_state.active_source == EXT_RGB_SIGNALRGB;
}
#endif

// Register activity and restore brightness if dimmed
static void register_activity(void) {
    dimming_state.last_activity_time = timer_read32();

    // Restore brightness only if we actually saved a valid brightness value
    if (dimming_state.is_dimmed) {
        if (dimming_state.saved_brightness > 0) {
            rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), dimming_state.saved_brightness);
        }
        rgb_matrix_mode_noeeprom(dimming_state.saved_mode);

        dimming_state.is_dimmed = false;
    }
}

// Persist the live brightness alone. rgb_matrix_*_val() would flush the whole
// config, mode included, and saving the direct mode a host is streaming in
// would boot into a blank frame buffer.
static void save_brightness(void) {
    rgb_config_t stored;
    eeconfig_read_rgb_matrix(&stored);
    if (stored.hsv.v != rgb_matrix_config.hsv.v) {
        stored.hsv.v = rgb_matrix_config.hsv.v;
        eeconfig_update_rgb_matrix(&stored);
    }
}

static bool in_direct_mode(void) {
#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
    return rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_SIGNALRGB;
#else
    return false;
#endif
}

// A host's lighting is on the keys: direct mode (OpenRGB or SignalRGB) or an
// effect OpenRGB set. The keyboard's own lighting controls stay out of its way.
static bool host_owns_lighting(void) {
#ifdef OPENRGB_ENABLE
    if (openrgb_owns_effect()) return true;
#endif
    return in_direct_mode();
}

// Same for the on/off state, toggled while a host streams
static void save_rgb_enable(void) {
    rgb_config_t stored;
    eeconfig_read_rgb_matrix(&stored);
    if (stored.enable != rgb_matrix_config.enable) {
        stored.enable = rgb_matrix_config.enable;
        eeconfig_update_rgb_matrix(&stored);
    }
}

// While OpenRGB drives the keys (direct mode, or an effect it set) and a host
// listens (kbd-daemon), the profile, brightness and on/off keys drive the host's
// lighting (openrgb-daemon) instead. True when the key went to the host; without
// one it stays local, so UG_NEXT always gets out of a mode whose host is gone.
static bool rgb_key_to_host(uint16_t keycode, keyrecord_t *record) {
#ifdef HOST_PROTOCOL_ENABLE
    if (!host_owns_lighting() || !host_listening()) return false;
#    ifdef SIGNALRGB_ENABLE
    if (signalrgb_is_streaming()) return false;
#    endif
    // Lighting switched off here only comes back on here
    if (keycode == UG_TOGG && !rgb_matrix_is_enabled()) return false;
    if (record->event.pressed) {
        switch (keycode) {
            case UG_NEXT:
                host_notify_rgb_key(HOST_RGB_KEY_PROFILE_NEXT);
                break;
            case UG_PREV:
                host_notify_rgb_key(HOST_RGB_KEY_PROFILE_PREV);
                break;
            case UG_VALU:
                host_notify_rgb_key(HOST_RGB_KEY_BRIGHTNESS_UP);
                break;
            case UG_VALD:
                host_notify_rgb_key(HOST_RGB_KEY_BRIGHTNESS_DOWN);
                break;
            case UG_TOGG:
                host_notify_rgb_key(HOST_RGB_KEY_TOGGLE);
                break;
        }
    }
    return true;
#else
    return false;
#endif
}

#ifdef HOST_PROTOCOL_ENABLE
// USB came back (wake, KVM switch) and no host spoke up: lighting OpenRGB left
// behind goes back to the keyboard's own saved effect, so another computer
// gets a keyboard it can control. SignalRGB streaming there keeps its own.
bool host_handback_user(void) {
#    ifdef SIGNALRGB_ENABLE
    if (signalrgb_is_streaming()) return false;
#    endif
    if (!host_owns_lighting()) return false;
    openrgb_mode_disable();
    return true;
}
#endif

// A host command changed something visible (gaming mode, colours, settings):
// counts as activity, so a dimmed keyboard wakes up and draws its indicators
void host_activity(void) {
    register_activity();
}

// (Re)start the auto-hide show window
static void indicators_wake(void) {
    indicators_shown_at = timer_read32();
}

// Wake when an auto-hide indicator turns on/off or changes color (lock keys,
// gaming profile switch, host colour edit). Leader/macro indicators are always
// shown anyway.
static void check_indicator_changes(void) {
    static bool      prev_active[INDICATOR_COUNT];
    static rgb_led_t prev_color[INDICATOR_COUNT];

    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        indicator_t *ind = &indicator_library[i];
        if (ind->always_on) continue;
        rgb_led_t color = indicator_current_color(i);
        if (ind->active != prev_active[i] || memcmp(&color, &prev_color[i], sizeof(rgb_led_t)) != 0) {
            prev_active[i] = ind->active;
            prev_color[i]  = color;
            indicators_wake();
        }
    }
}

// 255 = fully shown, 0 = hidden, in between = fading out
static uint8_t indicator_visibility(void) {
    // Always visible while a momentary layer is held; the window restarts on release
    if (user_config.indicators_always_on || active_fn_layer != 0) {
        return 255;
    }
    uint32_t elapsed = timer_elapsed32(indicators_shown_at);
    if (elapsed < INDICATOR_SHOW_MS) {
        return 255;
    }
    elapsed -= INDICATOR_SHOW_MS;
    if (elapsed >= INDICATOR_FADE_MS) {
        return 0;
    }
    return 255 - elapsed * 255 / INDICATOR_FADE_MS;
}

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
extern rgb_led_t srgb_led_buffer[];
#endif

// The color the current effect drew under an indicator, so it can fade into
// it. There's no read-back API, so this only covers modes where it can be
// computed; anything else returns false and the indicator just switches off.
static bool get_base_color(uint8_t led, rgb_led_t *out) {
    if (user_config.bg_blackout_mode) {
        *out = (rgb_led_t){0, 0, 0};
        return true;
    }
    switch (rgb_matrix_get_mode()) {
        case RGB_MATRIX_SOLID_COLOR:
            *out = hsv_to_rgb(rgb_matrix_get_hsv());
            return true;
#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
        case RGB_MATRIX_CUSTOM_SIGNALRGB: {
            // Same scaling as the SIGNALRGB effect in rgb_matrix_user.inc
            uint8_t val = rgb_matrix_get_val();
            out->r      = (uint16_t)srgb_led_buffer[led].r * val / 255;
            out->g      = (uint16_t)srgb_led_buffer[led].g * val / 255;
            out->b      = (uint16_t)srgb_led_buffer[led].b * val / 255;
            return true;
        }
#endif
        default:
            return false;
    }
}

static uint8_t blend(uint8_t from, uint8_t to, uint8_t amount) {
    return from + ((int16_t)to - from) * amount / 255;
}

// Hue, saturation and speed: under a host's lighting they'd only change, and
// save to EEPROM, what the host set (in direct mode not even visibly)
static bool inert_under_host(uint16_t keycode) {
    switch (keycode) {
        case UG_HUEU:
        case UG_HUED:
        case UG_SATU:
        case UG_SATD:
        case UG_SPDU:
        case UG_SPDD:
            return true;
        default:
            return false;
    }
}

// Update the LED mask based on the active FN layer
// This is called whenever the FN layer changes
static void update_mod_led_mask(uint8_t fn_layer) {
    // Clear the mask
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        mod_led_mask[i]      = false;
        host_inert_mask[i] = false;
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
                    mod_led_mask[led_index]      = true;
                    host_inert_mask[led_index] = inert_under_host(keycode);
                }
            }
        }
    }
}

// --- PERSISTENCE ---

static void user_config_load(void) {
#if (EECONFIG_USER_DATA_SIZE) > 0
    eeconfig_read_user_datablock(&user_config, offsetof(user_data_t, config), sizeof(user_config));
#else
    user_config.raw = eeconfig_read_user();
#endif
}

void user_config_save(void) {
#if (EECONFIG_USER_DATA_SIZE) > 0
    eeconfig_update_user_datablock(&user_config, offsetof(user_data_t, config), sizeof(user_config));
#else
    eeconfig_update_user(user_config.raw);
#endif
}

static void layer_dim_load(void) {
#if (EECONFIG_USER_DATA_SIZE) > 0
    eeconfig_read_user_datablock(&layer_dim, offsetof(user_data_t, layer_dim), sizeof(layer_dim));
#else
    layer_dim = LAYER_DIM_DEFAULT;
#endif
}

static void indicator_colors_load(void) {
#if (EECONFIG_USER_DATA_SIZE) > 0
    eeconfig_read_user_datablock(indicator_colors, offsetof(user_data_t, indicator_colors), sizeof(indicator_colors));
#else
    memcpy(indicator_colors, indicator_default_colors, sizeof(indicator_colors));
#endif
}

static void indicator_color_save(uint8_t id, uint8_t state) {
#if (EECONFIG_USER_DATA_SIZE) > 0
    uint32_t offset = offsetof(user_data_t, indicator_colors) + (id * INDICATOR_MAX_STATES + state) * sizeof(rgb_led_t);
    eeconfig_update_user_datablock(&indicator_colors[id][state], offset, sizeof(rgb_led_t));
#endif
}

// --- PUBLIC API FUNCTIONS ---

void eeconfig_init_user(void) {
#if (EECONFIG_USER_DATA_SIZE) > 0
    // Stamps the datablock version; a no-op rewrite after a full EEPROM reset
    eeconfig_init_user_datablock();
#endif
    user_config.version              = USER_CONFIG_VERSION;
    user_config.indicator_brightness = 255;
    user_config.bg_blackout_mode     = false;
    user_config.indicators_always_on = false;
    user_config_save();
    indicator_colors_reset(true);
    layer_dim_set(LAYER_DIM_DEFAULT, true);
}

uint8_t layer_dim_get(void) {
    return layer_dim;
}

void layer_dim_set(uint8_t dim, bool persist) {
    layer_dim = dim;
#if (EECONFIG_USER_DATA_SIZE) > 0
    if (persist) eeconfig_update_user_datablock(&layer_dim, offsetof(user_data_t, layer_dim), sizeof(layer_dim));
#endif
}

user_config_t *get_user_config(void) {
    return &user_config;
}

void indicators_set_always_on(bool always_on, bool persist) {
    user_config.indicators_always_on = always_on;
    if (persist) user_config_save();
    indicators_wake();
}

uint8_t indicator_wire_id(uint8_t id) {
    return indicator_wire_ids[id];
}

uint8_t indicator_state_count(uint8_t id) {
    return indicator_state_counts[id];
}

rgb_led_t indicator_color_get(uint8_t id, uint8_t state) {
    return indicator_colors[id][state];
}

void indicator_color_set(uint8_t id, uint8_t state, rgb_led_t color, bool persist) {
    if (id >= INDICATOR_COUNT || state >= indicator_state_counts[id]) return;
    indicator_colors[id][state] = color;
    if (persist) indicator_color_save(id, state);
}

// Back to the saved look (colours and layer dim), dropping unsaved host overrides
void indicator_colors_reload(void) {
    indicator_colors_load();
    layer_dim_load();
}

void indicator_colors_reset(bool persist) {
    memcpy(indicator_colors, indicator_default_colors, sizeof(indicator_colors));
#if (EECONFIG_USER_DATA_SIZE) > 0
    if (persist) eeconfig_update_user_datablock(indicator_colors, offsetof(user_data_t, indicator_colors), sizeof(indicator_colors));
#endif
}

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
    dimming_state.last_activity_time = timer_read32();
    dimming_state.is_dimmed          = false;
    dimming_state.saved_brightness   = 0;
    dimming_state.saved_mode         = RGB_MATRIX_SOLID_COLOR;

    // Load user config from EEPROM (with version check). Switching a keyboard
    // to the datablock leaves an invalid version stamp behind, so it starts
    // from defaults once.
#if (EECONFIG_USER_DATA_SIZE) > 0
    bool stored_valid = eeconfig_is_user_datablock_valid();
#else
    bool stored_valid = true;
#endif
    if (stored_valid) {
        user_config_load();
    }
    if (!stored_valid || user_config.version != USER_CONFIG_VERSION) {
        eeconfig_init_user();
    }
    indicator_colors_load();
    layer_dim_load();

    // Show indicators briefly on startup
    indicators_wake();

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
    // No source claim and no direct-mode switch here: active_source means "who is
    // currently driving the LEDs", which only real HID traffic can establish, and
    // the direct buffer is empty until a host streams into it. Claiming SignalRGB at
    // boot left the brightness keys forwarding hotkeys to a host app that wasn't
    // running, with no local brightness control.
    ext_rgb_state.timed_out     = false;
    ext_rgb_state.last_activity = 0;
    ext_rgb_state.active_source = EXT_RGB_NONE;
#endif

#ifdef KEYCHRON_RGB_ENABLE
    // Caps/num lock are drawn by our own indicators (with auto-hide), so keep
    // Keychron's always-on versions off. Keychron Launcher can still re-enable
    // and persist them via HID until the next boot.
    extern os_indicator_config_t os_ind_cfg;
    os_ind_cfg.disable.caps_lock = true;
    os_ind_cfg.disable.num_lock  = true;
#endif
}

#ifdef LK_WIRELESS_ENABLE
// Keychron re-inits the LED driver on every transport switch, which reloads the
// RGB config from EEPROM and drops whatever was set without saving: OpenRGB's
// direct mode, the brightness keys, the inactivity dim. Put the live config
// back; the direct-mode frame buffer survives the re-init, so OpenRGB's colours
// return without the host having to notice anything.
static void restore_rgb_after_transport_change(void) {
    static transport_t  last_transport = TRANSPORT_NONE;
    static rgb_config_t live_config;
    transport_t         transport = get_transport();

    if (transport != last_transport && last_transport != TRANSPORT_NONE) {
        rgb_matrix_config = live_config;
        if (rgb_matrix_config.enable) {
            rgb_matrix_mode_noeeprom(rgb_matrix_config.mode);
        }
    }
    last_transport = transport;
    live_config    = rgb_matrix_config;
}
#endif

void matrix_scan_shared(void) {
    if (suspended) return;

#ifdef LK_WIRELESS_ENABLE
    restore_rgb_after_transport_change();
#endif

    led_t host_leds                               = host_keyboard_led_state();
    indicator_library[INDICATOR_CAPS_LOCK].active = host_leds.caps_lock;
    indicator_library[INDICATOR_NUM_LOCK].active  = host_leds.num_lock;
    indicator_library[INDICATOR_CAPS_WORD].active = is_caps_word_on();
    check_indicator_changes();

    // Check for inactivity timeout
    uint32_t elapsed = timer_elapsed32(dimming_state.last_activity_time);
    if (!dimming_state.is_dimmed && elapsed > INACTIVITY_TIMEOUT_MS) {
        // Save current brightness and dim to minimum (val=16, just above off)
        dimming_state.saved_brightness = rgb_matrix_get_val();
        dimming_state.saved_mode       = rgb_matrix_get_mode();

        dimming_state.is_dimmed = true;
        rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), 1);
    }

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
    // SignalRGB streams continuously, so silence means its host app is gone:
    // fall back to the EEPROM effect. OpenRGB direct is deliberately not
    // timed out - a hardware/static mode set by OpenRGB must persist through
    // sleep and KVM switches, and a frozen last direct frame is acceptable.
    if (!ext_rgb_state.timed_out && ext_rgb_state.active_source == EXT_RGB_SIGNALRGB &&
        rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_SIGNALRGB &&
        timer_elapsed32(ext_rgb_state.last_activity) > 500) {
        ext_rgb_state.timed_out = true;
#    ifdef SIGNALRGB_ENABLE
        signalrgb_mode_disable();
#    endif
    }
#endif

#ifdef HOST_PROTOCOL_ENABLE
    host_protocol_task();
#endif
}

void update_led_index(uint8_t id, keyrecord_t *record) {
    // 1. Get Row and Col from the event
    uint8_t row = record->event.key.row;
    uint8_t col = record->event.key.col;

    // 2. Look up the LED Index
    // This table is defined in your keyboard's config (usually rgb_matrix.c)
    uint8_t led_index = g_led_config.matrix_co[row][col];

    // 3. Safety Check
    // Some matrix positions (like "ghost" keys) might not have an LED.
    // QMK uses NO_LED (usually 255) to mark these.
    if (led_index != NO_LED) {
        indicator_library[id].led_index = led_index;
    }
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
            case UG_ANIM1:
            case UG_ANIM2:
            case UG_ANIM3:
                // A key that does nothing here leaves the layer dimmed
                if (!(host_owns_lighting() && inert_under_host(keycode))) {
                    rgb_adjusted_in_fn = true;
                }
                break;
        }
    }

    switch (keycode) {
        case (QK_DYNAMIC_MACRO_RECORD_START_1):
        case (QK_DYNAMIC_MACRO_RECORD_START_2):
            update_led_index(INDICATOR_MACRO_REC, record);
            break;
        case LALT_T(KC_NO):
        case GUI_T(KC_NO):
            update_led_index(INDICATOR_LEADER, record);
            if (record->tap.count > 0) {
                if (!record->event.pressed) { // Trigger on release for better accuracy
                    leader_start();
                }
                return false; // Don't send KC_NO
            }
            break;
        case UG_ANIM1:
#ifdef SIGNALRGB_ENABLE
            if (signalrgb_is_streaming()) {
                if (record->event.pressed) {
                    tap_code16(S(C(A(G(KC_Z)))));
                }
                return false;
            }
#endif
            if (record->event.pressed) {
#ifdef OPENRGB_ENABLE
                openrgb_release_effect();
#endif
                rgb_matrix_mode(RGB_MATRIX_SOLID_COLOR);
                rgb_matrix_sethsv(0, 0, 255);
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

        case UG_VALU:
            // Local unless kbd-daemon takes it (rgb_key_to_host); then the
            // direct-mode renderer scales by val, so this dims the incoming
            // frames too (SignalRGB, or OpenRGB without kbd-daemon).
            if (rgb_key_to_host(keycode, record)) return false;
            if (record->event.pressed) {
                if (user_config.bg_blackout_mode) {
                    user_config.bg_blackout_mode = false;
                    user_config_save();
                    rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), MIN_SAFE_BRIGHTNESS + RGB_MATRIX_VAL_STEP);
                } else {
                    rgb_matrix_increase_val_noeeprom();
                }
                save_brightness();
                // Sync indicator brightness to keyboard brightness (with floor)
                uint8_t new_val                  = rgb_matrix_get_val();
                user_config.indicator_brightness = new_val < MIN_INDICATOR_BRIGHTNESS ? MIN_INDICATOR_BRIGHTNESS : new_val;
                user_config_save();
            }
            return false;

        case UG_VALD:
            // See UG_VALU
            if (rgb_key_to_host(keycode, record)) return false;
            if (record->event.pressed) {
                uint8_t current_val = rgb_matrix_get_val();
                if (current_val <= MIN_SAFE_BRIGHTNESS + RGB_MATRIX_VAL_STEP) {
                    rgb_matrix_sethsv_noeeprom(rgb_matrix_get_hue(), rgb_matrix_get_sat(), MIN_SAFE_BRIGHTNESS);
                    user_config.bg_blackout_mode = true;
                    user_config_save();
                } else {
                    if (user_config.bg_blackout_mode) {
                        user_config.bg_blackout_mode = false;
                        user_config_save();
                    }
                    rgb_matrix_decrease_val_noeeprom();
                }
                save_brightness();
                // Sync indicator brightness to keyboard brightness (with floor)
                uint8_t new_val                  = rgb_matrix_get_val();
                user_config.indicator_brightness = new_val < MIN_INDICATOR_BRIGHTNESS ? MIN_INDICATOR_BRIGHTNESS : new_val;
                user_config_save();
            }
            return false;

        case UG_NEXT:
#ifdef SIGNALRGB_ENABLE
            if (signalrgb_is_streaming()) {
                if (record->event.pressed) {
                    tap_code16(S(C(A(G(KC_Q)))));
                }
                return false;
            }
#endif
            if (rgb_key_to_host(keycode, record)) return false;
            // Let QMK handle it when no host takes it
            return true;

        case UG_PREV:
#ifdef SIGNALRGB_ENABLE
            if (signalrgb_is_streaming()) {
                if (record->event.pressed) {
                    tap_code16(S(C(A(G(KC_A)))));
                }
                return false;
            }
#endif
            if (rgb_key_to_host(keycode, record)) return false;
            // Let QMK handle it when no host takes it
            return true;

        case UG_TOGG:
            // RGB toggle should always use QMK handling, not SignalRGB
            // This prevents flickers when toggling RGB on/off
            if (rgb_key_to_host(keycode, record)) return false;
            if (host_owns_lighting()) {
                // QMK's toggle would save the host's mode along with it
                if (record->event.pressed) {
                    rgb_matrix_toggle_noeeprom();
                    save_rgb_enable();
                }
                return false;
            }
            return true;

        case UG_HUEU:
        case UG_HUED:
        case UG_SATU:
        case UG_SATD:
        case UG_SPDU:
        case UG_SPDD:
            // See inert_under_host
            return !host_owns_lighting();
        case IND_BR_U:
            if (record->event.pressed) {
                if (user_config.indicator_brightness < 255) {
                    user_config.indicator_brightness = (user_config.indicator_brightness + RGB_MATRIX_VAL_STEP > 255) ? 255 : user_config.indicator_brightness + RGB_MATRIX_VAL_STEP;
                    user_config_save();
                }
            }
            return false;
        case IND_BR_D:
            if (record->event.pressed) {
                if (user_config.indicator_brightness > 0) {
                    user_config.indicator_brightness = (user_config.indicator_brightness < RGB_MATRIX_VAL_STEP) ? 0 : user_config.indicator_brightness - RGB_MATRIX_VAL_STEP;
                    user_config_save();
                }
            }
            return false;
        case IND_MODE:
            if (record->event.pressed) {
                indicators_set_always_on(!user_config.indicators_always_on, true);
            }
            return false;
    }

    return true;
}

layer_state_t layer_state_set_shared(layer_state_t state) {
    // Note: This function should be called from the keymap's layer_state_set_user
    // The keymap needs to determine which FN layer is active and pass that info
    // This is a limitation since layer enums are keyboard-specific

    // This function primarily updates the LED mask when layer state changes
    // The actual layer tracking should be done in the keymap

    // Any layer change (holding space/Fn/hardware key, gaming toggle) brings
    // the indicators back
    static layer_state_t last_state = 0;
    if (state != last_state) {
        last_state = state;
        indicators_wake();
    }

    return state;
}

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
static uint8_t get_ext_rgb_max_brightness(void) {
    uint8_t max_val = 0;
    for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
        if (srgb_led_buffer[i].r > max_val) max_val = srgb_led_buffer[i].r;
        if (srgb_led_buffer[i].g > max_val) max_val = srgb_led_buffer[i].g;
        if (srgb_led_buffer[i].b > max_val) max_val = srgb_led_buffer[i].b;
    }
    return max_val;
}
#endif


bool rgb_matrix_indicators_advanced_shared(uint8_t led_min, uint8_t led_max) {
#ifdef OPENRGB_ENABLE
    openrgb_reassert_pending_hsv();
#endif
    if (user_config.bg_blackout_mode) {
        for (uint8_t i = led_min; i < led_max; i++) {
            rgb_matrix_set_color(i, 0, 0, 0);
        }
    }

    if (dimming_state.is_dimmed) {
        return true;
    }

    // Determine indicator brightness based on external RGB state
#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
    // Match indicator brightness to the external frame whenever the external
    // buffer is what's on the keys (SignalRGB or OpenRGB direct), regardless of the toggle
    uint8_t ind_brightness = (rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_SIGNALRGB) ? (uint16_t)get_ext_rgb_max_brightness() * rgb_matrix_get_val() / 255 : user_config.indicator_brightness;
#else
    uint8_t ind_brightness = user_config.indicator_brightness;
#endif
    // Enforce minimum indicator brightness unless user manually set it lower via IND_BR_D
    if (ind_brightness < MIN_INDICATOR_BRIGHTNESS && user_config.indicator_brightness >= MIN_INDICATOR_BRIGHTNESS) {
        ind_brightness = MIN_INDICATOR_BRIGHTNESS;
    }

    // Show FN layer mask if FN layer is active (override layer)
    if (active_fn_layer != 0) {
        // Per frame: the mode can change while the layer is held
        bool host = host_owns_lighting();
        for (uint8_t i = led_min; i < led_max; i++) {
            if (i < RGB_MATRIX_LED_COUNT) {
                if (mod_led_mask[i] && !(host && host_inert_mask[i])) {
                    // Bound keys in the fn-layer indicator colour, at indicator brightness
                    rgb_led_t c = indicator_colors[INDICATOR_FN_LAYER][0];
                    rgb_matrix_set_color(i, c.r * ind_brightness / 255, c.g * ind_brightness / 255, c.b * ind_brightness / 255);
                } else if (!rgb_adjusted_in_fn) {
                    // Keys without a binding on the layer dim to layer_dim of
                    // their regular colour (black where it can't be computed),
                    // unless RGB was adjusted while the layer was held
                    rgb_led_t base;
                    if (get_base_color(i, &base)) {
                        rgb_matrix_set_color(i, base.r * layer_dim / 255, base.g * layer_dim / 255, base.b * layer_dim / 255);
                    } else {
                        rgb_matrix_set_color(i, 0, 0, 0);
                    }
                }
                // Otherwise, let the RGB matrix show (either QMK effects or SignalRGB)
            }
        }
    }

    // Render active indicators (top layer)
    uint8_t visibility = indicator_visibility();
    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        if (indicator_library[i].active && indicator_library[i].led_index != 255) {
            uint8_t   led   = indicator_library[i].led_index;
            rgb_led_t color = indicator_current_color(i);
            // Scale brightness
            // Basic approximation: scale each component by the brightness ratio
            uint8_t r = (color.r * (uint16_t)ind_brightness) / 255;
            uint8_t g = (color.g * (uint16_t)ind_brightness) / 255;
            uint8_t b = (color.b * (uint16_t)ind_brightness) / 255;

            if (!indicator_library[i].always_on && visibility < 255) {
                rgb_led_t base;
                if (visibility == 0 || !get_base_color(led, &base)) {
                    continue; // Hidden: leave the effect's color alone
                }
                r = blend(base.r, r, visibility);
                g = blend(base.g, g, visibility);
                b = blend(base.b, b, visibility);
            }
            rgb_matrix_set_color(led, r, g, b);
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
    } else if (leader_sequence_two_keys(KC_E, KC_W)) {
        SEND_STRING("szymonf@google.com");
    } else if (leader_sequence_one_key(KC_T)) {
        SEND_STRING("+41778150320");
    } else if (leader_sequence_two_keys(KC_T, KC_P)) {
        SEND_STRING("+48732258424");
    }
}

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
#ifdef SIGNALRGB_ENABLE
extern bool srgb_raw_hid_rx(uint8_t *data, uint8_t length);
#endif

bool raw_hid_receive_shared(uint8_t src, uint8_t *data, uint8_t length) {
    // --- OpenRGB commands (0x01–0x09) ---
    // Never gated by the ext-RGB toggle: OpenRGB's detector busy-loops until it
    // gets a reply, so a dropped query hangs its whole HID detection thread.
    // SET_MODE decides whether the matrix is in direct or a native effect; the
    // toggle key below only governs SignalRGB.
#ifdef HOST_PROTOCOL_ENABLE
    // --- Host protocol (0xC0–0xCF) ---
    if (host_protocol_rx(data, length)) {
        return true;
    }
#endif

#ifdef OPENRGB_ENABLE
    if (data[0] >= 0x01 && data[0] <= 0x09) {
        ext_rgb_state.last_activity = timer_read32();
        ext_rgb_state.active_source = EXT_RGB_OPENRGB;
        ext_rgb_state.timed_out     = false;

        if (openrgb_raw_hid_rx(data, length)) {
            register_activity();
            return true;
        }
        return false;
    }
#endif

    // --- SignalRGB commands (0x21–0x28) ---
#ifdef SIGNALRGB_ENABLE
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

    // Only a frame means SignalRGB is actually driving. Its detection keeps polling
    // even with the device switched off in the UI, and letting a probe claim the
    // source would forward the RGB keys as hotkeys with nothing rendering.
    if (data[0] == STREAM_RGB_DATA) {
        ext_rgb_state.last_activity = timer_read32();
        ext_rgb_state.active_source = EXT_RGB_SIGNALRGB;
        ext_rgb_state.timed_out     = false;

        // Frames re-assert direct mode, not just the first one after a timeout:
        // MODE_ENABLE arrives only at startup or on a UI device toggle, so a local
        // effect change would otherwise strand the stream in a buffer nobody renders.
        if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_SIGNALRGB) {
            signalrgb_mode_enable();
        }
    }

    // Process SignalRGB HID messages
    if (srgb_raw_hid_rx(data, length)) {
        // Track USB activity for inactivity dimming
        register_activity();
        return true;
    }
#endif

    return false;
}

#if defined(VIA_ENABLE)
bool via_command_shared(uint8_t src, uint8_t *data, uint8_t length) {
    return raw_hid_receive_shared(src, data, length);
}
#endif
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
    indicator_library[INDICATOR_MACRO_REC].active = true;
    return true;
}

bool dynamic_macro_record_end_shared(int8_t direction) {
    indicator_library[INDICATOR_MACRO_REC].active = false;
    return true;
}

void suspend_power_down_shared(void) {
    suspended = true;
    if (!dimming_state.is_dimmed) {
        dimming_state.saved_brightness = rgb_matrix_get_val();
        dimming_state.saved_mode       = rgb_matrix_get_mode();
        dimming_state.is_dimmed        = true;
    }
}

void suspend_wakeup_init_shared(void) {
    suspended = false;
    register_activity();
    indicators_wake();
}
