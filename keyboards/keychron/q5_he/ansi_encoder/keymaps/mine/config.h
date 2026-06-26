#pragma once

// Enable combo_should_trigger to conditionally disable combos on gaming layers
#define COMBO_SHOULD_TRIGGER

// Expand the number of layers to 8
#define DYNAMIC_KEYMAP_LAYER_COUNT 8
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#define RGB_MATRIX_DEFAULT_HUE 156
#define RGB_MATRIX_DEFAULT_SAT 191
#define RGB_MATRIX_DEFAULT_VAL RGB_MATRIX_MAXIMUM_BRIGHTNESS // Sets the default brightness value, if none has been set

// Use SignalRGB buffer mode for better control of LED application order
#define SIGNALRGB_USE_BUFFER

// --- X-Macro Indicator Registry Configuration ---

// Define keyboard-specific indicator IDs (extends SHARED_INDICATOR_IDS)
#define KEYBOARD_INDICATOR_IDS X(INDICATOR_GAMING)

// Helper macro for conditional SignalRGB indicator mapping
#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
#    define M_SIGNALRGB_INDICATOR(idx) [INDICATOR_SIGNALRGB] = {.led_index = idx, .color = {255, 255, 255}},
#else
#    define M_SIGNALRGB_INDICATOR(idx)
#endif

// Map indicator IDs to physical LED indices and colors
// Format: [ID] = {.led_index = INDEX, .color = {R, G, B}}
#define KEYBOARD_LED_MAP [INDICATOR_GAMING] = {.led_index = 14, .color = {255, 0, 0}}, M_SIGNALRGB_INDICATOR(36)

#define OPENRGB_DEVICE_NAME "Keychron Q5 HE"
#define OPENRGB_DEVICE_VENDOR "Keychron"

