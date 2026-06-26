#if defined(SIGNALRGB_ENABLE) || defined(OPENRGB_ENABLE)

RGB_MATRIX_EFFECT(SIGNALRGB)
#ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

extern rgb_led_t srgb_led_buffer[];

bool SIGNALRGB(effect_params_t* params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    for (uint8_t i = led_min; i < led_max; i++) {
        rgb_matrix_set_color(i, srgb_led_buffer[i].r, srgb_led_buffer[i].g, srgb_led_buffer[i].b);
    }

    return rgb_matrix_check_finished_leds(led_max);
}

#endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS

#endif
