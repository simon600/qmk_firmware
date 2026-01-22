#pragma once

#include "quantum.h"
#ifdef SIGNALRGB_ENABLE
#    include "signalrgb.h"
#endif

// --- CUSTOM KEYCODES ---
enum custom_keycodes_shared {
    UG_SRGB = SAFE_RANGE,
    UG_ANIM1,
    UG_ANIM2,
    UG_ANIM3,
    M_ENDW,
    M_ENDM,
    SAFE_RANGE_SHARED, // For keymaps to extend
};

// --- CONSTANTS ---
// Inactivity timeout: 5 minutes in milliseconds
#define INACTIVITY_TIMEOUT_MS 300000

// --- INDICATOR REGISTRY (X-Macro System) ---

// Helper macro for conditional SignalRGB indicator
#if defined(SIGNALRGB_ENABLE)
#    define IF_SIGNALRGB_ENABLED(x) x
#else
#    define IF_SIGNALRGB_ENABLED(x)
#endif

// Universal indicators available in userspace
#define SHARED_INDICATOR_IDS \
    X(INDICATOR_MACRO_REC)   \
    IF_SIGNALRGB_ENABLED(X(INDICATOR_SIGNALRGB))

// Allow keyboards to extend with their own indicators
#ifndef KEYBOARD_INDICATOR_IDS
#    define KEYBOARD_INDICATOR_IDS
#endif

// Generate enum from indicator IDs
#define X(id) id,
enum indicator_ids { SHARED_INDICATOR_IDS KEYBOARD_INDICATOR_IDS INDICATOR_COUNT };
#undef X

// --- TYPE DEFINITIONS ---
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_t;

typedef struct {
    uint8_t   led_index; // Physical LED index (255 = disabled/unmapped)
    bool      active;    // Whether indicator is currently active
    rgb_led_t color;     // RGB color value
} indicator_t;

#ifdef SIGNALRGB_ENABLE
typedef struct {
    uint32_t last_activity; // Last HID activity timestamp
    bool     timed_out;     // Whether SignalRGB has timed out
    bool     user_enabled;  // Whether user has enabled SignalRGB via toggle
} signalrgb_state_t;
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
void leader_end_shared(void);

#if defined(VIA_ENABLE) && defined(SIGNALRGB_ENABLE)
// VIA command handling
bool via_command_shared(uint8_t src, uint8_t *data, uint8_t length);
#endif

// State access (for keymap-specific logic if needed)
indicator_t *get_indicators(void);
uint8_t      get_active_fn_layer(void);
void         set_active_fn_layer(uint8_t layer);
