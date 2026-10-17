#include "overlay.h"

#ifdef LK_WIRELESS_ENABLE
#    include "host_protocol.h"
#    include "openrgb.h"
#    include "simon.h"

#    define MASK_BYTES ((RGB_MATRIX_LED_COUNT + 7) / 8)
#    define MAX_SLOTS 16
#    define NUMBER_KEYS 10
#    define NO_NUMBER 255
// Modifiers show after being held this long, as NOTIFY_MODS on USB
#    define MODS_DELAY_MS 200
// Urgent keys breathe between 20% and 100%
#    define URGENT_FLOOR 51

#    define LOOK_WORKSPACES 1
#    define LOOK_URGENT 2

_Static_assert(6 + MASK_BYTES <= 32, "an LED bitmask must fit a 2.4 GHz packet");

extern rgb_led_t srgb_led_buffer[];

// A modifier combo's keys
typedef struct {
    uint8_t   mods;
    rgb_led_t color;
    uint8_t   mask[MASK_BYTES];
} slot_t;

// A highlight fading towards shown or hidden
typedef struct {
    bool     shown;
    uint8_t  from; // alpha when the fade started
    uint32_t since;
    uint8_t  alpha; // this frame's, 0-255
} fader_t;

static bool loaded;

static struct {
    uint8_t   submap_dim, submap_fade4, mods_dim, mods_fade4;
    rgb_led_t occupied, active, urgent;
    uint8_t   period20;
    uint8_t   flags;
} look = {.submap_dim = 64, .submap_fade4 = 30, .mods_dim = 64, .mods_fade4 = 30, .period20 = 100};

// Number key (0-9 for keys 1..9, 0) per LED, NO_NUMBER for the rest
static uint8_t number_of_led[RGB_MATRIX_LED_COUNT] = {[0 ... RGB_MATRIX_LED_COUNT - 1] = NO_NUMBER};

static slot_t  slots[MAX_SLOTS];
static uint8_t slot_count;

static struct {
    rgb_led_t color;
    uint8_t   mask[MASK_BYTES];
} submap;
static fader_t submap_fader;

static fader_t  mods_fader;
static int8_t   mods_slot = -1; // the slot on the keys, kept while it fades out
static uint8_t  mods_shown;     // and its modifiers, for the workspaces
static uint8_t  mods_held;
static uint32_t mods_since;
static uint8_t  mods_reported;

static uint32_t ws_active, ws_occupied, ws_urgent;
static uint8_t  urgent_level;

static bool led_bit(const uint8_t *mask, uint8_t led) {
    return mask[led / 8] & (1 << (led % 8));
}

static uint32_t read24(const uint8_t *p) {
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16;
}

static rgb_led_t read_rgb(const uint8_t *p) {
    return (rgb_led_t){p[0], p[1], p[2]};
}

static void fader_set(fader_t *f, bool shown) {
    if (f->shown == shown) return;
    f->shown = shown;
    f->from  = f->alpha;
    f->since = timer_read32();
}

static void fader_step(fader_t *f, uint8_t fade4) {
    uint8_t  target = f->shown ? 255 : 0;
    uint32_t span   = fade4 * 4;
    uint32_t t      = timer_elapsed32(f->since);
    if (span == 0 || t >= span) {
        f->alpha = target;
        return;
    }
    f->alpha = f->from + ((int32_t)target - f->from) * (int32_t)t / (int32_t)span;
}

static int8_t find_slot(uint8_t mods) {
    for (uint8_t i = 0; i < slot_count; i++) {
        if (slots[i].mods == mods) return i;
    }
    return -1;
}

// The host's lighting on the keys, in direct mode
static void show(void) {
    loaded = true;
    if (rgb_matrix_get_mode() != RGB_MATRIX_CUSTOM_SIGNALRGB) {
        // Undims first: waking from the inactivity dim restores the effect it
        // saved, which would replace this one on the next key press
        host_activity();
        openrgb_mode_enable();
    }
}

void overlay_rx(uint8_t *data, uint8_t length) {
    if (length < 32) return;
    switch (data[1]) {
        case OVERLAY_RELEASE:
            overlay_release();
            break;

        case OVERLAY_BASE_FILL: {
            rgb_led_t c = read_rgb(&data[2]);
            for (uint8_t i = 0; i < RGB_MATRIX_LED_COUNT; i++) {
                srgb_led_buffer[i] = c;
            }
            show();
            break;
        }

        case OVERLAY_BASE_LEDS: {
            uint8_t first = data[2];
            uint8_t count = MIN(data[3], 9);
            for (uint8_t k = 0; k < count && first + k < RGB_MATRIX_LED_COUNT; k++) {
                srgb_led_buffer[first + k] = read_rgb(&data[4 + k * 3]);
            }
            show();
            break;
        }

        case OVERLAY_LOOK:
            look.submap_dim   = data[2];
            look.submap_fade4 = data[3];
            look.mods_dim     = data[4];
            look.mods_fade4   = data[5];
            look.occupied     = read_rgb(&data[6]);
            look.active       = read_rgb(&data[9]);
            look.urgent       = read_rgb(&data[12]);
            look.period20     = MAX(data[15], 10);
            look.flags        = data[16];
            memset(number_of_led, NO_NUMBER, sizeof(number_of_led));
            for (uint8_t n = 0; n < NUMBER_KEYS; n++) {
                uint8_t led = data[17 + n];
                if (led < RGB_MATRIX_LED_COUNT) number_of_led[led] = n;
            }
            break;

        case OVERLAY_SLOTS_CLEAR:
            slot_count = 0;
            mods_slot  = -1;
            break;

        case OVERLAY_SLOT: {
            int8_t i = find_slot(data[2]);
            if (i < 0) {
                if (slot_count == MAX_SLOTS) break;
                i = slot_count++;
            }
            slots[i].mods  = data[2];
            slots[i].color = read_rgb(&data[3]);
            memcpy(slots[i].mask, &data[6], MASK_BYTES);
            break;
        }

        case OVERLAY_SUBMAP:
            // Hidden keeps the keys, to fade them out
            if (data[2]) {
                submap.color = read_rgb(&data[3]);
                memcpy(submap.mask, &data[6], MASK_BYTES);
            }
            fader_set(&submap_fader, data[2] != 0);
            break;

        case OVERLAY_WORKSPACES:
            ws_active   = read24(&data[2]);
            ws_occupied = read24(&data[5]);
            ws_urgent   = read24(&data[8]);
            break;
    }
}

bool overlay_loaded(void) {
    return loaded;
}

void overlay_release(void) {
    if (!loaded) return;
    loaded       = false;
    slot_count   = 0;
    mods_slot    = -1;
    submap_fader = mods_fader = (fader_t){0};
    ws_active = ws_occupied = ws_urgent = 0;
    if (rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_SIGNALRGB) {
        openrgb_mode_disable();
    }
}

// Held modifiers pick a slot as NOTIFY_MODS would report them: after
// MODS_DELAY_MS, changes right away while shown, and the release
static void mods_step(void) {
    uint8_t now = host_mods();
    if (now != mods_held) {
        mods_held  = now;
        mods_since = timer_read32();
    }
    mods_reported = (mods_held && (mods_reported || timer_elapsed32(mods_since) >= MODS_DELAY_MS)) ? mods_held : 0;
    int8_t slot   = mods_reported ? find_slot(mods_reported) : -1;
    if (slot >= 0) {
        mods_slot  = slot;
        mods_shown = mods_reported;
    }
    fader_set(&mods_fader, slot >= 0);
    fader_step(&mods_fader, look.mods_fade4);
}

void overlay_frame(void) {
    if (!loaded) return;
    fader_step(&submap_fader, look.submap_fade4);
    mods_step();
    // Triangle wave
    uint32_t period = look.period20 * 20;
    uint32_t phase  = (timer_read32() % period) * 510 / period;
    uint8_t  tri    = phase <= 255 ? phase : 510 - phase;
    urgent_level    = URGENT_FLOOR + (uint16_t)tri * (255 - URGENT_FLOOR) / 255;
}

static uint8_t mix(uint8_t from, uint8_t to, uint8_t amount) {
    return from + ((int16_t)to - from) * amount / 255;
}

static rgb_led_t scaled(rgb_led_t c, uint8_t level) {
    return (rgb_led_t){(uint16_t)c.r * level / 255, (uint16_t)c.g * level / 255, (uint16_t)c.b * level / 255};
}

// One highlight: its keys fade to their colour, the rest dim
static rgb_led_t highlight(rgb_led_t c, bool lit, rgb_led_t color, uint8_t dim, uint8_t alpha) {
    if (alpha == 0) return c;
    if (lit) return (rgb_led_t){mix(c.r, color.r, alpha), mix(c.g, color.g, alpha), mix(c.b, color.b, alpha)};
    return scaled(c, mix(255, dim, alpha));
}

rgb_led_t overlay_color(uint8_t led) {
    rgb_led_t c = srgb_led_buffer[led];
    if (!loaded) return c;

    c = highlight(c, led_bit(submap.mask, led), submap.color, look.submap_dim, submap_fader.alpha);

    if (mods_fader.alpha && mods_slot >= 0) {
        bool      lit    = led_bit(slots[mods_slot].mask, led);
        rgb_led_t color  = slots[mods_slot].color;
        uint8_t   number = number_of_led[led];
        // SUPER (not CTRL) shows workspaces 1-10 on the number keys, 11-20 with ALT
        if (number != NO_NUMBER && (look.flags & LOOK_WORKSPACES) && (mods_shown & HOST_MOD_SUPER) && !(mods_shown & HOST_MOD_CTRL)) {
            uint32_t bit = 1UL << (number + ((mods_shown & HOST_MOD_ALT) ? 10 : 0));
            lit          = (ws_active | ws_occupied) & bit;
            color        = (ws_active & bit) ? look.active : look.occupied;
        }
        c = highlight(c, lit, color, look.mods_dim, mods_fader.alpha);
    }

    uint8_t number = number_of_led[led];
    if ((look.flags & LOOK_URGENT) && number != NO_NUMBER && (ws_urgent & ((1UL << number) | (1UL << (number + 10))))) {
        c = scaled(look.urgent, urgent_level);
    }
    return c;
}
#endif
