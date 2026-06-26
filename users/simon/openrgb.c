#include "openrgb.h"
#include "quantum.h"
#include "raw_hid.h"
#include "version.h"
#include "print.h"



#ifndef SIGNALRGB_ENABLE
#    if defined(RGBLIGHT_ENABLE) && defined(RGB_MATRIX_ENABLE)
#        define TOTAL_LEDS (RGBLIGHT_LED_COUNT + RGB_MATRIX_LED_COUNT)
#    elif defined(RGBLIGHT_ENABLE)
#        define TOTAL_LEDS RGBLIGHT_LED_COUNT
#    elif defined(RGB_MATRIX_ENABLE)
#        define TOTAL_LEDS RGB_MATRIX_LED_COUNT
#    else
#        define TOTAL_LEDS 0
#    endif

rgb_led_t srgb_led_buffer[TOTAL_LEDS] = {0};
#else
extern rgb_led_t srgb_led_buffer[];
#endif

// Forward declarations of helper functions
static uint8_t qmk_mode_to_openrgb(uint8_t qmk_mode);
static uint8_t openrgb_mode_to_qmk(uint8_t openrgb_mode);
static uint8_t get_led_keycode(uint8_t led_index);
static uint8_t get_enabled_modes_list(uint8_t *buf);

bool openrgb_raw_hid_rx(uint8_t *data, uint8_t length) {

    uint8_t response[RAW_EPSIZE] = {0};

    switch (data[0]) {
        case QMK_OPENRGB_GET_PROTOCOL_VERSION:
            response[0] = QMK_OPENRGB_GET_PROTOCOL_VERSION;
            response[1] = OPENRGB_PROTOCOL_VERSION;
            response[RAW_EPSIZE - 1] = 100;

            raw_hid_send(response, RAW_EPSIZE);
            break;

        case QMK_OPENRGB_GET_QMK_VERSION:
            response[0] = QMK_OPENRGB_GET_QMK_VERSION;
            strncpy((char *)&response[1], QMK_VERSION, RAW_EPSIZE - 3);
            response[RAW_EPSIZE - 2] = '\0';
            response[RAW_EPSIZE - 1] = 100;

            raw_hid_send(response, RAW_EPSIZE);
            break;

        case QMK_OPENRGB_GET_DEVICE_INFO: {
            response[0] = QMK_OPENRGB_GET_DEVICE_INFO;
            response[1] = RGB_MATRIX_LED_COUNT;
            response[2] = RGB_MATRIX_LED_COUNT;
            uint8_t offset = 3;
#ifndef OPENRGB_DEVICE_NAME
#    define OPENRGB_DEVICE_NAME "Keychron Q5 HE"
#endif
#ifndef OPENRGB_DEVICE_VENDOR
#    define OPENRGB_DEVICE_VENDOR "Keychron"
#endif
            strncpy((char *)&response[offset], OPENRGB_DEVICE_NAME, (RAW_EPSIZE - 2) / 2 - offset);
            offset += strlen((char *)&response[offset]) + 1;
            if (offset < RAW_EPSIZE - 3) {
                strncpy((char *)&response[offset], OPENRGB_DEVICE_VENDOR, RAW_EPSIZE - 2 - offset);
            }
            response[RAW_EPSIZE - 2] = '\0';
            response[RAW_EPSIZE - 1] = 100;

            raw_hid_send(response, RAW_EPSIZE);
            break;
        }

        case QMK_OPENRGB_GET_MODE_INFO:
            response[0] = QMK_OPENRGB_GET_MODE_INFO;
            response[1] = qmk_mode_to_openrgb(rgb_matrix_get_mode());
            response[2] = rgb_matrix_get_speed();
            response[3] = rgb_matrix_get_hue();
            response[4] = rgb_matrix_get_sat();
            response[5] = rgb_matrix_get_val();
            response[RAW_EPSIZE - 1] = 100;

            raw_hid_send(response, RAW_EPSIZE);
            break;

        case QMK_OPENRGB_GET_LED_INFO: {
            uint8_t start = data[1];
            uint8_t count = data[2];
            if (count > 8) count = 8;
            response[0] = QMK_OPENRGB_GET_LED_INFO;
            for (uint8_t i = 0; i < count; i++) {
                uint8_t led_idx = start + i;
                uint8_t base = 1 + i * 7;
                if (led_idx < RGB_MATRIX_LED_COUNT) {
                    response[base]     = g_led_config.point[led_idx].x;
                    response[base + 1] = g_led_config.point[led_idx].y;
                    response[base + 2] = g_led_config.flags[led_idx];
                    response[base + 3] = 255;
                    response[base + 4] = 255;
                    response[base + 5] = 255;
                    response[base + 6] = get_led_keycode(led_idx);
                } else {
                    response[base]     = 255;
                    response[base + 1] = 255;
                    response[base + 2] = 0;
                    response[base + 3] = 0;
                    response[base + 4] = 0;
                    response[base + 5] = 0;
                    response[base + 6] = 0;
                }
            }
            response[RAW_EPSIZE - 1] = 100;

            raw_hid_send(response, RAW_EPSIZE);
            break;
        }

        case QMK_OPENRGB_GET_ENABLED_MODES:
            response[0] = QMK_OPENRGB_GET_ENABLED_MODES;
            get_enabled_modes_list(&response[1]);
            response[RAW_EPSIZE - 1] = 100;

            raw_hid_send(response, RAW_EPSIZE);
            break;

        case QMK_OPENRGB_SET_MODE: {
            uint8_t hue = data[1];
            uint8_t sat = data[2];
            uint8_t val = data[3];
            uint8_t mode = data[4];
            uint8_t speed = data[5];
            uint8_t save = data[6];


            if (mode == 25) {
                openrgb_mode_enable();
            } else {
                uint8_t qmk_mode = openrgb_mode_to_qmk(mode);
                if (save) {
                    rgb_matrix_sethsv(hue, sat, val);
                    rgb_matrix_set_speed(speed);
                    rgb_matrix_mode(qmk_mode);
                } else {
                    rgb_matrix_sethsv_noeeprom(hue, sat, val);
                    rgb_matrix_set_speed_noeeprom(speed);
                    rgb_matrix_mode_noeeprom(qmk_mode);
                }
            }
            break;
        }

        case QMK_OPENRGB_DIRECT_MODE_SET_SINGLE_LED: {
            uint8_t led = data[1];
            uint8_t r = data[2];
            uint8_t g = data[3];
            uint8_t b = data[4];
            if (led < RGB_MATRIX_LED_COUNT) {
                srgb_led_buffer[led] = (rgb_led_t){r, g, b};
            }
            break;
        }

        case QMK_OPENRGB_DIRECT_MODE_SET_LEDS: {
            uint8_t start = data[1];
            uint8_t count = data[2];
            if (count > 20) count = 20;
            for (uint8_t i = 0; i < count; i++) {
                uint8_t led_idx = start + i;
                uint8_t offset = 3 + i * 3;
                if (led_idx < RGB_MATRIX_LED_COUNT) {
                    srgb_led_buffer[led_idx] = (rgb_led_t){data[offset], data[offset + 1], data[offset + 2]};
                }
            }
            break;
        }

        default:
            return false;
    }
    return true;
}

void openrgb_mode_enable(void) {
#ifdef RGB_MATRIX_ENABLE
    rgb_matrix_mode_noeeprom(RGB_MATRIX_CUSTOM_SIGNALRGB);
#endif
}

void openrgb_mode_disable(void) {
#ifdef RGB_MATRIX_ENABLE
    rgb_matrix_reload_from_eeprom();
#endif
}

static uint8_t qmk_mode_to_openrgb(uint8_t qmk_mode) {
    if (qmk_mode == RGB_MATRIX_CUSTOM_SIGNALRGB) {
        return 25;
    }
    switch (qmk_mode) {
#ifdef ENABLE_RGB_MATRIX_SOLID_COLOR
        case RGB_MATRIX_SOLID_COLOR: return 1;
#endif
#ifdef ENABLE_RGB_MATRIX_ALPHAS_MODS
        case RGB_MATRIX_ALPHAS_MODS: return 2;
#endif
#ifdef ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
        case RGB_MATRIX_GRADIENT_UP_DOWN: return 3;
#endif
#ifdef ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
        case RGB_MATRIX_GRADIENT_LEFT_RIGHT: return 4;
#endif
#ifdef ENABLE_RGB_MATRIX_BREATHING
        case RGB_MATRIX_BREATHING: return 5;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SAT
        case RGB_MATRIX_BAND_SAT: return 6;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_VAL
        case RGB_MATRIX_BAND_VAL: return 7;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
        case RGB_MATRIX_BAND_PINWHEEL_SAT: return 8;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
        case RGB_MATRIX_BAND_PINWHEEL_VAL: return 9;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
        case RGB_MATRIX_BAND_SPIRAL_SAT: return 10;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL
        case RGB_MATRIX_BAND_SPIRAL_VAL: return 11;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_ALL
        case RGB_MATRIX_CYCLE_ALL: return 12;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
        case RGB_MATRIX_CYCLE_LEFT_RIGHT: return 13;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
        case RGB_MATRIX_CYCLE_UP_DOWN: return 14;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_OUT_IN
        case RGB_MATRIX_CYCLE_OUT_IN: return 15;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
        case RGB_MATRIX_CYCLE_OUT_IN_DUAL: return 16;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
        case RGB_MATRIX_RAINBOW_MOVING_CHEVRON: return 17;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
        case RGB_MATRIX_CYCLE_PINWHEEL: return 18;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_SPIRAL
        case RGB_MATRIX_CYCLE_SPIRAL: return 19;
#endif
#ifdef ENABLE_RGB_MATRIX_DUAL_BEACON
        case RGB_MATRIX_DUAL_BEACON: return 20;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_BEACON
        case RGB_MATRIX_RAINBOW_BEACON: return 21;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS
        case RGB_MATRIX_RAINBOW_PINWHEELS: return 22;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINDROPS
        case RGB_MATRIX_RAINDROPS: return 23;
#endif
#ifdef ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
        case RGB_MATRIX_JELLYBEAN_RAINDROPS: return 24;
#endif
        default: return 1;
    }
}

static uint8_t openrgb_mode_to_qmk(uint8_t openrgb_mode) {
    if (openrgb_mode == 25) {
        return RGB_MATRIX_CUSTOM_SIGNALRGB;
    }
    switch (openrgb_mode) {
#ifdef ENABLE_RGB_MATRIX_SOLID_COLOR
        case 1: return RGB_MATRIX_SOLID_COLOR;
#endif
#ifdef ENABLE_RGB_MATRIX_ALPHAS_MODS
        case 2: return RGB_MATRIX_ALPHAS_MODS;
#endif
#ifdef ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
        case 3: return RGB_MATRIX_GRADIENT_UP_DOWN;
#endif
#ifdef ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
        case 4: return RGB_MATRIX_GRADIENT_LEFT_RIGHT;
#endif
#ifdef ENABLE_RGB_MATRIX_BREATHING
        case 5: return RGB_MATRIX_BREATHING;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SAT
        case 6: return RGB_MATRIX_BAND_SAT;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_VAL
        case 7: return RGB_MATRIX_BAND_VAL;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
        case 8: return RGB_MATRIX_BAND_PINWHEEL_SAT;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
        case 9: return RGB_MATRIX_BAND_PINWHEEL_VAL;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
        case 10: return RGB_MATRIX_BAND_SPIRAL_SAT;
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL
        case 11: return RGB_MATRIX_BAND_SPIRAL_VAL;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_ALL
        case 12: return RGB_MATRIX_CYCLE_ALL;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
        case 13: return RGB_MATRIX_CYCLE_LEFT_RIGHT;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
        case 14: return RGB_MATRIX_CYCLE_UP_DOWN;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_OUT_IN
        case 15: return RGB_MATRIX_CYCLE_OUT_IN;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
        case 16: return RGB_MATRIX_CYCLE_OUT_IN_DUAL;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
        case 17: return RGB_MATRIX_RAINBOW_MOVING_CHEVRON;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
        case 18: return RGB_MATRIX_CYCLE_PINWHEEL;
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_SPIRAL
        case 19: return RGB_MATRIX_CYCLE_SPIRAL;
#endif
#ifdef ENABLE_RGB_MATRIX_DUAL_BEACON
        case 20: return RGB_MATRIX_DUAL_BEACON;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_BEACON
        case 21: return RGB_MATRIX_RAINBOW_BEACON;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS
        case 22: return RGB_MATRIX_RAINBOW_PINWHEELS;
#endif
#ifdef ENABLE_RGB_MATRIX_RAINDROPS
        case 23: return RGB_MATRIX_RAINDROPS;
#endif
#ifdef ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
        case 24: return RGB_MATRIX_JELLYBEAN_RAINDROPS;
#endif
        default: return RGB_MATRIX_SOLID_COLOR;
    }
}

static uint8_t get_led_keycode(uint8_t led_index) {
    if (led_index >= RGB_MATRIX_LED_COUNT) return 0;
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (g_led_config.matrix_co[row][col] == led_index) {
                uint16_t keycode = keymap_key_to_keycode(0, (keypos_t){col, row});
                return keycode & 0xFF;
            }
        }
    }
    return 0;
}

static uint8_t get_enabled_modes_list(uint8_t *buf) {
    uint8_t idx = 0;
    for (uint8_t i = 1; i <= 25; i++) {
        buf[idx++] = i;
    }
    buf[idx++] = 0;  // Null terminator
    return idx;
}
