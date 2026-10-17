#include "host_protocol.h"
#include "simon.h"
#include "host_raw.h"
#include "version.h"
#include "usb_device_state.h"
#ifdef LK_WIRELESS_ENABLE
#    include "transport.h"
#    include "wireless.h"
#    include "keychron_raw_hid.h"
#    include "overlay.h"
#endif
#ifdef ANANLOG_MATRIX
#    include "profile.h"
#endif

// Keyboard-side changes are coalesced into at most one notification per interval
#define HOST_NOTIFY_INTERVAL_MS 100
// Modifiers must be held this long before they're reported (quick combos stay quiet)
#define HOST_MODS_DELAY_MS 200
// The host pings every 2 s; silence this long means nobody is listening
#define HOST_LISTEN_TIMEOUT_MS 6000
// After USB comes back, how long a host has to speak up before the keyboard
// takes its lighting back. Waking this desktop (NVIDIA) takes a few seconds
// before kbd-daemon runs again.
#define HOST_HANDBACK_MS 15000

#define HOST_STATE_LEN 7

// 2.4 GHz: the module relays 32-byte reports. While the keyboard is in use it
// sends a heartbeat this often; the host answers each one
#define HOST_WL_PACKET_LEN 32
#define HOST_HEARTBEAT_MS 2000
// Reports the keyboard sends back to back get lost: an idle notice waits this
// long after the host's last packet (its queries come one at a time)
#define HOST_WL_QUIET_MS 250

static uint32_t last_state_check;
static uint8_t  baseline_state[HOST_STATE_LEN];
static bool     baseline_valid;
static bool     notify_pending;

// Held modifiers (HOST_MOD_*): what is held now, since when, and what the host last got
static uint8_t  mods_held;
static uint32_t mods_since;
static uint8_t  mods_reported;

// Last command from the host, for host_listening()
static uint32_t last_contact;
static bool     contacted;

// Set from the USB interrupt; the colours are reloaded (EEPROM, I2C) in the main loop
static volatile bool colors_reload_pending;
// USB went away (suspend, KVM switch, replug); set from the USB interrupt
static volatile bool usb_dropped;
static bool          handback_armed;
static uint32_t      handback_since;
// The keyboard shows its own lighting (just started, or took it back); the next
// host to speak hears about it
static bool handed_back = true;

#ifdef LK_WIRELESS_ENABLE
// 2.4 GHz host. The keyboard speaks once a host has, or once to say it's
// there when the link comes up: then it sends a heartbeat while in use and
// goes quiet when the backlight times out, so the
// host never has to send anything to a keyboard that may be asleep (any packet
// would wake it, and a wake keeps it up for another 30 minutes). Sleep keeps
// RAM, so the host is remembered across sleep and link drops; it's forgotten
// when the keyboard leaves 2.4 GHz or a host stays silent after a heartbeat.
static bool     wl_host;      // a host spoke over 2.4 GHz and hasn't gone silent
static bool     wl_confirmed; // ...and answered since the keyboard woke / reconnected
static bool     wl_idle;      // the host was told the keyboard is idle
static bool     wl_was_connected;
static bool     wl_handback_armed; // the link came up: a host must speak up within HOST_HANDBACK_MS
static uint32_t wl_handback_since;
static uint32_t wl_last_contact;
static uint32_t wl_last_beat;
#endif

__attribute__((weak)) uint8_t host_gaming_state_user(void) {
    return HOST_STATE_UNSUPPORTED;
}

__attribute__((weak)) bool host_gaming_set_user(uint8_t state) {
    return false;
}

__attribute__((weak)) uint8_t host_base_layer_user(void) {
    return HOST_BASE_LAYER_UNKNOWN;
}

__attribute__((weak)) bool host_handback_user(void) {
    return false;
}

// The host is the computer on USB: while typing goes elsewhere (wireless, cable
// pulled, host asleep) there's nobody to notify
static bool on_usb_transport(void) {
#ifdef LK_WIRELESS_ENABLE
    return get_transport() == TRANSPORT_USB;
#else
    return true;
#endif
}

static bool host_link_up(void) {
    return on_usb_transport() && usb_device_state_get_configure_state() == USB_DEVICE_STATE_CONFIGURED;
}

// Unsaved colour overrides (an OpenRGB profile's) belong to the computer that
// sent them: drop them when USB drops to unconfigured (KVM switch, replug).
// Suspend keeps them: same computer. kbd-daemon sends them again when it sees
// the keyboard come back. Lighting can't go by that: waking from S3 resets the
// keyboard just like a KVM switch does, so it waits for a host instead
// (host_handback_user). Runs in the USB interrupt.
void notify_usb_device_state_change_user(struct usb_device_state usb_device_state) {
    if (usb_device_state.configure_state == USB_DEVICE_STATE_NO_INIT || usb_device_state.configure_state == USB_DEVICE_STATE_INIT) {
        colors_reload_pending = true;
    }
    if (usb_device_state.configure_state != USB_DEVICE_STATE_CONFIGURED) {
        usb_dropped = true;
    }
}

static uint8_t link_packet_len(uint8_t link) {
    return link == HOST_LINK_WIRELESS ? HOST_WL_PACKET_LEN : HOST_PACKET_LEN;
}

static void host_send(uint8_t link, uint8_t *packet) {
#ifdef LK_WIRELESS_ENABLE
    if (link == HOST_LINK_WIRELESS) {
        kc_raw_hid_send(RAW_HID_SRC_WIRELESS, packet, HOST_WL_PACKET_LEN);
        return;
    }
#endif
    host_packet_send(packet);
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
    out[5] = host_base_layer_user();
    out[6] = layer_dim_get();
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

static void reply_info(uint8_t len, uint8_t *reply) {
    uint8_t features = 0;
    if (host_gaming_state_user() != HOST_STATE_UNSUPPORTED) features |= HOST_FEATURE_GAMING;
#ifdef ANANLOG_MATRIX
    features |= HOST_FEATURE_HE_PROFILES;
#endif
#if (EECONFIG_USER_DATA_SIZE) > 0
    features |= HOST_FEATURE_PERSISTENT_COLORS;
#endif
    features |= HOST_FEATURE_MODS_NOTIFY | HOST_FEATURE_RGB_KEYS;
#ifdef RGB_RAW_ENABLE
    features |= HOST_FEATURE_OWN_INTERFACE;
#endif
#ifdef LK_WIRELESS_ENABLE
    features |= HOST_FEATURE_WIRELESS | HOST_FEATURE_LIGHTING;
#endif
    reply[1] = HOST_PROTOCOL_VERSION;
    reply[2] = features;
    reply[3] = INDICATOR_COUNT;
    reply[4] = INDICATOR_MAX_STATES;
    strncpy((char *)&reply[5], QMK_BUILDDATE, len - 6);
}

static void reply_indicators(uint8_t len, uint8_t first, uint8_t *reply) {
    uint8_t pos   = 4;
    uint8_t count = 0;
    reply[1]      = INDICATOR_COUNT;
    reply[2]      = first;
    for (uint8_t i = first; i < INDICATOR_COUNT; i++) {
        uint8_t states = indicator_state_count(i);
        if (pos + 3 + states * 3 > len) break; // rest on the next page
        reply[pos++] = indicator_wire_id(i);
        reply[pos++] = states;
        reply[pos++] = get_indicators()[i].led_index;
        for (uint8_t s = 0; s < states; s++) {
            rgb_led_t c  = indicator_color_get(i, s);
            reply[pos++] = c.r;
            reply[pos++] = c.g;
            reply[pos++] = c.b;
        }
        count++;
    }
    reply[3] = count;
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
        case HOST_SETTING_LAYER_DIM:
            layer_dim_set(value, persist);
            break;
        case HOST_SETTING_BLACKOUT:
            cfg->bg_blackout_mode = value != 0;
            if (persist) user_config_save();
            break;
    }
}

#ifdef LK_WIRELESS_ENABLE
static bool wl_link_mode(void) {
    return get_transport() == TRANSPORT_P2P4;
}

static bool wl_link_up(void) {
    return wl_link_mode() && wireless_get_state() == WT_CONNECTED;
}

// The keyboard's input (keys, encoder) has gone quiet for the backlight timeout
static bool wl_keyboard_idle(void) {
    return rgb_matrix_timeouted();
}

// [1] 0 in use (the host answers with any command, e.g. PING), 1 idle
static void wl_send_heartbeat(bool idle) {
    uint8_t packet[HOST_PACKET_LEN] = {0};
    packet[0]                       = HOST_PING;
    packet[1]                       = idle ? HOST_HEARTBEAT_IDLE : HOST_HEARTBEAT_ACTIVE;
    packet[2]                       = overlay_loaded();
    host_send(HOST_LINK_WIRELESS, packet);
    wl_last_beat = timer_read32();
}

static bool wl_listening(void) {
    return wl_host && wl_confirmed && !wl_idle && wl_link_up();
}
#endif

bool host_protocol_rx(uint8_t link, uint8_t *data, uint8_t length) {
    if (data[0] < 0xC0 || data[0] > 0xCF) return false;

    if (link == HOST_LINK_WIRELESS) {
#ifdef LK_WIRELESS_ENABLE
        // Bluetooth stays as it was: VIA's
        if (!wl_link_mode()) return false;
        wl_host           = true;
        wl_confirmed      = true;
        wl_handback_armed = false;
        wl_last_contact   = timer_read32();
        // A host that speaks needs no heartbeat for a while; one now would go
        // out right behind the reply, and the link drops reports back to back
        wl_last_beat = wl_last_contact;
#else
        return false;
#endif
    } else {
        last_contact = timer_read32();
        contacted    = true;
        // The lighting is the USB host's business
        if (handed_back) {
            handed_back                     = false;
            uint8_t notice[HOST_PACKET_LEN] = {0};
            notice[0]                       = HOST_NOTIFY_HANDBACK;
            host_send(HOST_LINK_USB, notice);
        }
    }

    uint8_t len                    = link_packet_len(link);
    uint8_t reply[HOST_PACKET_LEN] = {0};
    reply[0]                       = data[0];

    switch (data[0]) {
        case HOST_GET_INFO:
            reply_info(len, reply);
            host_send(link, reply);
            break;

        case HOST_GET_STATE:
            read_state(&reply[1]);
            host_send(link, reply);
            break;

        case HOST_SET_GAMING:
            host_activity();
            host_gaming_set_user(data[1]);
            rebaseline();
            break;

        case HOST_GET_INDICATORS:
            reply_indicators(len, data[1], reply);
            host_send(link, reply);
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

        case HOST_RELOAD_INDICATOR_COLORS:
            indicator_colors_reload();
            break;

        case HOST_PING:
            // Contact is all it carries
            break;

#ifdef LK_WIRELESS_ENABLE
        case HOST_LIGHTING:
            // USB has OpenRGB's direct mode
            if (link == HOST_LINK_WIRELESS) overlay_rx(data, length);
            break;
#endif

        default:
            // Unknown command in our range: swallow it so nobody else answers
            break;
    }

#ifdef LK_WIRELESS_ENABLE
    // A host that speaks up while the keyboard is idle (e.g. it just started)
    // hears so once it's done, and waits for the next heartbeat
    if (link == HOST_LINK_WIRELESS) {
        wl_idle = false;
    }
#endif
    return true;
}

uint8_t host_mods(void) {
    uint8_t mods = get_mods();
    uint8_t out  = 0;
    if (mods & MOD_MASK_SHIFT) out |= HOST_MOD_SHIFT;
    if (mods & MOD_MASK_CTRL) out |= HOST_MOD_CTRL;
    if (mods & MOD_MASK_ALT) out |= HOST_MOD_ALT;
    if (mods & MOD_MASK_GUI) out |= HOST_MOD_SUPER;
    return out;
}

// NOTIFY_MODS for the host's shortcut highlight: reported after
// HOST_MODS_DELAY_MS, changes while reported and releases right away
static void notify_mods(void) {
    if (!host_listening()) {
        // Nobody to tell, or the host that got the last report is gone
        mods_held = mods_reported = 0;
        return;
    }
    uint8_t now = host_mods();
    if (now != mods_held) {
        mods_held  = now;
        mods_since = timer_read32();
    }
    uint8_t want = (mods_held && (mods_reported || timer_elapsed32(mods_since) >= HOST_MODS_DELAY_MS)) ? mods_held : 0;
    if (want == mods_reported) return;
    uint8_t packet[HOST_PACKET_LEN] = {0};
    packet[0]                  = HOST_NOTIFY_MODS;
    packet[1]                  = want;
    host_send(HOST_LINK_USB, packet);
    mods_reported = want;
}

bool host_listening(void) {
    return contacted && timer_elapsed32(last_contact) < HOST_LISTEN_TIMEOUT_MS && host_link_up();
}

#ifdef LK_WIRELESS_ENABLE
static bool wl_takes_rgb_keys(void) {
    return wl_listening() && overlay_loaded();
}
#endif

bool host_takes_rgb_keys(void) {
#ifdef LK_WIRELESS_ENABLE
    if (wl_takes_rgb_keys()) return true;
#endif
    return host_listening();
}

void host_notify_rgb_key(uint8_t key) {
    uint8_t packet[HOST_PACKET_LEN] = {0};
    packet[0]                  = HOST_NOTIFY_RGB_KEY;
    packet[1]                  = key;
#ifdef LK_WIRELESS_ENABLE
    if (wl_takes_rgb_keys()) {
        host_send(HOST_LINK_WIRELESS, packet);
        return;
    }
#endif
    host_send(HOST_LINK_USB, packet);
}

#ifdef LK_WIRELESS_ENABLE
static void wl_task(void) {
    if (!wl_link_mode()) {
        // Left 2.4 GHz: whoever is on the other end now introduces itself
        // again, and the lighting there was its
        wl_host = wl_confirmed = wl_idle = wl_was_connected = wl_handback_armed = false;
        overlay_release();
        return;
    }
    bool connected = wireless_get_state() == WT_CONNECTED;
    bool came_up   = connected && !wl_was_connected;
    wl_was_connected = connected;
    if (!connected) return;
    if (came_up) {
        wl_handback_armed = true;
        wl_handback_since = timer_read32();
    }
    // The link came back and no host spoke up (another computer, kbd-daemon
    // gone): its lighting goes, as on USB
    if (wl_handback_armed && timer_elapsed32(wl_handback_since) >= HOST_HANDBACK_MS) {
        wl_handback_armed = false;
        overlay_release();
    }
    if (came_up && !wl_host) {
        // The link just came up (power on, back to 2.4 GHz, reconnect) and no
        // host is known: a host waiting quietly on the other end can't know
        // the keyboard is back, so it gets a heartbeat. Kept only if it answers.
        wl_host = true;
    }
    if (!wl_host) return;

    if (wl_keyboard_idle()) {
        if (!wl_idle && timer_elapsed32(wl_last_contact) >= HOST_WL_QUIET_MS) {
            wl_idle = true;
            wl_send_heartbeat(true);
        }
        return;
    }

    if (wl_idle || came_up) {
        // In use again (key press, or the link is back): the host gets a
        // moment to answer, as if it had just spoken
        wl_idle         = false;
        wl_confirmed    = false;
        wl_last_contact = timer_read32();
        wl_send_heartbeat(false);
        return;
    }

    if (timer_elapsed32(wl_last_contact) >= HOST_LISTEN_TIMEOUT_MS) {
        // No answer to the last heartbeats: no host (another computer, or
        // kbd-daemon stopped). Quiet until one speaks.
        wl_host = wl_confirmed = false;
        return;
    }
    if (timer_elapsed32(wl_last_beat) >= HOST_HEARTBEAT_MS) {
        wl_send_heartbeat(false);
    }
}
#endif

void host_protocol_task(void) {
#ifdef LK_WIRELESS_ENABLE
    wl_task();
#endif
    // Also cleared here, so the 32-bit timer wrapping can't bring a long-gone
    // host back
    if (contacted && timer_elapsed32(last_contact) >= HOST_LISTEN_TIMEOUT_MS) {
        contacted = false;
    }
    if (colors_reload_pending) {
        colors_reload_pending = false;
        indicator_colors_reload();
    }
    // USB is back: a host that keeps pinging keeps its lighting
    if (usb_dropped && usb_device_state_get_configure_state() == USB_DEVICE_STATE_CONFIGURED) {
        usb_dropped    = false;
        handback_armed = true;
        handback_since = timer_read32();
    }
    if (handback_armed && timer_elapsed32(handback_since) >= HOST_HANDBACK_MS) {
        handback_armed = false;
        // Only while typing still goes over USB: on 2.4 GHz the lighting is
        // that host's (wl_task has its own handback)
        if (on_usb_transport() && !host_listening() && host_handback_user()) {
            handed_back = true;
        }
    }
    notify_mods();
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
    // Without a listening host the change is simply adopted: a host reads the
    // whole state when it connects (or, over 2.4 GHz, on the next heartbeat)
    if (notify_pending) {
        uint8_t packet[HOST_PACKET_LEN] = {0};
        packet[0]                       = HOST_NOTIFY_STATE;
        memcpy(&packet[1], baseline_state, HOST_STATE_LEN);
        if (host_listening()) {
            host_send(HOST_LINK_USB, packet);
        }
#ifdef LK_WIRELESS_ENABLE
        else if (wl_listening()) {
            host_send(HOST_LINK_WIRELESS, packet);
        }
#endif
    }
    notify_pending = false;
}
