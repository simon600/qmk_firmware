#pragma once

#include "quantum.h"
#ifdef SIGNALRGB_ENABLE
#    include "signalrgb.h"
#endif
#ifdef OPENRGB_ENABLE
#    include "openrgb.h"
#endif

// --- CUSTOM KEYCODES ---
enum custom_keycodes_shared {
    UG_ANIM1 = SAFE_RANGE,
    UG_ANIM2,
    UG_ANIM3,
    M_NW,
    M_NM,
    IND_BR_U,
    IND_BR_D,
    IND_MODE, // Toggle indicators between always-on and auto-hide
    SAFE_RANGE_SHARED, // For keymaps to extend
};

// --- COMBO DEFINITIONS ---
/*enum combos {
    OP_BSPC,
    IO_DEL,
    COMBO_COUNT,
};
 extern const uint16_t PROGMEM op_combo[];
 extern combo_t                key_combos[COMBO_COUNT];
 */

// --- CONSTANTS ---
// Inactivity timeout: 5 minutes in milliseconds
#define INACTIVITY_TIMEOUT_MS 300000

// Auto-hide indicators: how long they stay up after a layer change or state
// change, and how long they take to fade back into the key's regular color
#ifndef INDICATOR_SHOW_MS
#    define INDICATOR_SHOW_MS 2000
#endif
#ifndef INDICATOR_FADE_MS
#    define INDICATOR_FADE_MS 500
#endif

// --- INDICATOR COLORS ---
// Catppuccin Macchiato hues with saturation pushed up: the palette's pastels
// wash out to near-white on LEDs and vanish against a white backlight.
// Defaults for the indicator colour table. Usable as an initializer or a compound literal
// ((rgb_led_t)IND_COLOR_PEACH).
#define IND_COLOR_PEACH {255, 110, 40}     // #f5a97f
#define IND_COLOR_MAUVE {170, 80, 255}     // #c6a0f6
#define IND_COLOR_RED {255, 40, 70}        // #ed8796
#define IND_COLOR_MAROON {255, 60, 60}     // #ee99a0
#define IND_COLOR_GREEN {90, 255, 70}      // #a6da95
#define IND_COLOR_LAVENDER {140, 150, 255} // #b7bdf8

// --- INDICATOR REGISTRY (X-Macro System) ---

// Helper macro for conditional external RGB indicator
#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
#    define IF_EXT_RGB_ENABLED(x) x
#else
#    define IF_EXT_RGB_ENABLED(x)
#endif

// Entry format: X(enum id, host wire id, number of states)
// The wire id is how the host protocol addresses an indicator; it must stay
// stable across keyboards and firmware versions (see host_protocol.h).
// Each state has its own colour (e.g. gaming: rapid trigger / gamepad).
#define INDICATOR_MAX_STATES 2

// Universal indicators available in userspace
#define SHARED_INDICATOR_IDS     \
    X(INDICATOR_MACRO_REC, 1, 1) \
    X(INDICATOR_LEADER, 2, 1)    \
    X(INDICATOR_CAPS_LOCK, 5, 1) \
    X(INDICATOR_NUM_LOCK, 6, 1)  \
    X(INDICATOR_FN_LAYER, 7, 1)  \
    X(INDICATOR_CAPS_WORD, 10, 1) \
    IF_EXT_RGB_ENABLED(X(INDICATOR_SIGNALRGB, 3, 1))

// Allow keyboards to extend with their own indicators
#ifndef KEYBOARD_INDICATOR_IDS
#    define KEYBOARD_INDICATOR_IDS
#endif

// Generate enum from indicator IDs
#define X(id, wire, states) id,
enum indicator_ids { SHARED_INDICATOR_IDS KEYBOARD_INDICATOR_IDS INDICATOR_COUNT };
#undef X

// --- TYPE DEFINITIONS ---
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

typedef struct {
    uint8_t led_index; // Physical LED index (255 = disabled/unmapped)
    bool    active;    // Whether indicator is currently active
    uint8_t state;     // Selects the colour in the indicator colour table
    bool    always_on; // Exempt from auto-hide (shown for as long as it's active)
} indicator_t;

typedef struct {
    uint32_t last_activity_time;
    bool     is_dimmed;
    uint8_t  saved_brightness;
    uint8_t  saved_mode;
} dimming_state_t;

typedef enum {
    EXT_RGB_NONE,
    EXT_RGB_SIGNALRGB,
    EXT_RGB_OPENRGB,
} ext_rgb_source_t;

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
typedef struct {
    uint32_t         last_activity; // Last HID activity timestamp
    bool             timed_out;     // Whether external RGB has timed out
    ext_rgb_source_t active_source; // Which source is active (SignalRGB or OpenRGB)
} ext_rgb_state_t;
#endif

// --- EEPROM USER CONFIG ---
// Packed into eeconfig_read_user()/eeconfig_update_user() 32-bit slot.
// Version 3: workspace / urgent indicators removed (the colour table shrank)
#define USER_CONFIG_VERSION 3

typedef union {
    uint32_t raw;
    struct {
        uint8_t version;              // byte 0: data version id
        uint8_t indicator_brightness; // byte 1
        bool    bg_blackout_mode;     // byte 2
        bool    indicators_always_on; // byte 3: false = auto-hide after INDICATOR_SHOW_MS
    };
} user_config_t;


// --- USER EEPROM DATABLOCK ---
// Keyboards running the host protocol keep user_config plus the indicator
// colour table in a user datablock; the rest keep user_config in the 32-bit
// user slot. Not for VIA keyboards: the datablock shifts the dynamic keymap.
#if (EECONFIG_USER_DATA_SIZE) > 0
typedef struct {
    user_config_t config;
    rgb_led_t     indicator_colors[INDICATOR_COUNT][INDICATOR_MAX_STATES];
    uint8_t       layer_dim;
} user_data_t;
#endif

// --- GLOBAL INDICATOR REGISTRY ---
extern indicator_t indicator_library[INDICATOR_COUNT];

// --- FUNCTION DECLARATIONS ---

// Initialization
void keyboard_post_init_shared(void);

// Periodic tasks
void matrix_scan_shared(void);

// Input processing
bool process_record_shared(uint16_t keycode, keyrecord_t *record);

// Layer state management
layer_state_t layer_state_set_shared(layer_state_t state);

// RGB LED rendering
bool rgb_matrix_indicators_advanced_shared(uint8_t led_min, uint8_t led_max);

// Tap-hold configuration
bool get_permissive_hold_shared(uint16_t keycode, keyrecord_t *record);

// Leader key
void leader_start_shared(void);
void leader_end_shared(void);

// Dynamic Macro
bool dynamic_macro_record_start_shared(int8_t direction);
bool dynamic_macro_record_end_shared(int8_t direction);

#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
// Raw HID / External RGB command handling
bool raw_hid_receive_shared(uint8_t src, uint8_t *data, uint8_t length);
#if defined(VIA_ENABLE)
bool via_command_shared(uint8_t src, uint8_t *data, uint8_t length);
#endif
#endif

// Suspend callbacks
void suspend_power_down_shared(void);
void suspend_wakeup_init_shared(void);

// State access (for keymap-specific logic if needed)
indicator_t *get_indicators(void);
uint8_t      get_active_fn_layer(void);
void         set_active_fn_layer(uint8_t layer);

// Settings and indicator colours (used by the host protocol)
user_config_t *get_user_config(void);
void           user_config_save(void);
void           indicators_set_always_on(bool always_on, bool persist);
void           host_activity(void);
uint8_t        indicator_wire_id(uint8_t id);
uint8_t        indicator_state_count(uint8_t id);
rgb_led_t      indicator_color_get(uint8_t id, uint8_t state);
void           indicator_color_set(uint8_t id, uint8_t state, rgb_led_t color, bool persist);
void           indicator_colors_reset(bool persist);
void           indicator_colors_reload(void);
uint8_t        layer_dim_get(void);
void           layer_dim_set(uint8_t dim, bool persist);

