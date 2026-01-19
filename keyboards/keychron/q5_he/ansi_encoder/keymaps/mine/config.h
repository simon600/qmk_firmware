#pragma once

// Expand the number of layers to 8
#define DYNAMIC_KEYMAP_LAYER_COUNT 8
#define RGB_MATRIX_DEFAULT_MODE RGB_MATRIX_SOLID_COLOR
#define RGB_MATRIX_DEFAULT_HUE 156
#define RGB_MATRIX_DEFAULT_SAT 191
#define RGB_MATRIX_DEFAULT_VAL RGB_MATRIX_MAXIMUM_BRIGHTNESS // Sets the default brightness value, if none has been set

// Use SignalRGB buffer mode for better control of LED application order
#define SIGNALRGB_USE_BUFFER
