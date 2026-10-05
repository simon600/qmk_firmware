#include "host_protocol.h"
#include "simon.h"
#include "raw_hid.h"
#include "version.h"
#ifdef ANANLOG_MATRIX
#    include "profile.h"
#endif

// Quiet window after OpenRGB's protocol-version query; its detection takes ~50 ms
#define HOST_QUIET_MS 1000
// Keyboard-side changes are coalesced into at most one notification per interval
#define HOST_NOTIFY_INTERVAL_MS 100

#define HOST_STATE_LEN 5

static uint32_t quiet_since;
static bool     quiet;
static uint32_t last_state_check;
static uint8_t  baseline_state[HOST_STATE_LEN];
static bool     baseline_valid;
static bool     notify_pending;

__attribute__((weak)) uint8_t host_gaming_state_user(void) {
    return HOST_STATE_UNSUPPORTED;
}

__attribute__((weak)) bool host_gaming_set_user(uint8_t state) {
    return false;
}

static bool in_quiet_window(void) {
    if (quiet && timer_elapsed32(quiet_since) >= HOST_QUIET_MS) {
        quiet = false;
    }
    return quiet;
}

static void host_send(uint8_t *packet) {
    if (in_quiet_window()) return;
    raw_hid_send(packet, RAW_EPSIZE);
}

static void read_state(uint8_t *out) {
    user_config_t *cfg = get_user_config();
    out[0]             = host_gaming_state_user();
#ifdef ANANLOG_MATRIX
    out[1] = profile_get_current_index();
#else
    out[1] = HOST_STATE_UNSUPPORTED;
#endif
    out[2] = cfg->indicator_brightness;
    out[3] = cfg->indicators_always_on;
    out[4] = cfg->bg_blackout_mode;
}

// After a host command changed state, adopt it so it isn't echoed as a keyboard-side change
static void rebaseline(void) {
    read_state(baseline_state);
    baseline_valid = true;
}

static int8_t indicator_by_wire_id(uint8_t wire) {
    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        if (indicator_wire_id(i) == wire) return i;
    }
    return -1;
}

static void reply_info(uint8_t *reply) {
    uint8_t features = 0;
    if (host_gaming_state_user() != HOST_STATE_UNSUPPORTED) features |= HOST_FEATURE_GAMING;
#ifdef ANANLOG_MATRIX
    features |= HOST_FEATURE_HE_PROFILES;
#endif
#if (EECONFIG_USER_DATA_SIZE) > 0
    features |= HOST_FEATURE_PERSISTENT_COLORS;
#endif
    reply[1] = HOST_PROTOCOL_VERSION;
    reply[2] = features;
    reply[3] = INDICATOR_COUNT;
    reply[4] = INDICATOR_MAX_STATES;
    strncpy((char *)&reply[5], QMK_BUILDDATE, RAW_EPSIZE - 6);
}

static void reply_indicators(uint8_t *reply) {
    uint8_t pos = 2;
    reply[1]    = INDICATOR_COUNT;
    for (uint8_t i = 0; i < INDICATOR_COUNT; i++) {
        uint8_t states = indicator_state_count(i);
        if (pos + 3 + states * 3 > RAW_EPSIZE) break;
        reply[pos++] = indicator_wire_id(i);
        reply[pos++] = states;
        reply[pos++] = get_indicators()[i].led_index;
        for (uint8_t s = 0; s < states; s++) {
            rgb_led_t c  = indicator_color_get(i, s);
            reply[pos++] = c.r;
            reply[pos++] = c.g;
            reply[pos++] = c.b;
        }
    }
}

static void set_setting(uint8_t setting, uint8_t value, bool persist) {
    user_config_t *cfg = get_user_config();
    switch (setting) {
        case HOST_SETTING_INDICATOR_BRIGHTNESS:
            cfg->indicator_brightness = value;
            if (persist) user_config_save();
            break;
        case HOST_SETTING_INDICATORS_ALWAYS_ON:
            indicators_set_always_on(value != 0, persist);
            break;
        case HOST_SETTING_BLACKOUT:
            cfg->bg_blackout_mode = value != 0;
            if (persist) user_config_save();
            break;
    }
}

bool host_protocol_rx(uint8_t *data, uint8_t length) {
    if (data[0] < 0xC0 || data[0] > 0xCF) return false;

    uint8_t reply[RAW_EPSIZE] = {0};
    reply[0]                  = data[0];

    switch (data[0]) {
        case HOST_GET_INFO:
            reply_info(reply);
            host_send(reply);
            break;

        case HOST_GET_STATE:
            read_state(&reply[1]);
            host_send(reply);
            break;

        case HOST_SET_GAMING:
            host_activity();
            host_gaming_set_user(data[1]);
            rebaseline();
            break;

        case HOST_GET_INDICATORS:
            reply_indicators(reply);
            host_send(reply);
            break;

        case HOST_SET_INDICATOR_COLOR: {
            int8_t id = indicator_by_wire_id(data[1]);
            if (id >= 0) {
                host_activity();
                indicator_color_set(id, data[2], (rgb_led_t){data[3], data[4], data[5]}, data[6] != 0);
            }
            break;
        }

        case HOST_SET_SETTING:
            host_activity();
            set_setting(data[1], data[2], data[3] != 0);
            rebaseline();
            break;

        case HOST_RESET_INDICATOR_COLORS:
            host_activity();
            indicator_colors_reset(data[1] != 0);
            break;

        default:
            // Unknown command in our range: swallow it so nobody else answers
            break;
    }
    return true;
}

void host_protocol_openrgb_command(uint8_t command) {
    if (command == 0x01) {
        quiet_since = timer_read32();
        quiet       = true;
    }
}

void host_protocol_task(void) {
    if (timer_elapsed32(last_state_check) < HOST_NOTIFY_INTERVAL_MS) return;
    last_state_check = timer_read32();

    uint8_t current[HOST_STATE_LEN];
    read_state(current);
    if (!baseline_valid) {
        memcpy(baseline_state, current, HOST_STATE_LEN);
        baseline_valid = true;
        return;
    }
    if (memcmp(current, baseline_state, HOST_STATE_LEN) != 0) {
        memcpy(baseline_state, current, HOST_STATE_LEN);
        notify_pending = true;
    }
    if (notify_pending && !in_quiet_window()) {
        uint8_t packet[RAW_EPSIZE] = {0};
        packet[0]                  = HOST_NOTIFY_STATE;
        memcpy(&packet[1], baseline_state, HOST_STATE_LEN);
        raw_hid_send(packet, RAW_EPSIZE);
        notify_pending = false;
    }
}
