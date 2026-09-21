#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)

RGB_MATRIX_EFFECT(SIGNALRGB)
#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

extern rgb_led_t srgb_led_buffer[];

bool SIGNALRGB(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    // Direct-mode frames (live OpenRGB/SignalRGB stream, or the last frame left
    // behind after the host disconnects) are raw pixel values with no built-in
    // brightness handling, so apply the hardware brightness (val) here to keep
    // the brightness knob/keys working regardless of source.
    uint8_t val = rgb_matrix_get_val();

    for (uint8_t i = led_min; i < led_max; i++) {
        uint8_t r = (uint16_t)srgb_led_buffer[i].r * val / 255;
        uint8_t g = (uint16_t)srgb_led_buffer[i].g * val / 255;
        uint8_t b = (uint16_t)srgb_led_buffer[i].b * val / 255;
        rgb_matrix_set_color(i, r, g, b);
    }

    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#endif
