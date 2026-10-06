#pragma once

// Set RAW HID endpoint size to 64 bytes for OpenRGB
#define RAW_EPSIZE 64

// Enable combo_should_trigger to conditionally disable combos on gaming layers
#define COMBO_SHOULD_TRIGGER

// Expand the number of layers to 8
#define DYNAMIC_KEYMAP_LAYER_COUNT 8
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#define RGB_MATRIX_DEFAULT_HUE 0
#define RGB_MATRIX_DEFAULT_SAT 0
#define RGB_MATRIX_DEFAULT_VAL RGB_MATRIX_MAXIMUM_BRIGHTNESS // Sets the default brightness value, if none has been set

// Use SignalRGB buffer mode for better control of LED application order
#define SIGNALRGB_USE_BUFFER

// --- X-Macro Indicator Registry Configuration ---

// Define keyboard-specific indicator IDs (extends SHARED_INDICATOR_IDS)
// X(enum id, host wire id, states): gaming states are rapid trigger / gamepad
#define KEYBOARD_INDICATOR_IDS X(INDICATOR_GAMING, 4, 2)
#define GAMING_IND_RAPID 0
#define GAMING_IND_GAMEPAD 1

// Helper macro for conditional SignalRGB indicator mapping
#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)
#    define M_SIGNALRGB_INDICATOR(idx) [INDICATOR_SIGNALRGB] = {.led_index = idx, .always_on = true},
#else
#    define M_SIGNALRGB_INDICATOR(idx)
#endif

// Map indicator IDs to physical LED indices (36 = Tab: Caps Word)
// Format: [ID] = {.led_index = INDEX}
#define KEYBOARD_LED_MAP [INDICATOR_GAMING] = {.led_index = 14}, [INDICATOR_CAPS_WORD] = {.led_index = 36}, M_SIGNALRGB_INDICATOR(16)

// Default colour per state (host-editable, stored in the user datablock)
#define KEYBOARD_INDICATOR_COLORS [INDICATOR_GAMING] = {[GAMING_IND_RAPID] = IND_COLOR_MAROON, [GAMING_IND_GAMEPAD] = IND_COLOR_GREEN},

#define OPENRGB_DEVICE_NAME "Keychron Q5 HE"
#define OPENRGB_DEVICE_VENDOR "Keychron"

