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

#ifndef OPENRGB_DEVICE_NAME
#    define OPENRGB_DEVICE_NAME "Keychron Q5 HE"
#endif
#ifndef OPENRGB_DEVICE_VENDOR
#    define OPENRGB_DEVICE_VENDOR "Keychron"
#endif

// --- Mode table ---
//
// OpenRGB's SET_MODE value is *positional*: the host walks its fixed list of
// OpenRGB mode IDs (2..42, in enum order), keeps the ones we report as enabled,
// and numbers them 1..N in that order; "Direct" is always numbered N+1.
// So this table must stay in QMK_OPENRGB_MODE_* enum order and only contain
// effects that are actually compiled in.
typedef struct {
    uint8_t openrgb_id;
    uint8_t qmk_mode;
} openrgb_mode_entry_t;

#define OPENRGB_MODE(orgb, qmk) {QMK_OPENRGB_MODE_##orgb, RGB_MATRIX_##qmk},

static const openrgb_mode_entry_t openrgb_modes[] = {
    // Solid color is always compiled into rgb_matrix (no ENABLE_ define for it)
    OPENRGB_MODE(SOLID_COLOR, SOLID_COLOR)
#ifdef ENABLE_RGB_MATRIX_ALPHAS_MODS
    OPENRGB_MODE(ALPHA_MOD, ALPHAS_MODS)
#endif
#ifdef ENABLE_RGB_MATRIX_GRADIENT_UP_DOWN
    OPENRGB_MODE(GRADIENT_UP_DOWN, GRADIENT_UP_DOWN)
#endif
#ifdef ENABLE_RGB_MATRIX_GRADIENT_LEFT_RIGHT
    OPENRGB_MODE(GRADIENT_LEFT_RIGHT, GRADIENT_LEFT_RIGHT)
#endif
#ifdef ENABLE_RGB_MATRIX_BREATHING
    OPENRGB_MODE(BREATHING, BREATHING)
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SAT
    OPENRGB_MODE(BAND_SAT, BAND_SAT)
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_VAL
    OPENRGB_MODE(BAND_VAL, BAND_VAL)
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_PINWHEEL_SAT
    OPENRGB_MODE(BAND_PINWHEEL_SAT, BAND_PINWHEEL_SAT)
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_PINWHEEL_VAL
    OPENRGB_MODE(BAND_PINWHEEL_VAL, BAND_PINWHEEL_VAL)
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SPIRAL_SAT
    OPENRGB_MODE(BAND_SPIRAL_SAT, BAND_SPIRAL_SAT)
#endif
#ifdef ENABLE_RGB_MATRIX_BAND_SPIRAL_VAL
    OPENRGB_MODE(BAND_SPIRAL_VAL, BAND_SPIRAL_VAL)
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_ALL
    OPENRGB_MODE(CYCLE_ALL, CYCLE_ALL)
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_LEFT_RIGHT
    OPENRGB_MODE(CYCLE_LEFT_RIGHT, CYCLE_LEFT_RIGHT)
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_UP_DOWN
    OPENRGB_MODE(CYCLE_UP_DOWN, CYCLE_UP_DOWN)
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_OUT_IN
    OPENRGB_MODE(CYCLE_OUT_IN, CYCLE_OUT_IN)
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_OUT_IN_DUAL
    OPENRGB_MODE(CYCLE_OUT_IN_DUAL, CYCLE_OUT_IN_DUAL)
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_MOVING_CHEVRON
    OPENRGB_MODE(RAINBOW_MOVING_CHEVRON, RAINBOW_MOVING_CHEVRON)
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_PINWHEEL
    OPENRGB_MODE(CYCLE_PINWHEEL, CYCLE_PINWHEEL)
#endif
#ifdef ENABLE_RGB_MATRIX_CYCLE_SPIRAL
    OPENRGB_MODE(CYCLE_SPIRAL, CYCLE_SPIRAL)
#endif
#ifdef ENABLE_RGB_MATRIX_DUAL_BEACON
    OPENRGB_MODE(DUAL_BEACON, DUAL_BEACON)
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_BEACON
    OPENRGB_MODE(RAINBOW_BEACON, RAINBOW_BEACON)
#endif
#ifdef ENABLE_RGB_MATRIX_RAINBOW_PINWHEELS
    OPENRGB_MODE(RAINBOW_PINWHEELS, RAINBOW_PINWHEELS)
#endif
#ifdef ENABLE_RGB_MATRIX_RAINDROPS
    OPENRGB_MODE(RAINDROPS, RAINDROPS)
#endif
#ifdef ENABLE_RGB_MATRIX_JELLYBEAN_RAINDROPS
    OPENRGB_MODE(JELLYBEAN_RAINDROPS, JELLYBEAN_RAINDROPS)
#endif
#ifdef ENABLE_RGB_MATRIX_HUE_BREATHING
    OPENRGB_MODE(HUE_BREATHING, HUE_BREATHING)
#endif
#ifdef ENABLE_RGB_MATRIX_HUE_PENDULUM
    OPENRGB_MODE(HUE_PENDULUM, HUE_PENDULUM)
#endif
#ifdef ENABLE_RGB_MATRIX_HUE_WAVE
    OPENRGB_MODE(HUE_WAVE, HUE_WAVE)
#endif
#ifdef ENABLE_RGB_MATRIX_TYPING_HEATMAP
    OPENRGB_MODE(TYPING_HEATMAP, TYPING_HEATMAP)
#endif
#ifdef ENABLE_RGB_MATRIX_DIGITAL_RAIN
    OPENRGB_MODE(DIGITAL_RAIN, DIGITAL_RAIN)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE_SIMPLE
    OPENRGB_MODE(SOLID_REACTIVE_SIMPLE, SOLID_REACTIVE_SIMPLE)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE
    OPENRGB_MODE(SOLID_REACTIVE, SOLID_REACTIVE)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE_WIDE
    OPENRGB_MODE(SOLID_REACTIVE_WIDE, SOLID_REACTIVE_WIDE)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTIWIDE
    OPENRGB_MODE(SOLID_REACTIVE_MULTIWIDE, SOLID_REACTIVE_MULTIWIDE)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE_CROSS
    OPENRGB_MODE(SOLID_REACTIVE_CROSS, SOLID_REACTIVE_CROSS)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTICROSS
    OPENRGB_MODE(SOLID_REACTIVE_MULTICROSS, SOLID_REACTIVE_MULTICROSS)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE_NEXUS
    OPENRGB_MODE(SOLID_REACTIVE_NEXUS, SOLID_REACTIVE_NEXUS)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_REACTIVE_MULTINEXUS
    OPENRGB_MODE(SOLID_REACTIVE_MULTINEXUS, SOLID_REACTIVE_MULTINEXUS)
#endif
#ifdef ENABLE_RGB_MATRIX_SPLASH
    OPENRGB_MODE(SPLASH, SPLASH)
#endif
#ifdef ENABLE_RGB_MATRIX_MULTISPLASH
    OPENRGB_MODE(MULTISPLASH, MULTISPLASH)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_SPLASH
    OPENRGB_MODE(SOLID_SPLASH, SOLID_SPLASH)
#endif
#ifdef ENABLE_RGB_MATRIX_SOLID_MULTISPLASH
    OPENRGB_MODE(SOLID_MULTISPLASH, SOLID_MULTISPLASH)
#endif
};

#define OPENRGB_MODE_COUNT (sizeof(openrgb_modes) / sizeof(openrgb_modes[0]))
// Positional value OpenRGB uses for its "Direct" mode (always last)
#define OPENRGB_DIRECT_POSITION (OPENRGB_MODE_COUNT + 1)

// This Keychron fork reloads hsv/speed from EEPROM inside rgb_task_render()
// whenever the effect changes (quantum/rgb_matrix/rgb_matrix.c, "reload config
// if effect changed"), which silently discards a sethsv_noeeprom() that came
// with a mode change. Remember what OpenRGB asked for and re-assert it from the
// indicators hook for a few frames after SET_MODE, until the switch-over is done.
#define OPENRGB_HSV_REASSERT_MS 250
static struct {
    bool     active;
    uint32_t until;
    uint8_t  h, s, v, speed;
} pending_hsv;

// Forward declarations of helper functions
static uint8_t qmk_mode_to_openrgb(uint8_t qmk_mode);
static uint8_t openrgb_mode_to_qmk(uint8_t position);
static uint8_t get_led_keycode(uint8_t led_index);
static void    send_response(uint8_t *response);

bool openrgb_raw_hid_rx(uint8_t *data, uint8_t length) {
    uint8_t response[RAW_EPSIZE] = {0};
    response[0]                  = data[0];

    switch (data[0]) {
        case QMK_OPENRGB_GET_PROTOCOL_VERSION:
            response[1] = OPENRGB_PROTOCOL_VERSION;
            send_response(response);
            break;

        case QMK_OPENRGB_GET_QMK_VERSION:
            strncpy((char *)&response[1], QMK_VERSION, RAW_EPSIZE - 3);
            send_response(response);
            break;

        case QMK_OPENRGB_GET_DEVICE_INFO: {
            response[1]    = RGB_MATRIX_LED_COUNT;
            response[2]    = RGB_MATRIX_LED_COUNT;
            uint8_t offset = 3;
            strncpy((char *)&response[offset], OPENRGB_DEVICE_NAME, (RAW_EPSIZE - 2) / 2 - offset);
            offset += strlen((char *)&response[offset]) + 1;
            if (offset < RAW_EPSIZE - 3) {
                strncpy((char *)&response[offset], OPENRGB_DEVICE_VENDOR, RAW_EPSIZE - 2 - offset);
            }
            send_response(response);
            break;
        }

        case QMK_OPENRGB_GET_MODE_INFO:
            response[1] = qmk_mode_to_openrgb(rgb_matrix_get_mode());
            response[2] = rgb_matrix_get_speed();
            response[3] = rgb_matrix_get_hue();
            response[4] = rgb_matrix_get_sat();
            response[5] = rgb_matrix_get_val();
            send_response(response);
            break;

        case QMK_OPENRGB_GET_LED_INFO: {
            uint8_t start = data[1];
            uint8_t count = data[2];
            if (count > 8) count = 8;
            for (uint8_t i = 0; i < count; i++) {
                uint8_t led_idx = start + i;
                uint8_t base    = 1 + i * 7;
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
                    response[base + 2] = QMK_OPENRGB_FAILURE;
                }
            }
            send_response(response);
            break;
        }

        case QMK_OPENRGB_GET_ENABLED_MODES: {
            uint8_t idx = 1;
            for (uint8_t i = 0; i < OPENRGB_MODE_COUNT; i++) {
                response[idx++] = openrgb_modes[i].openrgb_id;
            }
            response[idx++] = QMK_OPENRGB_MODE_OPENRGB_DIRECT;
            response[idx]   = 0; // list terminator
            send_response(response);
            break;
        }

        case QMK_OPENRGB_SET_MODE: {
            uint8_t hue   = data[1];
            uint8_t sat   = data[2];
            uint8_t val   = data[3];
            uint8_t mode  = data[4];
            uint8_t speed = data[5];
            uint8_t save  = data[6];

            if (mode == OPENRGB_DIRECT_POSITION) {
                openrgb_mode_enable();
            } else {
                uint8_t qmk_mode = openrgb_mode_to_qmk(mode);
                if (save) {
                    rgb_matrix_sethsv(hue, sat, val);
                    rgb_matrix_set_speed(speed);
                    rgb_matrix_mode(qmk_mode);
                } else {
                    rgb_matrix_mode_noeeprom(qmk_mode);
                    rgb_matrix_sethsv_noeeprom(hue, sat, val);
                    rgb_matrix_set_speed_noeeprom(speed);
                    pending_hsv = (typeof(pending_hsv)){
                        .active = true,
                        .until  = timer_read32() + OPENRGB_HSV_REASSERT_MS,
                        .h      = hue,
                        .s      = sat,
                        .v      = val,
                        .speed  = speed,
                    };
                }
            }
            // OpenRGB waits (up to 50 ms) for a reply here; ack so it doesn't have to time out
            response[1] = QMK_OPENRGB_SUCCESS;
            send_response(response);
            break;
        }

        case QMK_OPENRGB_DIRECT_MODE_SET_SINGLE_LED: {
            uint8_t led = data[1];
            if (led < RGB_MATRIX_LED_COUNT) {
                srgb_led_buffer[led] = (rgb_led_t){data[2], data[3], data[4]};
                response[1]          = QMK_OPENRGB_SUCCESS;
            } else {
                response[1] = QMK_OPENRGB_FAILURE;
            }
            // OpenRGB reads a reply for single-LED writes too
            send_response(response);
            break;
        }

        case QMK_OPENRGB_DIRECT_MODE_SET_LEDS: {
            uint8_t start = data[1];
            uint8_t count = data[2];
            if (count > 20) count = 20;
            for (uint8_t i = 0; i < count; i++) {
                uint8_t led_idx = start + i;
                uint8_t offset  = 3 + i * 3;
                if (led_idx < RGB_MATRIX_LED_COUNT) {
                    srgb_led_buffer[led_idx] = (rgb_led_t){data[offset], data[offset + 1], data[offset + 2]};
                }
            }
            // No reply: OpenRGB never reads one for bulk writes, and an unread
            // report would sit in the host's hidraw queue and corrupt the next query.
            break;
        }

        default:
            return false;
    }
    return true;
}

void openrgb_reassert_pending_hsv(void) {
    if (!pending_hsv.active) return;
    if (timer_expired32(timer_read32(), pending_hsv.until)) {
        pending_hsv.active = false;
        return;
    }
    if (rgb_matrix_get_hue() != pending_hsv.h || rgb_matrix_get_sat() != pending_hsv.s || rgb_matrix_get_val() != pending_hsv.v) {
        rgb_matrix_sethsv_noeeprom(pending_hsv.h, pending_hsv.s, pending_hsv.v);
    }
    if (rgb_matrix_get_speed() != pending_hsv.speed) {
        rgb_matrix_set_speed_noeeprom(pending_hsv.speed);
    }
}

static void send_response(uint8_t *response) {
    response[RAW_EPSIZE - 1] = QMK_OPENRGB_END_OF_MESSAGE;
    raw_hid_send(response, RAW_EPSIZE);
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

// QMK effect -> OpenRGB positional mode value (1-based index into openrgb_modes)
static uint8_t qmk_mode_to_openrgb(uint8_t qmk_mode) {
    if (qmk_mode == RGB_MATRIX_CUSTOM_SIGNALRGB) {
        return OPENRGB_DIRECT_POSITION;
    }
    for (uint8_t i = 0; i < OPENRGB_MODE_COUNT; i++) {
        if (openrgb_modes[i].qmk_mode == qmk_mode) {
            return i + 1;
        }
    }
    return 1;
}

// OpenRGB positional mode value -> QMK effect
static uint8_t openrgb_mode_to_qmk(uint8_t position) {
    if (position == OPENRGB_DIRECT_POSITION) {
        return RGB_MATRIX_CUSTOM_SIGNALRGB;
    }
    if (position >= 1 && position <= OPENRGB_MODE_COUNT) {
        return openrgb_modes[position - 1].qmk_mode;
    }
    return RGB_MATRIX_SOLID_COLOR;
}

// Basic keycode of the key sitting on this LED (used by OpenRGB for key names)
static uint8_t get_led_keycode(uint8_t led_index) {
    if (led_index >= RGB_MATRIX_LED_COUNT) return 0;
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (g_led_config.matrix_co[row][col] == led_index) {
                uint16_t keycode = keymap_key_to_keycode(0, (keypos_t){col, row});
                if (IS_QK_MOD_TAP(keycode)) keycode = QK_MOD_TAP_GET_TAP_KEYCODE(keycode);
                else if (IS_QK_LAYER_TAP(keycode)) keycode = QK_LAYER_TAP_GET_TAP_KEYCODE(keycode);
                else if (IS_QK_MODS(keycode)) keycode = QK_MODS_GET_BASIC_KEYCODE(keycode);
                return IS_BASIC_KEYCODE(keycode) ? (uint8_t)keycode : 0;
            }
        }
    }
    return 0;
}
